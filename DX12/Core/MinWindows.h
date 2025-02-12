#pragma once

#if defined(_WINDOWS_) && !defined(IB_MIN_WINDOWS)
#pragma message ( "Warning: You have included windows.h before MinWindows.h." )
#pragma message ( "Please use MinWindows.h instead!" )
#endif

#define IB_MIN_WINDOWS

// WIN32_LEAN_AND_MEAN excludes rarely-used services from windows headers.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#define CORE_DEFINES

// The below excludes some other unused services from the windows headers.
// Check windows.h for details.
#define NOGDICAPMASKS			// CC_*, LC_*, PC_*, CP_*, TC_*, RC_
#ifndef CORE_DEFINES
#define NOVIRTUALKEYCODES		// VK_*
#define NOWINMESSAGES			// WM_*, EM_*, LB_*, CB_*
#define NOWINSTYLES			// WS_*, CS_*, ES_*, LBS_*, SBS_*, CBS_*
#define NOSYSMETRICS			// SM_*
#endif
#define NOMENUS				// MF_*
#define NOICONS				// IDI_*
#ifndef CORE_DEFINES
#define NOKEYSTATES			// MK_*
#endif
#define NOSYSCOMMANDS			// SC_*
#define NORASTEROPS			// Binary and Tertiary raster ops
#ifndef CORE_DEFINES
#define NOSHOWWINDOW			// SW_*
#endif
#define OEMRESOURCE				// OEM Resource values
#define NOATOM					// Atom Manager routines
#ifndef CORE_DEFINES
#define NOCLIPBOARD			// Clipboard routines
#define NOCOLOR				// Screen colors
#endif
#define NOCTLMGR				// Control and Dialog routines
#define NODRAWTEXT				// DrawText() and DT_*
#define NOGDI					// All GDI #defines and routines
#define NOKERNEL				// All KERNEL #defines and routines
#ifndef CORE_DEFINES
#define NOUSER				// All USER #defines and routines
#define NONLS					// All NLS #defines and routines
#define NOMB					// MB_* and MessageBox()
#endif
#define NOMEMMGR				// GMEM_*, LMEM_*, GHND, LHND, associated routines
#define NOMETAFILE				// typedef METAFILEPICT
#ifndef NOMINMAX                 // Macros min(a,b) and max(a,b)
#define NOMINMAX
#endif
#ifndef CORE_DEFINES
#define NOMSG					// typedef MSG and associated routines
#endif
#define NOOPENFILE				// OpenFile(), OemToAnsi, AnsiToOem, and OF_*
#define NOSCROLL				// SB_* and scrolling routines
#define NOSERVICE				// All Service Controller routines, SERVICE_ equates, etc.
#define NOSOUND					// Sound driver routines
#define NOTEXTMETRIC			// typedef TEXTMETRIC and associated routines
#define NOWH					// SetWindowsHook and WH_*
#ifndef CORE_DEFINES
#define NOWINOFFSETS			// GWL_*, GCL_*, associated routines
#endif
#define NOCOMM					// COMM driver routines
#define NOKANJI					// Kanji support stuff.
#define NOHELP					// Help engine interface.
#define NOPROFILER				// Profiler interface.
#define NODEFERWINDOWPOS		// DeferWindowPos routines
#define NOMCX					// Modem Configuration Extensions
#define NOCRYPT
#define NOTAPE
#define NOIMAGE
#define NOPROXYSTUB
#define NORPC
#define NOMINMAX

#include <Windows.h>