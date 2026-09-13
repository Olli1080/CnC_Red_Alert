# CODE/ - the main game engine (see Agent/project_state/modules/summary.md).
#
# Sub-milestone 1b (Agent/plans_wip/modernization_cmake_vcpkg.md): get this compiling and linking
# against win32lib. The source list below is not a glob - it is the exact Win32 (ra95.exe) object
# list transcribed from Westwood's own CODE/MAKEFILE (the OBJECTS/LIBFILES/TECHFILES macros plus
# the "!ifdef WIN32" extras), which is authoritative for exactly which files were part of the real
# shipped build - this sidesteps CODE/'s .BAK files, IDE-only helper files, and other cruft without
# having to rediscover "what's live" by trial and error the way WIN32LIB's glob+exclude did.
#
# .ASM files referenced by that object list (2TXTPRNT, IPXPROT) are deliberately NOT included here -
# same policy as WIN32LIB/CMakeLists.txt: UASM wiring is deferred to Phase 2 and 32-bit-flat assembly
# can't assemble for this project's x64-only target anyway, so this target only proves the C++ side
# compiles. Any symbols those .ASM files provide will surface as unresolved externals only once
# something links a full executable against this library, not before. CPUID.ASM, LCWCOMP.ASM, and
# 2KEYFBUF.ASM are exceptions: sub-milestone 1c rewrote them as CPUID.CPP/LCWCOMP.CPP/2KEYFBUF.CPP
# (see each file's own header comment) rather than leaving them excluded, since redalert.exe needs
# real implementations of Detect_MMX_Availability/CPUType, LCW_Comp, and Buffer_Frame_To_Page to link.
# WINASM.ASM's still-live symbols (Stop_Execution, Set_Palette_Register, ModeX_Blit, the
# Asm_Interpolate* family) are similarly covered by WINASM.CPP.
#
# One object name from the makefile has no corresponding source file anywhere in this GPL
# release (checked the whole repo, not just CODE/):
#   - PASSEDIT.OBJ: a Westwood Online (WOL) password-edit dialog. WOL depends on Westwood's own
#     long-defunct multiplayer matchmaking servers, so this gap does not block getting the game
#     itself compiling; revisit only if WOL-specific files (WOL_*.CPP, WOLEDIT.CPP, RAWOLAPI.CPP)
#     are ever made to link.
#
# PALETTE.OBJ's source (PALETTE.CPP) is also missing from the drop, but unlike PASSEDIT this one
# blocked ~50 files: PaletteClass itself (used everywhere for palette fades - GamePalette,
# BlackPalette, CCPalette, etc.) had no surviving implementation anywhere, not even in the makefile
# gap sense - RGB.H only ever forward-declared it. PALETTE.CPP/PALETTE.H here are a reconstruction
# written from every call site's actual usage (see PALETTE.H's own header comment for detail), not
# recovered original source.
set(CODE_SOURCES
    2KEYFBUF.CPP
    2KEYFRAM.CPP
    AADATA.CPP
    ABSTRACT.CPP
    ADATA.CPP
    AIRCRAFT.CPP
    ANIM.CPP
    AUDIO.CPP
    B64PIPE.CPP
    B64STRAW.CPP
    BAR.CPP
    BASE.CPP
    BASE64.CPP
    BBDATA.CPP
    BDATA.CPP
    BENCH.CPP
    BFIOFILE.CPP
    BIGCHECK.CPP
    BLOWFISH.CPP
    BLOWPIPE.CPP
    BLWSTRAW.CPP
    BUFF.CPP
    BUILDING.CPP
    BULLET.CPP
    CARGO.CPP
    CARRY.CPP
    CCDDE.CPP
    CCFILE.CPP
    CCINI.CPP
    CCPTR.CPP
    CDATA.CPP
    CDFILE.CPP
    CELL.CPP
    CHECKBOX.CPP
    CHEKLIST.CPP
    COLRLIST.CPP
    COMBAT.CPP
    COMBUF.CPP
    COMINIT.CPP
    CONNECT.CPP
    CONQUER.CPP
    CONST.CPP
    CONTROL.CPP
    COORD.CPP
    CPUID.CPP
    CRATE.CPP
    CRC.CPP
    CRCPIPE.CPP
    CRCSTRAW.CPP
    CREDITS.CPP
    CREW.CPP
    DDE.CPP
    DEBUG.CPP
    DIAL8.CPP
    DIALOG.CPP
    DIBFILE.CPP
    DIBUTIL.CPP
    DISPLAY.CPP
    DOOR.CPP
    DRIVE.CPP
    DROP.CPP
    DYNAVEC.CPP
    EDIT.CPP
    EGOS.CPP
    ENDING.CPP
    EVENT.CPP
    EXPAND.CPP
    FACE.CPP
    FACING.CPP
    FACTORY.CPP
    FIELD.CPP
    FINDPATH.CPP
    FIXED.CPP
    FLASHER.CPP
    FLY.CPP
    FOOT.CPP
    FUSE.CPP
    GADGET.CPP
    GAMEDLG.CPP
    GAUGE.CPP
    GETCPU.CPP
    GLOBALS.CPP
    GOPTIONS.CPP
    GSCREEN.CPP
    HDATA.CPP
    HEAP.CPP
    HELP.CPP
    HOUSE.CPP
    HSV.CPP
    ICONLIST.CPP
    IDATA.CPP
    INFANTRY.CPP
    INI.CPP
    INIT.CPP
    INT.CPP
    INTERNET.CPP
    INTERPAL.CPP
    INTRO.CPP
    IOMAP.CPP
    IOOBJ.CPP
    IPX.CPP
    IPX95.CPP
    IPXADDR.CPP
    IPXCONN.CPP
    IPXGCONN.CPP
    IPXMGR.CPP
    JSHELL.CPP
    KEY.CPP
    LAYER.CPP
    LCW.CPP
    LCWCOMP.CPP
    LCWUNCMP.CPP
    LCWPIPE.CPP
    LCWSTRAW.CPP
    LINK.CPP
    LIST.CPP
    LOADDLG.CPP
    LOGIC.CPP
    LZO1X_C.CPP
    LZO1X_D.CPP
    LZOPIPE.CPP
    LZOSTRAW.CPP
    LZW.CPP
    LZWPIPE.CPP
    LZWSTRAW.CPP
    MAP.CPP
    MAPEDDLG.CPP
    MAPEDIT.CPP
    MAPEDPLC.CPP
    MAPEDTM.CPP
    MAPSEL.CPP
    MCI.CPP
    MCIMOVIE.CPP
    MENUS.CPP
    MISSION.CPP
    MIXFILE.CPP
    MONOC.CPP
    MOUSE.CPP
    MP.CPP
    MPGSET.CPP
    MPLAYER.CPP
    MPU.CPP
    MSGBOX.CPP
    MSGLIST.CPP
    NETDLG.CPP
    NULLCONN.CPP
    # NULLDLG.CPP excluded: the null-modem dialer's phone-book UI needs PhoneEntryClass, declared
    # in "phone.h" which genuinely does not exist anywhere in this GPL release (see FUNCTION.H's
    # own note on the same gap). Direct-dial modem multiplayer is dead functionality regardless of
    # toolchain, so this isn't worth reconstructing the way PaletteClass was.
    # NULLMGR.CPP excluded: NullModemClass's actual method bodies call directly into the Greenleaf
    # Communications Library (GCL) API (COM1-5, ASSUCCESS/ASUSERABORT, HMWaitForOK,
    # HMSetUpEchoRoutine, CD_SET, ...) in many places regardless of the WIN32/non-WIN32 split its
    # own header uses - unlike NULLCONN.H/NULLMGR.H, where only the non-WIN32 branch needed GCL,
    # this .cpp's Win32 code path was never fully ported off it. GCL is proprietary and genuinely
    # absent from this GPL release (see FUNCTION.H's modem.h/fast.h note) - not worth reconstructing
    # for a feature (null-modem/dial-up multiplayer) that's dead regardless.
    NULLSTUB.CPP
    OBJECT.CPP
    ODATA.CPP
    OPTIONS.CPP
    OVERLAY.CPP
    PACKET.CPP
    PALETTE.CPP
    PIPE.CPP
    PK.CPP
    PKPIPE.CPP
    PKSTRAW.CPP
    POWER.CPP
    PROFILE.CPP
    QUEUE.CPP
    RADAR.CPP
    RADIO.CPP
    RAMFILE.CPP
    RANDOM.CPP
    RAWFILE.CPP
    RAWOLAPI.CPP
    READLINE.CPP
    RECT.CPP
    REINF.CPP
    RGB.CPP
    RNDSTRAW.CPP
    ROTBMP.CPP
    RULES.CPP
    SAVELOAD.CPP
    SCENARIO.CPP
    SCORE.CPP
    SCROLL.CPP
    SDATA.CPP
    SEDITDLG.CPP
    SENDFILE.CPP
    SESSION.CPP
    SHA.CPP
    SHAPEBTN.CPP
    SHAPIPE.CPP
    SHASTRAW.CPP
    SIDEBAR.CPP
    SLIDER.CPP
    SMUDGE.CPP
    SOUNDDLG.CPP
    SPECIAL.CPP
    SPRITE.CPP
    STARTUP.CPP
    STATBTN.CPP
    STATS.CPP
    STRAW.CPP
    SUPER.CPP
    TAB.CPP
    TACTION.CPP
    TARGET.CPP
    TCPIP.CPP
    TDATA.CPP
    TEAM.CPP
    TEAMTYPE.CPP
    TECHNO.CPP
    TEMPLATE.CPP
    TERRAIN.CPP
    TEVENT.CPP
    TEXTBTN.CPP
    THEME.CPP
    TOGGLE.CPP
    TOOLTIP.CPP
    TRACKER.CPP
    TRIGGER.CPP
    TRIGTYPE.CPP
    TXTLABEL.CPP
    UDATA.CPP
    UDPADDR.CPP
    UNIT.CPP
    UTRACKER.CPP
    VDATA.CPP
    VECTOR.CPP
    VERSION.CPP
    VESSEL.CPP
    VISUDLG.CPP
    VORTEX.CPP
    VQASTUB.CPP
    W95TRACE.CPP
    WARHEAD.CPP
    WEAPON.CPP
    WINASM.CPP
    WINSTUB.CPP
    WOLAPIOB.CPP
    WOLEDIT.CPP
    WOLSTRNG.CPP
    WOL_CGAM.CPP
    WOL_CHAT.CPP
    WOL_DNLD.CPP
    WOL_GSUP.CPP
    # WOL_LOGN.CPP excluded: the Westwood Online login dialog needs PassEditClass, declared in
    # "PassEdit.h", which genuinely does not exist anywhere in this GPL release. WOL depends on
    # Westwood's own long-defunct multiplayer servers regardless, matching the PASSEDIT.OBJ gap
    # already noted above.
    WOL_MAIN.CPP
    WOL_OPT.CPP
    WRITEPCX.CPP
    WSPIPX.CPP
    WSPROTO.CPP
    WSPUDP.CPP
    XPIPE.CPP
    XSTRAW.CPP
    _WSPROTO.CPP
)
list(TRANSFORM CODE_SOURCES PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")

add_library(redalert_code STATIC ${CODE_SOURCES})

# WIN32LIB/INCLUDE must come before CODE/ itself in this list: seven filenames exist in both
# directories (AUDIO.H, DEFINES.H, EXTERNS.H, FILEPCX.H, KEYBOARD.H, MOUSE.H, RAWFILE.H), and
# several CODE/ headers deliberately angle-bracket-include the low-level WIN32LIB version (e.g.
# JSHELL.H's "#include <mouse.h>" wants WIN32LIB's Get_Mouse_X()/WWMouseClass, not CODE's own
# high-level MouseClass). Angle-bracket lookup walks -I paths in order and does not special-case
# "the including file's own directory" the way quote-includes do, so CODE/ listed first here was
# silently shadowing WIN32LIB/INCLUDE's copy for every angle-bracket include of these seven names -
# this matches the original Watcom MAKEFILE, whose CC_CFG listed the library include directory
# ($(WWFLAT)\INCLUDE) explicitly, ahead of relying on the current directory.
target_include_directories(redalert_code PRIVATE
    "${CMAKE_SOURCE_DIR}/WIN32LIB/INCLUDE"
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/WOLAPI"
    "${CMAKE_SOURCE_DIR}/WINVQ/INCLUDE"
)

# These match the original Watcom MAKEFILE's CC_CFG for the WIN32 build (/DWOLAPI_INTEGRATION,
# /DWINSOCK_IPX, /D$(LANGUAGE)=1 with LANGUAGE=ENGLISH by default) - we hadn't been passing them,
# which silently compiled out large chunks of code guarded behind them (e.g. all of TOOLTIP.H's
# real content lives behind "#ifdef WOLAPI_INTEGRATION", so without this define the file preprocesses
# to nothing and TOOLTIP.CPP's own method definitions have no class to belong to).
target_compile_definitions(redalert_code PRIVATE
    WOLAPI_INTEGRATION
    WINSOCK_IPX
    ENGLISH=1
)

target_link_libraries(redalert_code PUBLIC win32lib)

# Sub-milestone 1c (Agent/plans_wip/modernization_cmake_vcpkg.md): the actual executable. WinMain
# lives in STARTUP.CPP, already part of redalert_code, so this target is just redalert_code plus
# the system import libraries the original MAKEFILE would have linked implicitly - none of these
# are declared via #pragma comment(lib, ...) anywhere in this codebase, so they must be listed
# explicitly here (grepped for DirectDraw/DirectSound/winmm/Winsock/registry/window API usage):
#   ddraw   - WIN32LIB/MISC/DDRAW.CPP (IDirectDraw/IDirectDraw2)
#   dsound  - WIN32LIB/AUDIO/SOUNDIO.CPP etc. (IDirectSound) - kept as-is for 1c, see port log
#   winmm   - timeSetEvent/timeBeginPeriod (audio maintenance timer), mciSendCommand (MCI.CPP)
#   ws2_32  - TCPIP.CPP/WSPROTO.CPP/WSPUDP.CPP/WSPIPX.CPP (Winsock, the live multiplayer transport)
#   user32/gdi32/advapi32/shell32 - CreateWindow/MessageBox, GDI, registry (RegOpenKeyEx et al.),
#     shell32 (SHGetFolderPath-style calls) - not auto-linked since <windows.h> isn't pulled in via
#     the usual #pragma comment(lib,...) idiom anywhere in this tree.
# CC_ICON.RC is NOT included: it references "redalert.ico", which does not exist anywhere in this
# GPL release (checked the whole repo) - cosmetic only, not worth blocking the link over.
#
# d3d11/d3dcompiler (Agent/plans_done/code_1c_port_log.md, "DirectDraw 8-bit palette wall"
# follow-up): modern DirectDraw refuses to create an 8-bit palette-indexed surface in any
# capacity (DDERR_INVALIDPIXELFORMAT, confirmed by actually running this build), not just an
# exclusive-fullscreen display mode - WIN32LIB/MISC/PRESENT_D3D.CPP presents the visible page via
# a small Direct3D 11 swapchain instead.
add_executable(redalert WIN32 EXE_STUB.CPP)
target_link_libraries(redalert PRIVATE
    redalert_code
    ddraw
    dsound
    winmm
    ws2_32
    user32
    gdi32
    advapi32
    shell32
    d3d11
    d3dcompiler
)
