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

// C++ replacement for the still-needed parts of APROFILE.ASM (sub-milestone 1c -
// Agent/plans_done/code_1c_port_log.md). This is Westwood's own dev-only CPU sampling profiler
// (WPROFILE.CPP's Start_Profiler/Stop_Profiler already run under MSVC/x64; only these three pieces
// were missing), not anything gameplay-relevant.
//
// Dropped: Profile_Init/Profile_End's real job was pointing the Watcom-specific __PRO/__EPI
// function-entry/exit compiler hooks (Pro_Start/Epi_Start/Pro_End/Epi_End, ___begtext) at
// profiling-enabled versions of themselves - a Watcom code-instrumentation feature MSVC doesn't
// have an equivalent for. What's kept is the portable half: resetting the sample buffer index.
//
// New_Profiler_Callback_ (the ASM's own name, no "C" qualifier, a different linker symbol from what
// WPROFILE.CPP's Thread_Callback actually calls - the same dead-trailing-underscore-label pattern
// already seen elsewhere in this port) is not what's needed; New_Profiler_Callback (no underscore)
// never had an ASM body at all. Its real logic - record the current sampled address if there's room
// in the buffer, else stop the profiler - is ported directly here instead, using WPROFILE.CPP's own
// ProfileList/ProfilePtr and PROFILE_RATE/MAX_PROFILE_TIME constants (profile.h).

#include "profile.h"

extern "C" {

unsigned ProfileFunctionAddress = 0;

extern unsigned ProfileList[PROFILE_RATE * MAX_PROFILE_TIME];
extern unsigned ProfilePtr;
void __cdecl Stop_Profiler(void);

void __cdecl Profile_Init(void)
{
	ProfilePtr = 1;
}

void __cdecl Profile_End(void)
{
}

void __cdecl New_Profiler_Callback(void)
{
	if (ProfilePtr >= (unsigned)(MAX_PROFILE_TIME * PROFILE_RATE)) {
		Stop_Profiler();
		return;
	}
	ProfileList[ProfilePtr] = ProfileFunctionAddress;
	ProfilePtr++;
}

}
