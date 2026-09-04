// loc.cpp — which words the UI uses. Ported from MinClock, which carries the
// same file with a second half for dates that this app has no use for.
//
// The English string is the key. No numeric ids, no entry in Resource.h,
// nothing to keep in sync, and no way for an id and a string to drift apart; a
// missing key answers itself, so a half-translated file is a working file and
// English is the fallback for free. The cost is that the English literal at
// each call site is now an identifier — editing one drops its translations.
//
// Storage is lang\<locale>.ini, read with GetPrivateProfileSectionW: one call
// returns the whole [Strings] section as a double-null-terminated block of
// key=value, which is a parser we then do not write. It is the same profile API
// SettingsManager already uses for let-it-rain.ini, so this adds no dependency.
//
// **The files must be UTF-16LE with a BOM.** The profile APIs decide the
// encoding from the BOM alone; without one they read the file in the system
// codepage and every Malayalam, CJK and Arabic string arrives as mojibake —
// silently, with the source looking perfectly correct. lang\build.ps1 generates
// the files and then verifies exactly this through the same API. Nothing here
// ever writes to them, so the API cannot rewrite one as ANSI behind us.
//
// The UI language and the region are different Windows settings and this file
// reads the UI one:
//
//   GetUserPreferredUILanguages  -> Settings > Display language   (what we want)
//   LOCALE_NAME_USER_DEFAULT     -> Settings > Regional format    (not this)
//
// They disagree by default on more machines than you would guess — English
// Windows with a German region is a normal setup, and it wants English menus.

#include "loc.h"

#include <strsafe.h>

// One block holds every string, keys included. The UI is 21 short strings and
// the largest file is under 1 KB, so 8192 WCHARs is roughly eight times what is
// needed. Worth knowing if that ever stops being true: GetPrivateProfileSectionW
// does not fail on overflow, it truncates and returns cch-2, which would show up
// as the last few strings quietly reverting to English.
static WCHAR s_strings[8192];
static bool  s_rtl;

// ------------------------------------------------------------------
// Loading
// ------------------------------------------------------------------

// Look for `name` next to the exe, then walking up a few levels — which covers
// running out of let-it-rain\x64\Release with the folder in let-it-rain\.
//
// StringCchPrintfW rather than wsprintfW: this joins a MAX_PATH directory and a
// name into a MAX_PATH buffer, which is the one case wsprintfW's missing size
// argument gets wrong.
static bool FindNearExe(const WCHAR* name, WCHAR* out, size_t cch)
{
	WCHAR dir[MAX_PATH];
	if (!GetModuleFileNameW(nullptr, dir, MAX_PATH)) return false;
	for (int up = 0; up < 4; up++)
	{
		WCHAR* slash = wcsrchr(dir, L'\\');
		if (!slash) break;
		*slash = 0;                                  // strip file / last dir
		WCHAR probe[MAX_PATH];
		if (FAILED(StringCchPrintfW(probe, _countof(probe), L"%s\\%s", dir, name)))
			continue;
		const DWORD a = GetFileAttributesW(probe);
		if (a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY))
			return SUCCEEDED(StringCchCopyW(out, cch, probe));
	}
	return false;
}

// Try one lang\<name>.ini. False if it isn't there or holds no [Strings].
static bool LoadLang(const WCHAR* dir, const WCHAR* name)
{
	WCHAR path[MAX_PATH];
	if (FAILED(StringCchPrintfW(path, _countof(path), L"%s\\%s.ini", dir, name)))
		return false;
	if (GetFileAttributesW(path) == INVALID_FILE_ATTRIBUTES) return false;
	// Returns the number of characters copied, not counting the final null.
	// Zero means no such section, or an empty one — either way, nothing to use.
	const DWORD n = GetPrivateProfileSectionW(L"Strings", s_strings, _countof(s_strings), path);
	if (n == 0) { s_strings[0] = 0; return false; }
	return true;
}

