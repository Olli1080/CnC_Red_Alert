---
name: code-ai-scripting-scenario
description: CODE/ team/trigger scripted-AI systems, scenario/rules data loading, superweapons
metadata:
  type: project_state
---

# CODE/ — AI, Scripting & Scenario Data

Parent index: [summary.md](summary.md). See also [../../../knowledge_base/codebase_history_and_cruft.md](../../../knowledge_base/codebase_history_and_cruft.md) — there is **no single `SCRIPT.CPP`**; scripted behavior is split across two related-but-distinct systems, both described below.

## Team scripting (squad-level scripted missions)
- `TEAM.CPP`/`.H` — `TeamClass : AbstractClass`: a runtime squad of units executing a scripted team mission.
- `TEAMTYPE.CPP`/`.H` — `TeamTypeClass`: the scenario-authored template (composition + mission script) `TeamClass` instances spawn from; `TeamMissionType` enum (attack, move, guard, loop, deploy, etc).

## Trigger system (map-event-driven condition→action pairs)
- `TRIGGER.CPP`/`.H` — `TriggerClass`: runtime instance of a scenario trigger, holding two `TDEventClass` "has this event already sprung" records.
- `TRIGTYPE.CPP`/`.H` — `TriggerTypeClass`: the authored trigger definition (event + action pairing) triggers are instantiated from.
- `TACTION.CPP`/`.H` — `TActionType` enum: catalog of trigger actions (win, lose, begin production, create/destroy team, all-hunt, etc).
- `TEVENT.CPP`/`.H` — `TEventType`/`TDEventClass`: catalog of trigger-firing conditions (time elapsed, house discovered, all destroyed, etc).

## Scenario / rules data
- `SCENARIO.CPP`/`.H` — `ScenarioClass`: current-game metadata (scenario number, briefing, difficulty, global scenario variables) — the container loaded from the `.INI` scenario file (see [file_io_compression_saveload.md](file_io_compression_saveload.md) for the INI parser itself).
- `RULES.CPP`/`.H` — `RulesClass`/`DifficultyClass`: parses `RULES.INI`-style gameplay balance data (fixed-point firepower/speed/armor/cost/build-speed biases per difficulty). Full `.INI` schema not yet documented — see [../../../Analysis/summary.md](../../../Analysis/summary.md).
- `SPECIAL.CPP`/`.H` — special/cheat rule toggles.
- `REINF.CPP` — scripted reinforcement delivery logic.
- `SCORE.CPP`/`.H` — end-of-mission scoring/statistics screen.

## Superweapons / special abilities
- `SUPER.CPP`/`.H` — `SuperClass`: generic superweapon charge-up/ready/fire state machine (Chronosphere, Nuke, Iron Curtain, etc).
- `VORTEX.CPP`/`.H` — the Chronosphere/vortex warp-teleport visual/logic effect.

## Per-frame update dispatch
- `LOGIC.CPP`/`.H` — `LogicClass`: the per-frame master update-tick dispatcher driving all active objects each logic frame (ties into the object hierarchy in [object_hierarchy.md](object_hierarchy.md) and the event queue in [networking_multiplayer.md](networking_multiplayer.md)).
