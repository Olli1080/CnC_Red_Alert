/*
**	Command & Conquer Red Alert(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// Movie-playback exclusion stubs for sub-milestone 1c (Agent/plans_done/code_1c_port_log.md), per
// the plan's approved decision to exclude movie playback from this sub-milestone rather than wire
// in the real VQA player: WINVQ/VQA32/ already contains a full, real C++ implementation of these
// functions (TASK.CPP, LOADER.CPP, CONFIG.CPP), but it isn't part of any CMake target yet and pulls
// in its own dependencies (including the same never-shipped SOS audio driver already excluded from
// the main audio path) - wiring that module in is left to a future sub-milestone, matching the
// NULLDLG.CPP-style documented-exclusion precedent from sub-milestone 1b.
//
// VQA_Alloc() returning NULL is sufficient: every call site (CONQUER.CPP's Play_Movie, INTRO.CPP's
// Open_Movie) already treats a NULL handle as "movie unavailable" and skips playback gracefully -
// this was verified by reading both call sites, not assumed.
//
// VQA_PauseAudio/VQA_ResumeAudio (called from WINSTUB.CPP's Check_For_Focus_Loss and CONQUER.CPP)
// have no declaring header - both call sites forward-declare them locally - so they're defined here
// with plain C++ linkage to match those ad hoc declarations.

#include <vqa32/vqaplay.h>
#include <string.h>

VQAHandle * VQA_Alloc(void)
{
	return 0;
}

void VQA_Free(VQAHandle *)
{
}

void VQA_Init(VQAHandle *, long (*)(VQAHandle *, long, void *, long))
{
}

long VQA_Open(VQAHandle *, char const *, VQAConfig *)
{
	return -1;
}

void VQA_Close(VQAHandle *)
{
}

long VQA_Play(VQAHandle *, long)
{
	return 0;
}

void VQA_DefaultConfig(VQAConfig * config)
{
	if (config) memset(config, 0, sizeof(VQAConfig));
}

void VQA_PauseAudio(void)
{
}

void VQA_ResumeAudio(void)
{
}
