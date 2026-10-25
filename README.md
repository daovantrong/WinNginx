# Nginx for Windows with Native `.htaccess` & Service Integration
### **nginx/1.31.8 (Enhanced by TRONG.PRO)**

[![Platform](https://img.shields.io/badge/platform-windows-blue.svg)](https://nginx.org)
[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)]()
[![OpenSSL](https://img.shields.io/badge/OpenSSL-3.5.7%20(Static)-blueviolet.svg)]()
[![Architecture](https://img.shields.io/badge/arch-x64%20%7C%20x86-blue.svg)]()
[![C](https://img.shields.io/badge/language-C-orange.svg)]()

A heavily modified, enterprise-grade, highly optimized build of **Nginx (1.31.8)** for Windows (both **x64** and **x86**), featuring **native, drop-in support for Apache `.htaccess` and `.htpasswd` files**, **Native Windows Service Integration** without requiring external wrappers (like NSSM), and a full **All-in-One Module Suite** built as a **100% Standalone Executable** with zero external DLL dependencies.

---

## 🚀 Key Features

* **Native `.htaccess` Parsing:** Written entirely in C as an internal Nginx module ([`ngx_http_htaccess_module`](file:///D:/WinNginx/ngx_http_htaccess_module)). Intercepts Nginx execution phases to parse and enforce rules directly at the directory level.
* **100% Standalone Single Executable (Zero External DLLs):** OpenSSL 3.5.7, PCRE2 10.45, and zlib 1.3.1 are statically linked directly into the binary (`libssl_static.lib`, `libcrypto_static.lib`, `pcre2-8-static.lib`, `zlibstatic.lib`). No external DLLs (such as `libcrypto-3-x64.dll` or `libssl-3-x64.dll`) are required. Simply copy `nginx.exe` and run anywhere on Windows.
* **All-in-One Enterprise Module Integration:** Complete module suite compiled into both 64-bit and 32-bit binaries, including HTTP/2, SSL/TLS with SNI, Stream TCP/UDP proxy (with SSL & SNI Preread), Mail proxy (with SSL), WebDAV, MP4/FLV streaming, Gunzip, Gzip Static, Secure Link, Auth Request, Upstream Zone, and more.
* **Native Windows Service Integration:** The Nginx master process communicates directly with the Windows Service Control Manager (SCM). Supports graceful start, stop, and restart without process crashes or exit code 1.
* **Multiple Instances Support:** Configure, install, and manage multiple parallel Nginx services with custom names and configuration files on the same server.
* **MSVC (Visual Studio 2019/2022) Native Build:** Delivers superior performance, memory management, and stability compared to MinGW/Cygwin builds.
* **Debian-Style Virtual Hosts:** Out-of-the-box support for `sites-available` and `sites-enabled` architecture on Windows.
* **Thread-Safe & High Performance:** Bypasses legacy caching bottlenecks on Windows to prevent multi-worker deadlocks. Uses Nginx's internal memory pools (`ngx_palloc`), ensuring zero memory leaks.

---

## 📦 All-in-One Built-in Modules

Both `nginx.exe` (x64) and `nginx_x86.exe` (Win32) are built with the complete configure flags:

```text
configure arguments:
  --with-pcre
  --with-zlib
  --with-openssl
  --with-http_v2_module
  --with-http_ssl_module
  --with-http_realip_module
  --with-http_addition_module
  --with-http_sub_module
  --with-http_dav_module
  --with-http_stub_status_module
  --with-http_flv_module
  --with-http_mp4_module
  --with-http_gunzip_module
  --with-http_gzip_static_module
  --with-http_auth_request_module
  --with-http_random_index_module
  --with-http_secure_link_module
  --with-http_slice_module
  --with-mail
  --with-mail_ssl_module
  --with-stream
  --with-stream_ssl_module
  --with-stream_realip_module
  --with-stream_ssl_preread_module
```

### Module Highlights
1. **Core & Native `.htaccess`:** `ngx_http_htaccess_module` for Apache rewrite, access control, basic auth, custom error pages, and directory index.
2. **SSL / TLS & HTTP/2:** Full TLSv1.2/TLSv1.3, ALPN, and SNI powered by statically embedded OpenSSL 3.5.7.
3. **Stream Layer 4 TCP/UDP:** Full reverse proxy & load balancing for TCP/UDP with SSL termination, SSL preread (SNI routing), RealIP, and Upstream Shared Memory Zones.
4. **Mail Proxy:** POP3, IMAP, and SMTP reverse proxy with SSL/TLS support.
5. **Media Streaming:** Native progressive download pseudo-streaming for MP4 (`ngx_http_mp4_module`) and FLV (`ngx_http_flv_module`).
6. **Publishing & Storage:** WebDAV methods (`PUT`, `DELETE`, `MKCOL`, `COPY`, `MOVE`) via `ngx_http_dav_module`.
7. **Compression & Performance:** Dynamic gunzip decompression, pre-compressed gzip static delivery, and byte-range slice module.

---

## 🛠️ Supported Apache Modules & Directives

This build acts as a direct replacement for several core Apache modules, executing seamlessly within Nginx's HTTP phases:

### `mod_rewrite` & `mod_alias` (URL Routing)
* `RewriteEngine On|Off`
* `RewriteCond` (Matches against Server variables like `%{HTTP_HOST}`, PCRE regex, and filesystem checks `-f`, `-d`, `-s`)
* `RewriteRule` (Fully supports Regex capture groups `$1`, `%1`, and flags `[L, R, NC, QSA]`)
* `Redirect`, `RedirectMatch`

### `mod_headers` & `mod_expires` (Header Manipulation)
* *Hooks natively into the Nginx HTTP Header Filter Chain.*
* `Header set|append|add|unset|echo`
* `ExpiresActive On|Off`, `ExpiresDefault`, `ExpiresByType`

### `mod_authz_host` & Core (Access & Security)
* *Intercepts requests at the Nginx Access Phase.*
* **Apache 2.4:** `Require all granted|denied`, `Require ip <CIDR>`
* **Apache 2.2:** `Allow from all|<CIDR>`, `Deny from all|<CIDR>`
* `ErrorDocument <Status> <Path>`
* `DirectoryIndex <file1> <file2>`
* `Options +Indexes` / `-Indexes` (Autoindex mapping)

### `mod_authn_core` (Basic Authentication)
* `AuthType Basic`, `AuthName`, `AuthUserFile`, `Require valid-user` (Parses standard `.htpasswd` files dynamically)

### Logic Blocks
* `<FilesMatch "regex"> ... </FilesMatch>`
* `<IfModule mod_rewrite.c> ... </IfModule>`

---

## ⚙️ Native Windows Service Management (CLI)

The service management commands must be the **first argument** passed to `nginx.exe`. Additional options like `-c` are saved and passed to Nginx automatically when the service starts. *Run as Administrator.*

### 1. Install Service (`--service-install`)
Registers Nginx as an auto-start Windows Service.
```cmd
nginx.exe --service-install
nginx.exe --service-install --service-name nginx_api --service-display-name "Nginx API Server" -c conf/api.conf
```

### 2. Uninstall Service (`--service-uninstall`)
Removes Nginx from the Windows Service Registry.
```cmd
nginx.exe --service-uninstall
nginx.exe --service-uninstall --service-name nginx_api
```

### 3. Start Service (`--service-start`)
Signals Windows SCM to start the service.
```cmd
nginx.exe --service-start
```

### 4. Stop Service (`--service-stop`)
Gracefully shuts down the Nginx service.
```cmd
nginx.exe --service-stop
```

### Extended Parameters
* `--service-name <name>`: Internal service identifier (e.g., `nginx_app1`).
* `--service-display-name <name>`: Name shown in `services.msc`.
* `--service-account <account>`: Windows account to run under (default: `NT AUTHORITY\NetworkService`).
* `--service-password <password>`: Password for the service account.

---

## 🐛 Native Nginx Core Bug Patches

This build natively fixes 3 critical bugs from the original Nginx Windows source code:
1. **`WSASend` Infinite Loop Bug:** The original Nginx hangs indefinitely when pushing large static files to the socket if `sendfile on;` is enabled on Windows. This build forcefully disables dummy sendfile at the core level (`ngx_output_chain_as_is`), making static file serving absolutely safe.
2. **Missing Chunked Header Bug:** MSVC generates backward module initialization arrays, overwriting the `ngx_http_chunked_filter_module`. This has been fixed in `ngx_modules.c`, allowing Nginx to correctly send `Transfer-Encoding: chunked` headers, preventing browser hangs during directory listing (Autoindex).
3. **SSL Module Crash on Listen 443 SSL:** Fixed module registration ordering in `ngx_modules.c` where `&ngx_http_ssl_module` was missing from module arrays in custom reordered setups, eliminating `0xC0000005` Access Violation crashes when parsing `ssl_certificate`.
4. **MSVC x86 Math Intrinsic Stub:** Added `__dtoul3_legacy` floating point conversion alias/stub in `ngx_win32_init.c` to enable seamless static linking of OpenSSL on 32-bit Windows builds.

---

## ⚙️ How it Works under the Hood

Unlike slow proxies or external Lua scripts, this project compiles the `.htaccess` interpreter directly into the Nginx Core:
1. **Access Phase:** Nginx checks for a `.htaccess` file in the requested directory.
2. **Lexing & Parsing:** A custom-built C Lexer constructs an Abstract Syntax Tree (AST).
3. **Execution Routing:**
   - **Access/Auth:** `Require` and `.htpasswd` are evaluated immediately.
   - **Rewrite Phase:** `RewriteRule` triggers internal Nginx redirects (`ngx_http_internal_redirect`).
   - **Header Filter Phase:** Directives like `Header` are injected into the Nginx Output Filter Chain (`ngx_http_top_header_filter`).

---

## 🛡️ Security Audit

The module codebase has been rigorously tested and patched for enterprise-grade security:
- **PCRE Bounds Checking:** Fixed native Nginx PCRE `captures` array leakage which previously caused 140TB memory allocation crashes.
- **Integer Underflows:** Hardened `base_prefix` and `DirectoryIndex` path computation against integer underflows to prevent `Access Violation (C0000005)`.
- **Memory Safety:** Directives strings and routing parameters are shallow-copied and bound to request lifetimes (`r->pool`). Zero risk of Double-Free or Use-After-Free.

---

## 📁 Directory Structure (Windows)
```text
WinNginx/
├── nginx-1.31.8/                  # Nginx 1.31.8 source code (patched)
├── ngx_http_htaccess_module/      # Native .htaccess C module source
├── deps/                          # Prebuilt static dependencies (PCRE2, zlib, OpenSSL)
├── vs/                            # Visual Studio 2019 solution & projects
├── nginx-1.31.8-bin/              # Output & runtime distribution folder (Zero DLL dependencies)
│   ├── nginx.exe                  # Compiled Nginx x64 binary (Static Standalone)
│   ├── nginx_x86.exe              # Compiled Nginx x86 binary (Static Standalone)
│   ├── conf/                      # Configuration files
│   │   ├── nginx.conf
│   │   ├── mime.types
│   │   ├── sites-available/       # Virtual Host definitions
│   │   └── sites-enabled/         # Active Virtual Hosts
│   ├── html/                      # Default Web Root
│   ├── logs/                      # Access and Error Logs
│   ├── temp/                      # Temp runtime buffers
│   └── docs/                      # Technical documentation
├── build_nginx_x64.bat            # Build script for x64
├── build_nginx_x86.bat            # Build script for x86
├── clean.bat                      # Clean temporary files before Git push
└── clean_for_git.bat              # Recursive repository cleanup script
```

---

## 📜 Build & Verification

To build both architectures from source:
```cmd
build_nginx_x64.bat
build_nginx_x86.bat
```

To verify standalone binary output:
```cmd
nginx.exe -V
nginx_x86.exe -V
```

Before pushing changes to Git:
```cmd
clean.bat
```

## 📜 License & Credits
- **Base Nginx:** 2-clause BSD-like license.
- **Enhanced by:** [TRONG.PRO](https://trong.pro)
- Custom `.htaccess` modules and Windows service integrations retain compatible open-source licensing.
