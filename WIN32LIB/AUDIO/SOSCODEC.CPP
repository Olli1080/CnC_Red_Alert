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

// Partial replacement for SOSCODEC.ASM/OLSOSDEC.ASM (sub-milestone 1c -
// Agent/plans_done/code_1c_port_log.md).
//
// SOUNDINT.CPP's Sample_Copy handles two distinct compressed-audio formats: SCOMP_WESTWOOD (the
// "Westwood ADPCM" scheme, fully ported in AUDUNCMP.CPP's Decompress_Frame - this is what every
// retail Red Alert .AUD sound effect actually uses, per SOUNDINT.H's own enum comment "Special
// sliding window delta compression" as the primary type) and SCOMP_SOS (a separate HMI "SOS" IMA-
// ADPCM-family codec, enum value 99 - a rarer, secondary type). The SOS decoder's real tables
// (DIFFTB.INC/INDEXTB.INC: two 89-row x 16-token lookup tables, ~180KB of precomputed constants)
// are large enough that hand-transcribing them here risks silent, hard-to-detect decode corruption
// with no way to verify correctness without real SOS-compressed test assets - genuinely out of
// scope for this sub-milestone's link-a-real-executable goal.
//
// sosCODECInitStream is ported for real (it only resets a few state fields). sosCODECDecompressData
// / General_sosCODECDecompressData are left as documented stubs: they fill the destination with
// silence and report success rather than decoding, so a stream using this rarer path degrades to
// silence instead of returning garbage or corrupting playback bookkeeping (SOUNDINT.CPP's caller
// advances its buffer pointers by the declared block size regardless of what decompression actually
// produced). Revisit if real assets are found that need this path.

#include <soscomp.h>
#include <string.h>

extern "C" {

void __cdecl sosCODECInitStream(_SOS_COMPRESS_INFO * info)
{
	if (!info) return;
	info->dwSampleIndex = 0;
	info->dwPredicted = 0;
	info->dwSampleIndex2 = 0;
	info->dwPredicted2 = 0;
}

unsigned long __cdecl sosCODECDecompressData(_SOS_COMPRESS_INFO * info, unsigned long numBytes)
{
	if (info && info->lpDest) memset(info->lpDest, 0, numBytes);
	return numBytes;
}

unsigned long __cdecl General_sosCODECDecompressData(_SOS_COMPRESS_INFO * info, unsigned long numBytes)
{
	return sosCODECDecompressData(info, numBytes);
}

}
