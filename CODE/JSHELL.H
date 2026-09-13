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

/* $Header: /CounterStrike/JSHELL.H 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : JSHELL.H                                                     *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : 03/13/95                                                     *
 *                                                                                             *
 *                  Last Update : March 13, 1995 [JLB]                                         *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef JSHELL_H
#define JSHELL_H


#include	<assert.h>
#include	"noinit.h"

// Get_Mouse_X/Get_Mouse_Y (used below) and WinTimerClass (used further down) come from WIN32LIB's
// own mouse.h/timer.h - this file must not depend on some other includer having pulled those in
// first.
#include	<mouse.h>
#include	<timer.h>

// ShapeFlags_Type (used below) is declared in WIN32LIB's shape.h. FUNCTION.H's own include order
// happens to reach this file (via CRATE.H) before it reaches shape.h elsewhere in its own list,
// so this file must not rely on that order and needs its own explicit include.
#include	<shape.h>

#ifdef WIN32
//#define getch	Get_Key_Num
//#define kbhit	Check_Key_Num
#include	"key.h"
#else
#include	<conio.h>
#endif

/*
**	Interface class to the keyboard. This insulates the game from library vagaries. Most
**	notable being the return values are declared as "int" in the library whereas C&C
**	expects it to be of KeyNumType.
*/
#ifdef WIN32
//#define KeyNumType	int
//#define KeyASCIIType	int

//lint -esym(1725,KeyboardClass::MouseQX,KeyboardClass::MouseQY)
struct KeyboardClass : public WWKeyboardClass

#else
struct KeyboardClass
#endif
{

	/*
	**	This flag is used to indicate whether the WW library has taken over
	**	the keyboard or not. If not, then the normal console input
	**	takes precedence.
	*/
	unsigned IsLibrary;

#ifndef WIN32
	int &MouseQX;
	int &MouseQY;

	KeyboardClass() :
		IsLibrary(true),
		MouseQX(::MouseQX),
		MouseQY(::MouseQY)
	{}
	KeyNumType Get(void) {return (IsLibrary ? (KeyNumType)Get_Key_Num() : (KeyNumType)getch());};
	KeyNumType Check(void) {return (IsLibrary ? (KeyNumType)Check_Key_Num() : (KeyNumType)kbhit());};
	KeyASCIIType To_ASCII(KeyNumType key) {return((KeyASCIIType)KN_To_KA(key));};
	void Clear(void) {if (IsLibrary) Clear_KeyBuffer();};
	int Down(KeyNumType key) {return(Key_Down(key));};
#else
	KeyboardClass() : IsLibrary(true) {}
	KeyNumType Get(void) {return ((KeyNumType)WWKeyboardClass::Get());};
	KeyNumType Check(void) {return ((KeyNumType)WWKeyboardClass::Check());};
	KeyASCIIType To_ASCII(KeyNumType key) {return((KeyASCIIType)WWKeyboardClass::To_ASCII(key));};
	void Clear(void) {WWKeyboardClass::Clear();};
	int Down(KeyNumType key) {return(WWKeyboardClass::Down(key));};
#endif

	int Mouse_X(void) {return(Get_Mouse_X());};
	int Mouse_Y(void) {return(Get_Mouse_Y());};
};


/*
**	These templates allow enumeration types to have simple bitwise
**	arithmatic performed. The operators must be instatiated for the
**	enumerated types desired.
*/
template<class T> inline T operator ++(T & a)
{
	a = (T)((int)a + (int)1);
	return(a);
}
template<class T> inline T operator ++(T & a, int)
{
	T aa = a;
	a = (T)((int)a + (int)1);
	return(aa);
}
template<class T> inline T operator --(T & a)
{
	a = (T)((int)a - (int)1);
	return(a);
}
template<class T> inline T operator --(T & a, int)
{
	T aa = a;
	a = (T)((int)a - (int)1);
	return(aa);
}
template<class T> inline T operator |(T t1, T t2)
{
	return((T)((int)t1 | (int)t2));
}
template<class T> inline T operator &(T t1, T t2)
{
	return((T)((int)t1 & (int)t2));
}
template<class T> inline T operator ~(T t1)
{
	return((T)(~(int)t1));
}

#ifndef WIN32
template<class T> inline T min(T value1, T value2)
{
	if (value1 < value2) {
		return(value1);
	}
	return(value2);
}
int min(int, int);
long min(long, long);

template<class T> inline T max(T value1, T value2)
{
	if (value1 > value2) {
		return(value1);
	}
	return(value2);
}
int max(int, int);
long max(long, long);
#endif

template<class T> inline void swap(T &value1, T &value2)
{
	T temp = value1;
	value1 = value2;
	value2 = temp;
}
// NOT touched: these two don't even match the template above (by-value int/long params and a
// non-void return, versus the template's by-reference T& params and void return) - they're inert,
// uncalled leftovers from the original source, not a case of the shadowing bug fixed elsewhere in
// this file. Left exactly as-is (still bodyless/unlinkable, but nothing calls them).
int swap(int, int);
long swap(long, long);

