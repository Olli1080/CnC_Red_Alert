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

// C++ replacement for CPUID.ASM (sub-milestone 1c - Agent/plans_done/code_1c_port_log.md), using
// MSVC's __cpuid intrinsic (<intrin.h>) rather than a hand-written CPUID wrapper - the exact
// substitution the plan called out as trivial. The original's 386/486 detection dance (toggling the
// AC/ID eflags bits to check whether CPUID is even available) is dropped: this project targets x64
// only, and CPUID/MMX have been present on every x64 CPU since the ISA's introduction, so that
// dance can never take the "older CPU" branch here - CPUType always ends up computed from a real
// CPUID leaf 1 call, and Detect_MMX_Availability always finds the MMX feature bit set.

#include <intrin.h>

extern "C" {

char CPUType = 0;
char VendorID[20] = "Not available";

int __cdecl Detect_MMX_Availability(void)
{
	int info[4];

	__cpuid(info, 0);
	*(int *)(VendorID + 0) = info[1];
	*(int *)(VendorID + 4) = info[3];
	*(int *)(VendorID + 8) = info[2];
	VendorID[12] = ' ';

	if (info[0] < 1) return 0;

	__cpuid(info, 1);
	CPUType = (char)((info[0] >> 8) & 0x0F);

	return (info[3] & 0x00800000) != 0;
}

}
