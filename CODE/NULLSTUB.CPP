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

// Dead-feature stubs for sub-milestone 1c (Agent/plans_done/code_1c_port_log.md), matching the
// policy already established for NULLDLG.CPP/WOL_LOGN.CPP/NULLMGR.CPP in sub-milestone 1b
// (CODE/CMakeLists.txt) - direct-dial modem, null-modem, and Westwood Online multiplayer are dead
// regardless of toolchain (WOL depends on Westwood's long-defunct matchmaking servers; null-modem/
// dial-up needs the proprietary, GPL-absent Greenleaf Communications Library).
//
// Select_Serial_Dialog/Com_Scenario_Dialog/Com_Show_Scenario_Dialog were themselves defined in the
// already-excluded NULLDLG.CPP - live callers elsewhere (INIT.CPP, MPLAYER.CPP, WOL_GSUP.CPP) still
// need *something* to link against. Every stub here returns the same "cancelled / unavailable /
// nothing happened" sentinel its caller already treats as the graceful non-multiplayer path -
// verified by reading each call site, not assumed.
//
// NullModemClass's own missing methods (declared in NULLMGR.H, bodied only in the excluded
// NULLMGR.CPP) get the same treatment: the class is instantiated as a real global (GLOBALS.CPP's
// `NullModem`, "declaring this class doesn't perform any allocations" per its own comment), so the
// constructor/destructor must be safe to run even though the feature never does anything.

#include "function.h"

bool Find_Local_Scenario(char *, char *, unsigned int, char *, bool)
{
	return false;
}

GameType Select_Serial_Dialog(void)
{
	return GAME_NORMAL;
}

int Com_Scenario_Dialog(bool)
{
	return 0;
}

int Com_Show_Scenario_Dialog(void)
{
	return 0;
}

void Modem_Signoff(void)
{
}

void Shutdown_Modem(void)
{
}

int Reconnect_Modem(void)
{
	return 0;
}

void Destroy_Null_Connection(int, int)
{
}

class WolapiObject;
int WOL_Login_Dialog(WolapiObject *)
{
	return 0;
}

NullModemClass::NullModemClass(int numsend, int numreceive, int maxlen, unsigned short magicnum) :
	BuildBuf(NULL), MaxLen(maxlen), EchoBuf(NULL), EchoSize(0), EchoCount(0),
	NumSend(numsend), NumReceive(numreceive), MagicNum(magicnum),
	RXBuf(NULL), RXSize(0), RXCount(0)
{
}

NullModemClass::~NullModemClass()
{
}

int NullModemClass::Num_Connections(void)
{
	return 0;
}

int NullModemClass::Init_Send_Queue(void)
{
	return 0;
}

int NullModemClass::Send_Message(void *, int, int)
{
	return 0;
}

int NullModemClass::Get_Message(void *, int *)
{
	return 0;
}

int NullModemClass::Service(void)
{
	return 0;
}

int NullModemClass::Num_Send(void)
{
	return 0;
}

unsigned long NullModemClass::Response_Time(void)
{
	return 0;
}

int NullModemClass::Num_Receive(void)
{
	return 0;
}

void NullModemClass::Reset_Response_Time(void)
{
}

void NullModemClass::Set_Timing(unsigned long, unsigned long, unsigned long)
{
}

void NullModemClass::Configure_Debug(int, int, int, char **, int, int)
{
}