template<class T> inline
T Bound(T original, T minval, T maxval)
{
	if (original < minval) return(minval);
	if (original > maxval) return(maxval);
	return(original);
};
inline int Bound(signed int original, signed int minval, signed int maxval)
{
	if (original < minval) return minval;
	if (original > maxval) return maxval;
	return original;
}
inline unsigned Bound(unsigned original, unsigned minval, unsigned maxval)
{
	if (original < minval) return minval;
	if (original > maxval) return maxval;
	return original;
}
inline long Bound(long original, long minval, long maxval)
{
	if (original < minval) return minval;
	if (original > maxval) return maxval;
	return original;
}

template<class T>
T _rotl(T X, int n)
{
	return((T)(( ( ( X ) << n ) | ( ( X ) >> ( (sizeof(T)*8) - n ) ) )));
}


/*
**	This macro serves as a general way to determine the number of elements
**	within an array.
*/
#define	ARRAY_LENGTH(x)	int(sizeof(x)/sizeof(x[0]))
#define	ARRAY_SIZE(x)		int(sizeof(x)/sizeof(x[0]))


/*
**	The shape flags are likely to be "or"ed together and other such bitwise
**	manipulations. These force explicit instantiation of the operator templates above for this
**	type - originally written as plain (bodyless) declarations, which older compilers apparently
**	treated as instantiation requests against the matching template, but which strict modern
**	compilers instead treat as separate, never-defined non-template overloads that shadow the
**	template and never link (see Agent/plans_done/code_1c_port_log.md).
*/
template ShapeFlags_Type operator |(ShapeFlags_Type, ShapeFlags_Type);
template ShapeFlags_Type operator &(ShapeFlags_Type, ShapeFlags_Type);
template ShapeFlags_Type operator ~(ShapeFlags_Type);


/*
**	These were Watcom `#pragma aux` inline-assembly bodies (real-mode-era bit-array primitives on
**	32-bit words), which MSVC/clang silently ignore as an unrecognized pragma - leaving the
**	declarations bodyless and unresolved at link time. Rewritten here as plain portable C++ with
**	identical semantics (sub-milestone 1c - Agent/plans_done/code_1c_port_log.md).
*/
inline void Set_Bit(void * array, int bit, int value)
{
	unsigned & word = ((unsigned *)array)[bit >> 5];
	unsigned mask = 1u << (bit & 0x1F);
	if (value) {
		word |= mask;
	} else {
		word &= ~mask;
	}
}

inline int Get_Bit(void const * array, int bit)
{
	unsigned word = ((unsigned const *)array)[bit >> 5];
	return (word >> (bit & 0x1F)) & 1;
}

inline int First_True_Bit(void const * array)
{
	unsigned const * ptr = (unsigned const *)array;
	int base = 0;
	while (*ptr == 0) {
		ptr++;
		base += 32;
	}
	unsigned word = *ptr;
	int index = 0;
	while (!(word & 1)) {
		word >>= 1;
		index++;
	}
	return base + index;
}

inline int First_False_Bit(void const * array)
{
	unsigned const * ptr = (unsigned const *)array;
	int base = 0;
	while (~*ptr == 0) {
		ptr++;
		base += 32;
	}
	unsigned word = ~*ptr;
	int index = 0;
	while (!(word & 1)) {
		word >>= 1;
		index++;
	}
	return base + index;
}

#ifdef OBSOLETE
extern int Bound(int original, int min, int max);
#pragma aux Bound parm [eax] [ebx] [ecx] \
	modify [eax] \
	value [eax]		= 				\
	"cmp	ebx,ecx"					\
	"jl	okorder"					\
	"xchg	ebx,ecx"					\
	"okorder: cmp	eax,ebx"		\
	"jg	okmin"					\
	"mov	eax,ebx"					\
	"okmin: cmp	eax,ecx"			\
	"jl	okmax"					\
	"mov	eax,ecx"					\
	"okmax:"

extern unsigned Bound(unsigned original, unsigned min, unsigned max);
#pragma aux Bound parm [eax] [ebx] [ecx] \
	modify [eax] \
	value [eax]		= 				\
	"cmp	ebx,ecx"					\
	"jb	okorder"					\
	"xchg	ebx,ecx"					\
	"okorder: cmp	eax,ebx"		\
	"ja	okmin"					\
	"mov	eax,ebx"					\
	"okmin: cmp	eax,ecx"			\
	"jb	okmax"					\
	"mov	eax,ecx"					\
	"okmax:"
#endif


// Watcom `#pragma aux` inline assembly, silently ignored (bodyless) under MSVC/clang - rewritten
// as plain C++ with identical semantics, including the original's 32-bit-truncating intermediate
// values (sub-milestone 1c - Agent/plans_done/code_1c_port_log.md).
inline unsigned Fixed_To_Cardinal(unsigned base, unsigned fixed)
{
	unsigned product = base * fixed + 0x80;
	if (product & 0xFF000000) product = 0xFFFFFF;
	return product >> 8;
}

