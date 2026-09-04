#pragma once

#include <windows.h>

// The UI language. Ported from MinClock's loc.cpp, minus its date half — this
// app draws no dates, so it needs only the "which words" job.

// Reads the user's display language and loads lang\<locale>.ini over it. Call
// once, before any window or menu is built. Silent on failure: no folder, no
// matching file and a malformed file all leave the UI in English.
void LocInit();

// English in, translated out. Never returns null and never fails — an unknown
// key, a blank value or a language that was never loaded all give back `en`
// unchanged, which is what makes a half-translated file a working file.
//
// The English literal at the call site IS the key, so editing one silently
// drops its translations. Change the string here and in lang\translations.csv
// together, then re-run lang\build.ps1.
const WCHAR* T(const WCHAR* en);

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
