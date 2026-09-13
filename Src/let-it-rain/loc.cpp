// loc.cpp — which words the UI uses. Ported from MinClock, which carries the
// same file with a second half for dates that this app has no use for.
//
// Storage is one STRINGTABLE per language in Localization\strings.rc,
// generated from Localization\translations.csv and compiled into the exe, so
// the exe ships as a single file. A blank translation is filled with the
// English at generation time, so every language block carries every id and a
// half-translated language is a working one.
//
// LoadStringW is not used because it takes no language: it answers in the
// thread's UI language, and SetThreadUILanguage to steer it would also change
// which language system UI on this thread loads (the colour picker, for one),
// with a fallback order that is not ours. Instead the language is chosen once
// here and T() reads that block directly.
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

// RT_STRING resources are bundles of 16: ids n*16 .. n*16+15 live in bundle n+1.
static LPCWSTR Bundle(UINT id) { return MAKEINTRESOURCEW((id >> 4) + 1); }

static LANGID s_lang = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
static bool   s_rtl;

// ------------------------------------------------------------------
// Choosing the language
// ------------------------------------------------------------------

// The languages strings.rc was built with, read back from the exe rather than
// listed again here — the same-language tier below needs the whole list, not
// a yes or no per LANGID.
struct Shipped { LANGID id[32]; UINT n; };

static BOOL CALLBACK CollectLang(HMODULE, LPCWSTR, LPCWSTR, WORD lang, LONG_PTR param)
{
	auto* s = reinterpret_cast<Shipped*>(param);
	if (s->n < _countof(s->id)) s->id[s->n++] = lang;
	return TRUE;
}

void LocInit()
{
	// The display language chain, most preferred first. The chain matters as
	// much as the name: Windows answers e.g. "de-AT" -> "de" -> "de-DE", and
	// following it is how a language pack we have no exact block for still
	// resolves to one we do. 512 WCHARs is dozens of languages; a chain that
	// overflows it leaves us in English.
	WCHAR langs[512] = { 0 };
	ULONG count = 0, cch = _countof(langs);
	if (!GetUserPreferredUILanguages(MUI_LANGUAGE_NAME, &count, langs, &cch))
		langs[0] = 0;

	// Mirroring follows the language the UI is *written in*, so it reads from
	// the same chain — an Arabic display language on a US region still wants
	// mirrored windows, translated or not. A null name is
	// LOCALE_NAME_USER_DEFAULT, the documented fallback for an empty chain.
	DWORD layout = 0;
	if (GetLocaleInfoEx(langs[0] ? langs : nullptr,
	                    LOCALE_IREADINGLAYOUT | LOCALE_RETURN_NUMBER,
	                    reinterpret_cast<WCHAR*>(&layout), sizeof(layout) / sizeof(WCHAR)))
		s_rtl = (layout == 1);                       // 1 = RTL, reading LTR lines

	Shipped shipped{};
	EnumResourceLanguagesW(nullptr, RT_STRING, Bundle(IDS_LOC_FIRST), CollectLang,
	                       reinterpret_cast<LONG_PTR>(&shipped));

	// Both tiers for one language before moving to the next, which is the whole
	// point of a preference order: a de-AT primary must reach de-DE before an
	// en-US secondary is even considered.
	for (const WCHAR* p = langs; *p; p += lstrlenW(p) + 1)
	{
		// Plenty of real locales have no LCID of their own — pt-AO maps to a
		// transient one with primary language 0 — so those retry with
		// just the language part, which does have one ("pt" -> 0x16) and is all
		// the same-language tier needs. 0 after that is a name Windows cannot
		// place at all: next.
		LANGID want = LANGIDFROMLCID(LocaleNameToLCID(p, 0));
		if (PRIMARYLANGID(want) == LANG_NEUTRAL)
		{
			WCHAR lang[LOCALE_NAME_MAX_LENGTH];
			if (FAILED(StringCchCopyW(lang, _countof(lang), p))) continue;
			if (WCHAR* dash = wcschr(lang, L'-')) *dash = 0;
			want = LANGIDFROMLCID(LocaleNameToLCID(lang, LOCALE_ALLOW_NEUTRAL_NAMES));
			if (PRIMARYLANGID(want) == LANG_NEUTRAL) continue;
		}

		// Exact first, and that matters rather than being tidy: pt-PT/pt-BR and
		// zh-CN/zh-TW are different blocks, and going straight to the primary
		// language would hand half of those users the other half's translation.
		for (UINT i = 0; i < shipped.n; i++)
			if (shipped.id[i] == want) { s_lang = want; return; }

		// Then any block of the same language. This serves the regional variants
		// nobody ships a block for — es-MX takes es-ES, de-AT and de-CH take
		// de-DE — and also the neutral "de" entries in the chain.
		//
		// It resolves by enumeration order, which is LANGID order: the language
		// is right, the flavour can be wrong. zh-HK takes zh-TW (0x0404, before
		// zh-CN's 0x0804), which is right for Hong Kong; zh-SG takes it too,
		// which is not, as Singapore writes Simplified. pt-AO takes pt-BR. The
		// fix is a column in translations.csv, never code: an exact block wins.
		for (UINT i = 0; i < shipped.n; i++)
			if (PRIMARYLANGID(shipped.id[i]) == PRIMARYLANGID(want)) { s_lang = shipped.id[i]; return; }
	}
}

// ------------------------------------------------------------------
// Lookup
// ------------------------------------------------------------------

const WCHAR* T(UINT id)
{
	// s_lang is always a language EnumResourceLanguagesW reported, so this finds
	// that exact block. A bundle holds 16 entries back to back, each a WORD
	// length and then that many WCHARs. /n on strings.rc appends a null to
	// every string and counts it in the length, so the walk below steps over it
	// and the pointer can be handed out as is. An unused slot has length 0.
	// Walked when a dialog or the tray menu is built, never per frame.
	const HRSRC res = FindResourceExW(nullptr, RT_STRING, Bundle(id), s_lang);
	const HGLOBAL mem = res ? LoadResource(nullptr, res) : nullptr;
	const WCHAR* p = mem ? static_cast<const WCHAR*>(LockResource(mem)) : nullptr;
	if (!p) return L"";
	for (UINT i = id & 0xF; i; i--) p += 1 + *p;
	return *p ? p + 1 : L"";                         // length 0: no such id
}

bool LocIsRTL()
{
	return s_rtl;
}
