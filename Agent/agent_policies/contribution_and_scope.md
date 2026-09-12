---
name: contribution-and-scope
description: This checkout is a personal fork of EA's upstream repo, which accepts no contributions — scope agent work accordingly
metadata:
  type: agent_policy
---

# Contribution & Scope

## Remote configuration (verified 2026-09-12)
```
origin    https://github.com/Olli1080/CnC_Red_Alert.git   (fetch/push)
upstream  https://github.com/electronicarts/CnC_Red_Alert.git (fetch/push)
```
This is the user's own fork (`Olli1080/CnC_Red_Alert`) of EA's official release (`electronicarts/CnC_Red_Alert`). Git history at the time of this note is just three commits: EA's initial source drop, then `LICENSE.md`/`README.md`.

## What upstream will and won't accept
Per the root [README.md](../../README.md): *"This repository will not be accepting contributions (pull requests, issues, etc.). If you wish to create changes to the source code and encourage collaboration, please create a fork."* The repo is explicitly **archived for preservation, unsupported**, and exists to back the game's [Steam Workshop](https://steamcommunity.com/workshop/browse/?appid=2229840) mod support.

## What this means for agent work here
- **Never propose opening a PR or issue against `electronicarts/CnC_Red_Alert`** — it will be ignored/rejected by policy, not just unlikely to be merged.
- Treat this checkout as the user's **own sandbox**: modifications, mods, restoration-of-buildability work, or Workshop-content tooling are all in scope and land in `origin` (the user's fork), not upstream.
- If the user wants to publish changes, that means pushing to `origin` or making their own release — confirm with them before any push, per standard git-safety practice, and don't assume they want visibility beyond their own fork.
- If asked to "contribute back" or "send this upstream," clarify that this is explicitly out of scope per EA's own stated policy, and ask what they actually want to happen (keep on their fork? package as a Steam Workshop item? something else?).

## Related
- [licensing.md](licensing.md) — obligations that apply if/when the user does convey (distribute) modified code from this fork.
