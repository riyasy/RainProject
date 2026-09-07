// version.h — the version and the binary's identity, in one place.
//
// Included by let-it-rain.rc: the VERSIONINFO block that gives the exe its
// identity, and the "v X.X.X.X" label at the bottom of the settings dialog.
// A native binary shipping with no company, no product and no version is one
// of the strongest signals an antivirus heuristic has to work with, so all
// three must stay filled in — and the properties sheet and the dialog must
// agree, which is what this file is for.
//
// RC only understands #define, so this file must stay free of anything else —
// no #pragma once, no types, no C++.
//
// One place this cannot reach: LetItRainInstallerMSIX\Package.appxmanifest,
// which is XML and has to be bumped by hand. See Notes.txt.

#define VER_MAJOR       2
#define VER_MINOR       2
#define VER_PATCH       3
#define VER_BUILD       0

#define VER_NUMBER      2,2,3,0             // VERSIONINFO wants commas
#define VER_STRING      "2.2.3.0"           // and a matching string
#define VER_DISPLAY     "v 2.2.3.0"         // what a person reads, in the dialog

#define VER_COMPANY     "RYF Tools"
#define VER_PRODUCT     "Let It Rain FX"
#define VER_DESCRIPTION "Rain and Snow for Windows desktop"
// The exe's properties sheet, and the English fallback for the About dialog's
// copyright line in let-it-rain.rc. The dialog itself builds the same sentence
// at runtime from VER_COMPANY plus a translated "All rights reserved." — the
// holder must not go through a translator, and this whole string as one key
// would make nineteen of them retype it. VERSIONINFO stays US English on
// purpose, so this stays as it is.
#define VER_COPYRIGHT   "\xA9 RYF Tools. All rights reserved."
#define VER_FILENAME    "let-it-rain.exe"
