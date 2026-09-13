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

// C++ replacement for VMPAGEIN.ASM (sub-milestone 1c - Agent/plans_done/code_1c_port_log.md). The
// original touched one byte of every 4096-byte page in the range with a no-op read-modify-write to
// force Win95's virtual memory manager to page each one in before a time-critical section runs.
// Still a valid (if now largely unnecessary, given modern working-set management) technique -
// ported as a literal per-page touch rather than dropped, since it's cheap and harmless.

extern "C" void __cdecl Force_VM_Page_In(void * buffer, int length)
{
	if (length <= 0) return;

	volatile unsigned char * page = (unsigned char *)buffer;
	int remaining = length;
	while (remaining > 0) {
		*page += 0;
		page += 4096;
		remaining -= 4096;
	}
}
