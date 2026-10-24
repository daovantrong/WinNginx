$ROOT_DIR = $PSScriptRoot
if (-not $ROOT_DIR) { $ROOT_DIR = "D:\WinNginx" }
$NGINX_DIR = Join-Path $ROOT_DIR "nginx-1.31.8-bin"
if (-not (Test-Path $NGINX_DIR)) { $NGINX_DIR = $ROOT_DIR }
$NGINX_EXE = "nginx.exe"
if (-not (Test-Path "$NGINX_DIR\$NGINX_EXE")) { $NGINX_EXE = "build\Release\nginx.exe"; $NGINX_DIR = $ROOT_DIR }
$CONF_DIR = "$NGINX_DIR\conf"

Write-Host "Setting up test environment..."
# Clean up any existing test state
if (Test-Path "$CONF_DIR\sites-available") { Remove-Item "$CONF_DIR\sites-available" -Recurse -Force }
if (Test-Path "$CONF_DIR\sites-enabled") { Remove-Item "$CONF_DIR\sites-enabled" -Recurse -Force }

New-Item -ItemType Directory -Force "$CONF_DIR\sites-available" | Out-Null
New-Item -ItemType Directory -Force "$CONF_DIR\sites-enabled" | Out-Null

function Run-NginxTest {
    param([string]$Message)
    Write-Host "`n=== Test: $Message ==="
    $result = & $NGINX_DIR\$NGINX_EXE -t -p $NGINX_DIR 2>&1
    $success = $LASTEXITCODE -eq 0
    if ($success) {
        Write-Host "RESULT: PASS" -ForegroundColor Green
    } else {
        Write-Host "RESULT: FAIL" -ForegroundColor Red
        Write-Host $result
    }
    return $success
}

function Run-NginxTestFail {
    param([string]$Message)
    Write-Host "`n=== Test: $Message ==="
    $result = & $NGINX_DIR\$NGINX_EXE -t -p $NGINX_DIR 2>&1
    $success = $LASTEXITCODE -ne 0
    if ($success) {
        Write-Host "RESULT: PASS (Failed as expected)" -ForegroundColor Green
        Write-Host $result
    } else {
        Write-Host "RESULT: FAIL (Succeeded unexpectedly)" -ForegroundColor Red
    }
    return $success
}

# Ensure include is in nginx.conf
$confContent = Get-Content "$CONF_DIR\nginx.conf"
if ($confContent -notmatch "include\s+sites-enabled/\*.conf;") {
    $confContent = $confContent -replace "include\s+mime\.types;", "include       mime.types;`n    include       sites-enabled/*.conf;"
    Set-Content "$CONF_DIR\nginx.conf" $confContent
}

# Test 1: Basic enabled site
Set-Content "$CONF_DIR\sites-available\example.conf" "server { listen 8081; server_name example.local; location / { return 200 'example'; } }"
Copy-Item "$CONF_DIR\sites-available\example.conf" "$CONF_DIR\sites-enabled\example.conf"
Run-NginxTest "Basic enabled site"

# Test 2: Multiple sites
Set-Content "$CONF_DIR\sites-enabled\site2.conf" "server { listen 8082; server_name site2.local; }"
Run-NginxTest "Multiple sites"

# Test 3: Disabled site
Set-Content "$CONF_DIR\sites-available\disabled.conf" "server { listen 8083; invalid_directive_test; }"
Run-NginxTest "Disabled site (not in sites-enabled)"

# Test 4: Wildcard include
# Verified implicitly by Test 1 and 2

# Test 5: Absolute Windows path
# We test absolute path directly using nginx -t
Write-Host "`n=== Test: Absolute Windows path ==="
$absConf = "$CONF_DIR\nginx_abs_test.conf"
$sitesPattern = ($NGINX_DIR -replace '\\', '/') + "/conf/sites-enabled/*.conf"
Set-Content $absConf "events {} http { include $sitesPattern; }"
$result = & $NGINX_DIR\$NGINX_EXE -t -c $absConf -p $NGINX_DIR 2>&1
if ($LASTEXITCODE -eq 0) {
    Write-Host "RESULT: PASS" -ForegroundColor Green
} else {
    Write-Host "RESULT: FAIL" -ForegroundColor Red
}
Remove-Item $absConf

# Test 6: No matching files
Remove-Item "$CONF_DIR\sites-enabled\*" -Force
Run-NginxTest "No matching files"

# Test 7: Invalid site configuration
Set-Content "$CONF_DIR\sites-enabled\broken.conf" "invalid_directive;"
Run-NginxTestFail "Invalid site configuration"
Remove-Item "$CONF_DIR\sites-enabled\broken.conf"

# Test 8: Reload
Write-Host "`n=== Test: Reload ==="
Start-Process -FilePath "$NGINX_DIR\$NGINX_EXE" -ArgumentList "-p $NGINX_DIR" -WindowStyle Hidden
Start-Sleep -Seconds 1
Set-Content "$CONF_DIR\sites-enabled\reload.conf" "server { listen 8084; }"
$result = & $NGINX_DIR\$NGINX_EXE -s reload -p $NGINX_DIR 2>&1
if ($LASTEXITCODE -eq 0) {
    Write-Host "RESULT: PASS" -ForegroundColor Green
} else {
    Write-Host "RESULT: FAIL" -ForegroundColor Red
    Write-Host $result
}
& $NGINX_DIR\$NGINX_EXE -s stop -p $NGINX_DIR 2>&1 | Out-Null
Remove-Item "$CONF_DIR\sites-enabled\*" -Force -ErrorAction SilentlyContinue
Remove-Item "$CONF_DIR\sites-available\*" -Force -ErrorAction SilentlyContinue

Write-Host "`nAll tests completed."