// Last resort for one language: any lang\<prefix>-*.ini at all. This is what
// serves the regional variants nobody ships a file for — es-MX, es-AR and es-CO
// all land on es-ES.ini, de-AT and de-CH on de-DE.ini. Without it every one of
// those users would get English while a translation of their own language sat
// unread in the folder, and Spanish alone has twenty variants of which we ship
// exactly one.
//
// It resolves by directory order, which picks the *language* right and can pick
// the *flavour* wrong: zh-HK takes zh-CN.ini — Simplified, where Hong Kong
// writes Traditional — because zh-CN sorts ahead of zh-TW, and pt-AO takes
// pt-BR.ini for the same reason. The fix is one file and no code: the exact
// name is tried before this is reached at all, so dropping in a zh-HK.ini
// overrides it.
static bool LoadLangByPrefix(const WCHAR* dir, const WCHAR* prefix)
{
	WCHAR pat[MAX_PATH];
	if (FAILED(StringCchPrintfW(pat, _countof(pat), L"%s\\%s-*.ini", dir, prefix)))
		return false;
	WIN32_FIND_DATAW fd;
	const HANDLE h = FindFirstFileW(pat, &fd);
	if (h == INVALID_HANDLE_VALUE) return false;
	bool ok = false;
	do {
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
		WCHAR* dot = wcsrchr(fd.cFileName, L'.');
		if (dot) *dot = 0;                           // LoadLang appends .ini itself
		ok = LoadLang(dir, fd.cFileName);
	} while (!ok && FindNextFileW(h, &fd));
	FindClose(h);
	return ok;
}

void LocInit()
{
	// The display language chain, most preferred first. The chain matters as
	// much as the name: Windows answers e.g. "de-AT" -> "de" -> "de-DE", and
	// following it is how a language pack we have no exact file for still
	// resolves to one we do. 512 WCHARs is dozens of languages; a chain that
	// overflows it leaves us in English, the same answer as no folder at all.
	WCHAR langs[512] = { 0 };
	ULONG count = 0, cch = _countof(langs);
	if (!GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &count, langs, &cch))
		langs[0] = 0;

	// Mirroring follows the language the UI is *written in*, so it reads from
	// the same chain — an Arabic display language on a US region still wants
	// mirrored windows, with or without an ar-SA.ini behind them. A null name is
	// LOCALE_NAME_USER_DEFAULT, the documented fallback for an empty chain.
	DWORD layout = 0;
	if (GetLocaleInfoEx(langs[0] ? langs : nullptr,
	                    LOCALE_IREADINGLAYOUT | LOCALE_RETURN_NUMBER,
	                    reinterpret_cast<WCHAR*>(&layout), sizeof(layout) / sizeof(WCHAR)))
		s_rtl = (layout == 1);                       // 1 = RTL, reading LTR lines

	WCHAR dir[MAX_PATH];
	if (!FindNearExe(L"lang", dir, _countof(dir))) return;   // English, then

	// Every tier for one language before moving to the next, which is the whole
	// point of a preference order: a de-AT primary must reach de-DE.ini before
	// an en-US secondary is even considered.
	//
	// Full name first, and that matters rather than being tidy: pt-PT/pt-BR and
	// zh-CN/zh-TW are different files, and going straight to the "pt" or "zh"
	// prefix would hand half of those users the other half's translation.
	for (const WCHAR* p = langs; *p; p += lstrlenW(p) + 1)
	{
		WCHAR name[LOCALE_NAME_MAX_LENGTH];
		if (FAILED(StringCchCopyW(name, _countof(name), p))) continue;
		if (LoadLang(dir, name)) return;                     // de-DE.ini
		WCHAR* dash = wcschr(name, L'-');
		if (!dash) continue;
		*dash = 0;
		if (LoadLang(dir, name)) return;                     // de.ini
		if (LoadLangByPrefix(dir, name)) return;             // de-AT -> de-DE.ini
	}
}

// ------------------------------------------------------------------
// Lookup
// ------------------------------------------------------------------

const WCHAR* T(const WCHAR* en)
{
	if (!en || !s_strings[0]) return en;
	// Linear scan of the double-null-terminated block. 21 entries, walked a
	// couple of dozen times when a dialog or the tray menu is built, and never
	// per frame — an index would cost more code than it saves.
	for (const WCHAR* p = s_strings; *p; p += lstrlenW(p) + 1)
	{
		const WCHAR* eq = wcschr(p, L'=');
		if (!eq || eq == p) continue;                // no key, or no value: skip
		// Compare only the key half, without copying it out.
		if (CompareStringOrdinal(p, static_cast<int>(eq - p), en, -1, TRUE) != CSTR_EQUAL)
			continue;
		// An empty value is a string a translator has not filled in yet. That is
		// a normal state of a shipped file, and it means English.
		return eq[1] ? eq + 1 : en;
	}
	return en;
}

bool LocIsRTL()
{
	return s_rtl;
}
