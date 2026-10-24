# Nginx for Windows with Native `.htaccess` & Service Integration

[![Platform](https://img.shields.io/badge/platform-windows-blue.svg)](https://nginx.org)
[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)]()
[![C](https://img.shields.io/badge/language-C-orange.svg)]()

A heavily modified, highly optimized build of **Nginx (1.31.8)** for Windows (x64 & x86), uniquely featuring **native, drop-in support for Apache `.htaccess` and `.htpasswd` files**, as well as **Native Windows Service Integration** without needing third-party wrappers like NSSM.

This project bridges the gap between Nginx's asynchronous event-driven architecture and Apache's directory-level configuration convenience, while ensuring rock-solid stability as a Windows background service.

---

## 🚀 Key Features

* **Native `.htaccess` Parsing:** Written entirely in C as an internal Nginx module (`ngx_http_htaccess_module`). Intercepts Nginx execution phases to parse and enforce rules directly at the directory level.
* **Native Windows Service Integration:** Nginx master process communicates directly with the Windows Service Control Manager (SCM). Supports graceful start, stop, and restart (Native lifecycle routing), eliminating crash exit code 1.
* **Multiple Instances Support:** Configure, install, and manage multiple parallel Nginx services with custom names and configuration files on the same server.
* **MSVC (Visual Studio) Compiled:** Delivers superior performance and stability compared to MinGW/Cygwin builds.
* **Modern & Secure:** Integrates PCRE2 (10.45), Zlib 1.3.1, and OpenSSL 3.5.x LTS. Fully supports HTTP/2 and TLS 1.3.
* **Debian-Style Virtual Hosts:** Out-of-the-box support for `sites-available` and `sites-enabled` architecture on Windows.
* **Thread-Safe & High Performance:** Bypasses legacy caching bottlenecks on Windows to prevent multi-worker deadlock. Uses Nginx's internal memory pools (`ngx_palloc`), ensuring zero memory leaks.

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

This build natively fixes 2 critical bugs from the original Nginx 1.31.8 Windows source code:
1. **`WSASend` Infinite Loop Bug:** The original Nginx hangs indefinitely when pushing large static files to the socket if `sendfile on;` is enabled on Windows. This build forcefully disables dummy sendfile at the core level (`ngx_output_chain_as_is`), making static file serving absolutely safe.
2. **Missing Chunked Header Bug:** MSVC generates backward module initialization arrays, overwriting the `ngx_http_chunked_filter_module`. This has been fixed in `ngx_modules.c`, allowing Nginx to correctly send `Transfer-Encoding: chunked` headers, preventing browser hangs during directory listing (Autoindex).

---

## ⚙️ How it Works under the Hood

Unlike slow proxies or external Lua scripts, this project compiles the `.htaccess` interpreter directly into the Nginx Core `nginx.exe`.
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

## 📦 Built-in Modules
Includes: `ngx_core`, `ngx_http_ssl`, `ngx_http_v2`, `ngx_http_realip`, `ngx_http_gzip_static`, `ngx_http_sub_filter`, `ngx_http_proxy_module`, `ngx_http_fastcgi_module`, and many standard Upstream/HTTP modules.

---

## 📁 Directory Structure (Windows)
```text
WinNginx/
├── nginx-1.31.8/                  # Nginx 1.31.8 source code (patched)
├── ngx_http_htaccess_module/      # Native .htaccess C module source
├── deps/                          # Compiled dependencies (PCRE2, zlib, OpenSSL)
├── vs/                            # Visual Studio 2019 solution & projects
├── build/                         # Intermediate build artifacts
├── nginx-1.31.8-bin/              # Output & runtime distribution folder
│   ├── nginx.exe                  # Compiled Nginx x64 binary
│   ├── nginx_x86.exe              # Compiled Nginx x86 binary
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
└── build_nginx_x86.bat            # Build script for x86
```

## 📜 License
Extends the original Nginx source code (2-clause BSD-like license). Custom `.htaccess` modules retain the same licensing.
