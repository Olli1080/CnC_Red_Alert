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

// C++ replacement for SOSCODEC.ASM / OLSOSDEC.ASM (HMI "SOS" 4:1 IMA-ADPCM decoder).
//
// SOUNDINT.CPP's Sample_Copy hands every SCOMP_SOS block (enum value 99) to these routines. This is
// NOT a rare secondary format: all Red Alert music (the .AUD files in SCORES.MIX) uses it, and the
// earlier port left the decoders as stubs that wrote silence, so the game had no music at all.
//
// The algorithm is the standard IMA/DVI ADPCM and is ported from OLSOSDEC.ASM's
// General_sosCODECDecompressData (the straightforward reference version; SOSCODEC.ASM's
// sosCODECDecompressData is a hand-pipelined 16-bit-mono-only copy of the same thing):
//   - one 4-bit code per sample, low nibble first, two codes per source byte;
//   - diff = step/8 [+ step/4 if code&1] [+ step/2 if code&2] [+ step if code&4], negated if code&8;
//   - predicted += diff, clamped to 16 bit; index += IndexTab[code&7], clamped to 0..88;
//   - output is a 16-bit little-endian sample, or ((predicted >> 8) ^ 0x80) for 8-bit streams;
//   - stereo streams are byte-interleaved (L byte, R byte, ...), each byte holding two consecutive
//     samples of ONE channel, and the output is interleaved L,R.
// Decoder state (predicted value and step index, per channel) lives in the _SOS_COMPRESS_INFO and
// carries from one call to the next; the nibble phase restarts with every call, so a call must be
// given an even number of samples (Sample_Copy passes whole frames).
//
// The step table below was extracted from wCODECStepTab in OLSOSDEC.ASM, not typed in by hand.

#include <soscomp.h>
#include <string.h>

namespace {

unsigned short const StepTab[89] = {
	7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31,
	34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143,
	157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658,
	724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024,
	3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
	15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

short const IndexTab[8] = {-1, -1, -1, -1, 2, 4, 6, 8};

// One decoder channel: points at the predicted/index/step fields of the info structure.
struct Channel {
	long *	Predicted;
	short *	Index;
	short *	Step;
};

// Decode one 4-bit code, update the channel state and return the new predicted sample (-32768..32767).
inline long Decode_Code(Channel const & ch, unsigned code)
{
	unsigned step = StepTab[*ch.Index];

	long diff = step >> 3;
	if (code & 4) diff += step;
	if (code & 2) diff += step >> 1;
	if (code & 1) diff += step >> 2;
	if (code & 8) diff = -diff;

	long predicted = *ch.Predicted + diff;
	if (predicted > 0x7FFF) predicted = 0x7FFF;
	if (predicted < -0x8000) predicted = -0x8000;
	*ch.Predicted = predicted;

	int index = *ch.Index + IndexTab[code & 7];
	if (index < 0) index = 0;
	if (index > 88) index = 88;
	*ch.Index = (short)index;
	*ch.Step = (short)StepTab[index];

	return predicted;
}

// Decode `samples` samples of one channel. `src_stride` is the distance in source bytes between two
// token bytes (1 mono, 2 stereo), `dst_stride` the distance in destination bytes between two samples
// of this channel.
void Decode_Channel(Channel const & ch, unsigned char const * src, int src_stride,
		unsigned char * dst, int dst_stride, bool sixteen, unsigned long samples)
{
	unsigned token = 0;
	for (unsigned long i = 0; i < samples; i++) {
		unsigned code;
		if ((i & 1) == 0) {
			token = *src;
			src += src_stride;
			code = token & 0x0F;
		} else {
			code = (token >> 4) & 0x0F;
		}

		long predicted = Decode_Code(ch, code);

		if (sixteen) {
			dst[0] = (unsigned char)(predicted & 0xFF);
			dst[1] = (unsigned char)((predicted >> 8) & 0xFF);
		} else {
			dst[0] = (unsigned char)(((predicted >> 8) & 0xFF) ^ 0x80);
		}
		dst += dst_stride;
	}
}

}	// namespace

extern "C" {

void __cdecl sosCODECInitStream(_SOS_COMPRESS_INFO * info)
{
	if (!info) return;
	info->wIndex = 0;			// starting index 0
	info->wStep = 7;			// start with a step of 7
	info->dwPredicted = 0;		// no predicted value
	info->dwSampleIndex = 0;
	info->wIndex2 = 0;
	info->wStep2 = 7;
	info->dwPredicted2 = 0;
	info->dwSampleIndex2 = 0;
}

unsigned long __cdecl General_sosCODECDecompressData(_SOS_COMPRESS_INFO * info, unsigned long numBytes)
{
	if (!info || !info->lpSource || !info->lpDest) return 0;

	bool const sixteen = (info->wBitSize == 16);
	bool const stereo = (info->wChannels == 2);

	unsigned long const bytes_per_sample = sixteen ? 2 : 1;
	unsigned long const samples = numBytes / (bytes_per_sample * (stereo ? 2 : 1));	// per channel

	unsigned char const * src = (unsigned char const *)info->lpSource;
	unsigned char * dst = (unsigned char *)info->lpDest;

	Channel left = {&info->dwPredicted, &info->wIndex, &info->wStep};
	info->dwSampleIndex = 0;

	if (!stereo) {
		Decode_Channel(left, src, 1, dst, (int)bytes_per_sample, sixteen, samples);
	} else {
		Channel right = {&info->dwPredicted2, &info->wIndex2, &info->wStep2};
		info->dwSampleIndex2 = 0;
		int const dst_stride = (int)(bytes_per_sample * 2);
		Decode_Channel(left, src, 2, dst, dst_stride, sixteen, samples);
		Decode_Channel(right, src + 1, 2, dst + bytes_per_sample, dst_stride, sixteen, samples);
	}

	return numBytes;
}

unsigned long __cdecl sosCODECDecompressData(_SOS_COMPRESS_INFO * info, unsigned long numBytes)
{
	return General_sosCODECDecompressData(info, numBytes);
}

}
