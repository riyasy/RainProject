# build.ps1 — regenerate lang\<locale>.ini from translations.csv, then verify them.
#
# translations.csv is the master: one row per English string, one column per
# locale, plus a "context" column that exists only for human reviewers. The .ini
# files are generated, so never hand-edit one - edit the CSV and re-run this.
#
# The verify half is not optional politeness. GetPrivateProfileSectionW decides a
# file's encoding from the BOM alone; a file saved as UTF-8 or ANSI reads back as
# mojibake *silently*, with the source looking perfectly correct. That is the
# trap loc.cpp documents, and this is what catches it.

$ErrorActionPreference = 'Stop'
$dir = $PSScriptRoot
$rows = Import-Csv (Join-Path $dir 'translations.csv') -Encoding UTF8

$locales = @('ar-SA','de-DE','es-ES','fi-FI','fr-FR','hu-HU','it-IT','ja-JP','ko-KR',
             'ml-IN','nl-NL','pl-PL','pt-BR','pt-PT','ru-RU','sv-SE','uk-UA','zh-CN','zh-TW')

function Write-Ini($path, $lines) {
    # Encoding::Unicode is UTF-16LE, and WriteAllText emits its BOM.
    [System.IO.File]::WriteAllText($path, (($lines -join "`r`n") + "`r`n"),
                                   [System.Text.Encoding]::Unicode)
}

$header = @(
  '; Let It Rain FX UI strings. Key = the English text.'
  '; Blank value = fall back to English. UTF-16 LE with BOM, always.'
  '; Generated from translations.csv by build.ps1 - do not hand-edit.'
  '[Strings]'
)
foreach ($loc in $locales) {
    Write-Ini (Join-Path $dir "$loc.ini") ($header + ($rows | ForEach-Object { '{0}={1}' -f $_.key, $_.$loc }))
}

Write-Ini (Join-Path $dir 'template.ini') (@(
  '; Let It Rain FX UI strings. Key = the English text; fill in the value.'
  '; Blank value = falls back to English, so a partial file is a working file.'
  '; SAVE AS UTF-16 LE WITH BOM. Any other encoding reads as mojibake.'
  '[Strings]'
) + ($rows | ForEach-Object { '{0}=' -f $_.key }))

# ---- verify, through the same API the app will use ----
$sig = @'
[DllImport("kernel32.dll", CharSet=CharSet.Unicode, SetLastError=true)]
public static extern uint GetPrivateProfileSectionW(string sec, System.IntPtr buf, uint size, string file);
'@
$k = Add-Type -MemberDefinition $sig -Name IniCheck -Namespace LetItRain -PassThru
$cch = 32768
$buf = [System.Runtime.InteropServices.Marshal]::AllocHGlobal($cch * 2)
$fail = 0
foreach ($f in Get-ChildItem "$dir\*.ini" | Sort-Object Name) {
    $bom = [System.IO.File]::ReadAllBytes($f.FullName)[0..1] -join ','
    $n = $k::GetPrivateProfileSectionW('Strings', $buf, $cch, $f.FullName)
    # PtrToStringUni, not a StringBuilder: the section is double-null-terminated
    # and the marshaller would stop dead at the first embedded null.
    $entries = ([System.Runtime.InteropServices.Marshal]::PtrToStringUni($buf, [int]$n) -split "`0") |
               Where-Object { $_ }
    $bad = ($entries | Where-Object { $_ -match '\?\?|�' }).Count
    if ($bom -ne '255,254' -or $entries.Count -ne $rows.Count -or $bad -gt 0) {
        $fail++
        "FAIL {0}: BOM={1} keys={2}/{3} mojibake={4}" -f $f.Name, $bom, $entries.Count, $rows.Count, $bad
    }
}
[System.Runtime.InteropServices.Marshal]::FreeHGlobal($buf)

if ($fail) { throw "$fail language file(s) failed verification" }
"{0} strings x {1} locales - all files verified" -f $rows.Count, $locales.Count
