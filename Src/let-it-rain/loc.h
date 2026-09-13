#pragma once

#include <windows.h>

#include "Localization/strings.h"

// The UI language. Ported from MinClock's loc.cpp, minus its date half — this
// app draws no dates, so it needs only the "which words" job.

// Reads the user's display language and picks the matching STRINGTABLE built
// into the exe from Localization\strings.rc. Call once, before any window or
// menu is built. Silent on failure: a display language with no block, or none
// at all, leaves the UI in English.
void LocInit();

// The string for an IDS_ id from Localization\strings.h, in the chosen
// language. Never returns null. The pointer is into the exe's own resources,
// null-terminated (strings.rc is compiled with /n) and valid for the life of
// the process.
const WCHAR* T(UINT id);

// True when the UI language reads right-to-left. Only the tray menu uses this,
// via TPM_LAYOUTRTL, which TrackPopupMenu applies as the popup is built.
//
// The two dialogs are deliberately NOT mirrored. WS_EX_LAYOUTRTL has to be on
// the window before its controls are created, and neither route to that is
// worth it here: setting the style from WM_INITDIALOG sets the bit and moves
// nothing (measured — every child keeps its left-to-right position), and
// SetProcessDefaultLayout around CreateDialogParam did not take either.
// Arabic reads correctly in an unmirrored dialog, which is the bar that
// matters; mirroring is polish this app can do without.
bool LocIsRTL();
