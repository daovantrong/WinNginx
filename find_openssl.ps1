$hashData = Invoke-WebRequest -Uri 'https://raw.githubusercontent.com/slproweb/opensslhashes/master/win32_openssl_hashes.json' -UseBasicParsing | ConvertFrom-Json
$hashData.files.PSObject.Properties | Where-Object {
    $_.Value.bits -eq 64 -and
    $_.Value.light -eq $false -and
    $_.Value.installer -eq 'exe' -and
    $_.Value.arch -ne 'ARM'
} | ForEach-Object { "$($_.Value.basever)$($_.Value.subver) - $($_.Value.url)" }
