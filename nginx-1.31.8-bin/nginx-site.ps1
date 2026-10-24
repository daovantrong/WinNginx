param (
    [Parameter(Mandatory=$true, Position=0)]
    [ValidateSet("enable", "disable", "list")]
    [string]$Action,

    [Parameter(Mandatory=$false, Position=1)]
    [string]$SiteName
)

$ErrorActionPreference = "Stop"

# Get paths relative to this script
$NginxDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
if (Test-Path (Join-Path $NginxDir "nginx-1.31.8-bin\nginx.exe")) {
    $NginxDir = Join-Path $NginxDir "nginx-1.31.8-bin"
}
$NginxExe = Join-Path $NginxDir "nginx.exe"
if (-not (Test-Path $NginxExe)) {
    $NginxExe = Join-Path $NginxDir "build\Release\nginx.exe"
}
$ConfDir = Join-Path $NginxDir "conf"
$SitesAvailable = Join-Path $ConfDir "sites-available"
$SitesEnabled = Join-Path $ConfDir "sites-enabled"

# Ensure directories exist
if (-not (Test-Path $SitesAvailable)) { New-Item -ItemType Directory -Force $SitesAvailable | Out-Null }
if (-not (Test-Path $SitesEnabled)) { New-Item -ItemType Directory -Force $SitesEnabled | Out-Null }

function Ensure-ConfExtension {
    param ([string]$Name)
    if ($Name -notmatch "\.conf$") {
        return "$Name.conf"
    }
    return $Name
}

function Invoke-NginxReload {
    Write-Host "Reloading Nginx..." -ForegroundColor Cyan
    $p = Start-Process -FilePath $NginxExe -ArgumentList "-s reload -p `"$NginxDir`"" -Wait -NoNewWindow -PassThru
    if ($p.ExitCode -eq 0) {
        Write-Host "Nginx reloaded successfully." -ForegroundColor Green
    } else {
        Write-Host "Warning: Nginx reload returned exit code $($p.ExitCode). Is Nginx running?" -ForegroundColor Yellow
    }
}

switch ($Action) {
    "list" {
        Write-Host ""
        Write-Host "Nginx Sites Status:" -ForegroundColor Cyan
        Write-Host "-------------------"
        
        $availableSites = Get-ChildItem -Path $SitesAvailable -Filter "*.conf" | Select-Object -ExpandProperty Name
        $enabledSites = Get-ChildItem -Path $SitesEnabled -Filter "*.conf" | Select-Object -ExpandProperty Name

        if ($availableSites.Count -eq 0) {
            Write-Host "  (No sites found in conf/sites-available/)" -ForegroundColor Gray
        } else {
            foreach ($site in $availableSites) {
                if ($enabledSites -contains $site) {
                    Write-Host "  [+] $site" -ForegroundColor Green
                } else {
                    Write-Host "  [ ] $site" -ForegroundColor Gray
                }
            }
        }
        Write-Host ""
    }

    "enable" {
        if ([string]::IsNullOrWhiteSpace($SiteName)) {
            Write-Host "Error: Site name is required." -ForegroundColor Red
            exit 1
        }

        $SiteName = Ensure-ConfExtension $SiteName
        $source = Join-Path $SitesAvailable $SiteName
        $target = Join-Path $SitesEnabled $SiteName

        if (-not (Test-Path $source)) {
            Write-Host "Error: Site configuration not found: $source" -ForegroundColor Red
            exit 1
        }

        if (Test-Path $target) {
            Write-Host "Site '$SiteName' is already enabled." -ForegroundColor Yellow
            exit 0
        }

        Write-Host "Enabling site $SiteName..." -ForegroundColor Cyan
        Copy-Item $source $target -Force

        # Test Nginx configuration
        Write-Host "Testing Nginx configuration..." -ForegroundColor Cyan
        $p = Start-Process -FilePath $NginxExe -ArgumentList "-t -p `"$NginxDir`"" -Wait -NoNewWindow -PassThru
        
        if ($p.ExitCode -ne 0) {
            Write-Host "Configuration test failed! Rolling back changes..." -ForegroundColor Red
            Remove-Item $target -Force
            exit 1
        }

        Invoke-NginxReload
    }

    "disable" {
        if ([string]::IsNullOrWhiteSpace($SiteName)) {
            Write-Host "Error: Site name is required." -ForegroundColor Red
            exit 1
        }

        $SiteName = Ensure-ConfExtension $SiteName
        $target = Join-Path $SitesEnabled $SiteName

        if (-not (Test-Path $target)) {
            Write-Host "Site '$SiteName' is not enabled." -ForegroundColor Yellow
            exit 0
        }

        Write-Host "Disabling site $SiteName..." -ForegroundColor Cyan
        Remove-Item $target -Force

        Invoke-NginxReload
    }
}
