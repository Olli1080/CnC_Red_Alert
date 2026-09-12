---
name: licensing
description: GPLv3 + EA additional terms this repo is released under, and what that means for agent behavior
metadata:
  type: agent_policy
---

# Licensing

Source: [LICENSE.md](../../LICENSE.md).

Electronic Arts released this source under **GPL v3**, with EA-specific **additional terms under GPL §7** appended at the end of the license file. Both layers apply simultaneously.

## GPLv3 baseline (standard)
Standard copyleft: you can run/modify/convey the program; conveyed copies (modified or not) must remain under GPLv3, come with source, and carry the license notices. No sublicensing. No additional restrictions may be imposed beyond what's in the license (§10) — except the permitted "additional terms" below, which EA has exercised.

## EA's additional terms (the part specific to this repo)
1. **No trademark/publicity rights.** The license does **not** grant any right to the "Command & Conquer" name or any other EA trademark. Do not distribute a modification of this program *using* an EA trademark, and do not claim affiliation or association with EA or its employees.
2. **Notice preservation.** Any propagation/conveyance must include the copyright notice and these terms.
3. **Indemnification on assumed liability.** If you convey the program (or a modification) and separately assume contractual liability to your recipients for it, you must indemnify EA for any liability that assumption imposes on EA.
4. **No misrepresentation of origin.** You may not misrepresent where this program came from; modified versions must be marked as modified, not presented as the original.
5. **Disclaimer stacking.** EA's own "AS IS", no-warranty disclaimer is layered on top of the standard GPL disclaimer (both apply).

## Practical implications for agents working in this repo
- **Don't add EA/Command & Conquer branding to new artifacts** (e.g. don't title a build output, mod package, or splash screen in a way that implies it's an official EA product) unless the user is knowingly building something that stays unmodified/unconveyed, or has separate trademark permission.
- **If asked to fork/republish a modified copy publicly**, remind the user that modifications must be marked as such and the GPLv3 + EA terms must travel with it — this is a licensing fact worth surfacing, not a blocker to helping.
- **This applies to the code itself**, not to unrelated new work the user does in the same directory (e.g. an unrelated tool) — use judgment about what's "based on" the covered program.
- No action needed for purely local exploration, reading, or editing for the user's own use — the license's obligations trigger on *conveying* (distributing) the work, not on modifying it privately.

See also: [contribution_and_scope.md](contribution_and_scope.md) for how this interacts with the fact that this checkout is a personal fork.