inline unsigned Cardinal_To_Fixed(unsigned base, unsigned cardinal)
{
	if (base == 0) return cardinal;
	unsigned shifted = cardinal << 8;
	return shifted / base;
}


#ifndef OUTPORTB
#define OUTPORTB
// Raw x86 `out` port I/O (real-mode/DOS-era direct VGA register access) - meaningless on modern
// Windows regardless of toolchain, since usermode code cannot execute privileged I/O instructions
// at all (this would fault). Kept as inline no-ops rather than removing the call sites: the actual
// palette hardware programming already happens through IDirectDraw::CreatePalette/SetEntries (see
// WIN32LIB/MISC/DDRAW.CPP), so these calls are dead code on any Win32-era build already, not
// something newly broken by this modernization (sub-milestone 1c - Agent/plans_done/code_1c_port_log.md).
inline void outportb(int, unsigned char) {}
inline void outport(int, unsigned short) {}
#endif


/*
**	Timer objects that fetch the appropriate timer value according to
**	the type of timer they are.
*/
extern long Frame;
class FrameTimerClass
{
	public:
		long operator () (void) const {return(Frame);};
		operator long (void) const {return(Frame);};
};


#ifndef WIN32
extern bool TimerSystemOn;
extern "C" {
	long Get_System_Tick_Count(void);
	long Get_User_Tick_Count(void);
}
//bool Init_Timer_System(unsigned int freq, int partial=false);
bool Remove_Timer_System(void);
#else
extern WinTimerClass	*		WindowsTimer;
#endif

#ifndef SYSTEM_TIMER_CLASS
#define SYSTEM_TIMER_CLASS
class SystemTimerClass
{
	public:
		#ifdef WIN32
			long operator () (void) const {if (!WindowsTimer) return(0);return(WindowsTimer->Get_System_Tick_Count());};
			operator long (void) const {if (!WindowsTimer) return(0);return(WindowsTimer->Get_System_Tick_Count());};
		#else
			long operator () (void) const {return(Get_System_Tick_Count());};
			operator long (void) const {return(Get_System_Tick_Count());};
		#endif
};
#endif


class UserTimerClass
{
	public:
		#ifdef WIN32
			long operator () (void) const {if (!WindowsTimer) return(0);return(WindowsTimer->Get_User_Tick_Count());};
			operator long (void) const {if (!WindowsTimer) return(0);return(WindowsTimer->Get_User_Tick_Count());};
		#else
			long operator () (void) const {return(Get_User_Tick_Count());};
			operator long (void) const {return(Get_User_Tick_Count());};
		#endif
};


template<class T>
void Bubble_Sort(T * array, int count)
{
	if (array != NULL && count > 1) {
		bool swapflag;

		do {
			swapflag = false;
			for (int index = 0; index < count-1; index++) {
				if (array[index] > array[index+1]) {
					T temp = array[index];
					array[index] = array[index+1];
					array[index+1] = temp;
					swapflag = true;
				}
			}
		} while (swapflag);
	}
}

template<class T>
void PBubble_Sort(T * array, int count)
{
	if (array != NULL && count > 1) {
		bool swapflag;

		do {
			swapflag = false;
			for (int index = 0; index < count-1; index++) {
				if (*array[index] > *array[index+1]) {
					T temp = array[index];
					array[index] = array[index+1];
					array[index+1] = temp;
					swapflag = true;
				}
			}
		} while (swapflag);
	}
}

template<class T>
void PNBubble_Sort(T * array, int count)
{
	if (array != NULL && count > 1) {
		bool swapflag;

		do {
			swapflag = false;
			for (int index = 0; index < count-1; index++) {
				if (stricmp(array[index]->Name(), array[index+1]->Name()) > 0) {
					T temp = array[index];
					array[index] = array[index+1];
					array[index+1] = temp;
					swapflag = true;
				}
			}
		} while (swapflag);
	}
}

template<class T>
class SmartPtr
{
	public:
		SmartPtr(NoInitClass const &) {}
		SmartPtr(T * realptr = 0) : Pointer(realptr) {}
		SmartPtr(SmartPtr const & rvalue) : Pointer(rvalue.Pointer) {}
		~SmartPtr(void) {Pointer = 0;}

		operator T * (void) const {return(Pointer);}

		operator long (void) const {return((long)Pointer);}

		SmartPtr<T> operator ++ (int) {assert(Pointer != 0);SmartPtr<T> temp = *this;++Pointer;return(temp);}
		SmartPtr<T> & operator ++ (void) {assert(Pointer != 0);++Pointer;return(*this);}
		SmartPtr<T> operator -- (int) {assert(Pointer != 0);SmartPtr<T> temp = *this;--Pointer;return(temp);}
		SmartPtr<T> & operator -- (void) {assert(Pointer != 0);--Pointer;return(*this);}

		SmartPtr & operator = (SmartPtr const & rvalue) {Pointer = rvalue.Pointer;return(*this);}
		T * operator -> (void) const {assert(Pointer != 0);return(Pointer);}
		T & operator * (void) const {assert(Pointer != 0);return(*Pointer);}

	private:
		T * Pointer;
};


#endif
