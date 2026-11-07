#!/usr/bin/env python3
"""Give C++ enums the underlying type Watcom would have chosen.

CODE/MAKEFILE builds without /ei, so Watcom stored every enum in the smallest integer that holds
all its values (see Agent/knowledge_base/x64_porting_pitfalls.md #20). MSVC always uses 4 bytes
unless the enum has a fixed underlying type, which breaks every enum that is read from / written
to disk, sent over the network or saved by sizeof().

usage:  py -3 scripts/size_enums.py FILE.H [FILE2.H ...]            # dry run, prints a table
        py -3 scripts/size_enums.py --apply FILE.H [FILE2.H ...]    # rewrites "enum X {" to "enum X : T {"

Enums that already have an underlying type, anonymous enums, and enums whose values cannot be
evaluated (reported as SKIP) are left alone.
"""
import re
import sys


def strip_comments(text):
    text = re.sub(r'/\*.*?\*/', lambda m: ' ' * len(m.group(0)), text, flags=re.S)
    return re.sub(r'//[^\n]*', lambda m: ' ' * len(m.group(0)), text)


def collect_defines(text):
    defs = {}
    for m in re.finditer(r'^\s*#\s*define\s+(\w+)\s+([^\n]+)$', text, flags=re.M):
        defs[m.group(1)] = m.group(2).strip()
    return defs


def evaluate(expr, env, defs, depth=0):
    expr = expr.strip()
    # drop C casts like (int) and suffixes
    expr = re.sub(r'\((?:unsigned\s+|signed\s+)?(?:int|long|short|char)\)', '', expr)
    expr = re.sub(r'\b(0x[0-9a-fA-F]+|\d+)[uUlL]+\b', r'\1', expr)

    def repl(m):
        name = m.group(0)
        if name in env:
            return str(env[name])
        if name in defs and depth < 8:
            return '(' + str(evaluate(defs[name], env, defs, depth + 1)) + ')'
        raise KeyError(name)

    expr = re.sub(r'\b[A-Za-z_]\w*\b', repl, expr)
    if not re.fullmatch(r'[0-9xXa-fA-F+\-*/%<>&|^~() \t]+', expr):
        raise ValueError(expr)
    expr = expr.replace('/', '//')
    return int(eval(expr, {'__builtins__': {}}, {}))


def watcom_type(lo, hi):
    if lo >= 0:
        if hi <= 255:
            return 'unsigned char'
        if hi <= 65535:
            return 'unsigned short'
        return None  # unsigned int: leave as the (4-byte) default
    if lo >= -128 and hi <= 127:
        return 'signed char'
    if lo >= -32768 and hi <= 32767:
        return 'short'
    return None


ENUM_RE = re.compile(r'\benum\b(\s+(\w+))?(\s*:\s*[\w\s]+?)?\s*\{(.*?)\}', re.S)


def process(path, apply):
    raw = open(path, encoding='latin-1', newline='').read()
    text = strip_comments(raw)
    defs = collect_defines(text)
    out = []
    edits = []
    for m in ENUM_RE.finditer(text):
        name = m.group(2) or '<anon>'
        if m.group(3):
            out.append((name, 'has type', ''))
            continue
        body = m.group(4)
        # drop preprocessor lines inside the body; keep every branch (union of values)
        body = '\n'.join(l for l in body.split('\n') if not l.strip().startswith('#'))
        env = {}
        cur = -1
        lo = hi = None
        try:
            for item in body.split(','):
                item = item.strip()
                if not item:
                    continue
                if '=' in item:
                    key, expr = item.split('=', 1)
                    key = key.strip()
                    cur = evaluate(expr, env, defs)
                else:
                    key = item
                    cur += 1
                env[key] = cur
                lo = cur if lo is None else min(lo, cur)
                hi = cur if hi is None else max(hi, cur)
        except Exception as exc:  # noqa
            out.append((name, 'SKIP', repr(exc)))
            continue
        if lo is None:
            continue
        t = watcom_type(lo, hi)
        out.append((name, t or 'int(4)', '%d..%d' % (lo, hi)))
        if t and m.group(2):
            # position of "{" in the raw text equals position in the stripped text
            brace = text.index('{', m.start())
            edits.append((brace, t))
    for name, t, rng in out:
        print('%-22s %-16s %s' % (name, t, rng))
    if apply and edits:
        for brace, t in sorted(edits, reverse=True):
            raw = raw[:brace].rstrip(' \t') + ' : ' + t + ' ' + raw[brace:]
        open(path, 'w', encoding='latin-1', newline='').write(raw)
        print('applied %d edits to %s' % (len(edits), path))


if __name__ == '__main__':
    args = sys.argv[1:]
    apply = '--apply' in args
    for p in [a for a in args if a != '--apply']:
        print('==', p)
        process(p, apply)
