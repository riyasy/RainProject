# build.ps1 — regenerate strings.h and strings.rc from translations.csv.
#
# translations.csv is the master: one row per UI string, an "id" column that
# becomes the IDS_ name, one column per locale (en-US included), plus a
# "context" column that exists only for human reviewers. The locales are the
# CSV's own columns, so adding a language is a column and nothing here. Both
# outputs are generated, so never hand-edit either - edit the CSV and re-run
# this.
#
# strings.rc holds one STRINGTABLE per language, tagged with that language's
# LANGUAGE statement, and is compiled into the exe; loc.cpp picks the block.
# It is written as UTF-16LE with a BOM, the encoding rc.exe reads without a
# #pragma code_page and the one let-it-rain.rc already uses.

$ErrorActionPreference = 'Stop'
$dir = $PSScriptRoot
$rows = @(Import-Csv (Join-Path $dir 'translations.csv') -Encoding UTF8)

# Read from the header rather than listed here: a hand-kept list silently
# dropped any column it did not name, and a column it named that the CSV lacked
# shipped as English. en-US first, so it is always a block.
$locales = @('en-US') + @($rows[0].PSObject.Properties.Name | Where-Object { $_ -notin 'id', 'en-US', 'context' })

# The first id sits on a 16-string boundary, so the strings fill RT_STRING
# bundles from their first slot. Nothing depends on that; it just keeps the
# resource tidy. Well clear of Resource.h's IDs.
$base = 1024

# The ids become C identifiers and the English becomes the fallback for every
# blank cell, so both are checked before anything is written.
$seen = @{}
foreach ($r in $rows) {
    if ($r.id -cnotmatch '^IDS_[A-Z0-9_]+$') { throw "bad id '$($r.id)'" }
    if ($seen[$r.id]) { throw "duplicate id $($r.id)" }
    $seen[$r.id] = $true
    if (-not $r.'en-US') { throw "$($r.id) has no English" }
}

$h = @(
  '// strings.h - generated from translations.csv by build.ps1. Do not hand-edit.'
  '#pragma once'
  ''
  "#define IDS_LOC_FIRST $base"
)
for ($i = 0; $i -lt $rows.Count; $i++) { $h += '#define {0,-24} {1}' -f $rows[$i].id, ($base + $i) }
[System.IO.File]::WriteAllText((Join-Path $dir 'strings.h'), (($h -join "`r`n") + "`r`n"),
                               [System.Text.Encoding]::ASCII)

# RC string literals: a quote is doubled, a backslash starts an escape.
function RcQuote($s) { '"' + ($s -replace '\\', '\\' -replace '"', '""') + '"' }

$rc = @(
  '// strings.rc - generated from translations.csv by build.ps1. Do not hand-edit.'
  '// Numeric ids rather than #include "strings.h", so rc.exe needs no include path.'
)
foreach ($loc in $locales) {
    # CultureInfo throws on an unknown name, which is the check we want. A
    # custom or transient locale (LCID 0x1000, 0x2000...) has primary language
    # 0, no LANGID a resource can carry, so it cannot ship this way at all.
    $lcid = [System.Globalization.CultureInfo]::new($loc).LCID
    if (($lcid -band 0x3FF) -eq 0) { throw "$loc has no fixed LCID" }
    $rc += ''
    $rc += 'LANGUAGE 0x{0:X2}, 0x{1:X2}    // {2}' -f ($lcid -band 0x3FF), (($lcid -shr 10) -band 0x3F), $loc
    $rc += 'STRINGTABLE'
    $rc += 'BEGIN'
    for ($i = 0; $i -lt $rows.Count; $i++) {
        # A blank cell is a string nobody has translated yet: it ships as English.
        $text = $rows[$i].$loc
        if (-not $text) { $text = $rows[$i].'en-US' }
        $rc += '    {0,-5} {1}' -f ($base + $i), (RcQuote $text)
    }
    $rc += 'END'
}
[System.IO.File]::WriteAllText((Join-Path $dir 'strings.rc'), (($rc -join "`r`n") + "`r`n"),
                               [System.Text.Encoding]::Unicode)

"{0} strings x {1} locales -> strings.h, strings.rc" -f $rows.Count, $locales.Count
