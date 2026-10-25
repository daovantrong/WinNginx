# Nginx cho Windows tích hợp `.htaccess` & Windows Service Native
### **nginx/1.31.8 (Enhanced by TRONG.PRO)**

[![Platform](https://img.shields.io/badge/platform-windows-blue.svg)](https://nginx.org)
[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)]()
[![OpenSSL](https://img.shields.io/badge/OpenSSL-3.5.7%20(Static)-blueviolet.svg)]()
[![Architecture](https://img.shields.io/badge/arch-x64%20%7C%20x86-blue.svg)]()
[![C](https://img.shields.io/badge/language-C-orange.svg)]()

Đây là phiên bản biên dịch tùy chỉnh cao cấp, tối ưu hóa toàn diện của **Nginx (phiên bản 1.31.8)** dành cho môi trường Windows (hỗ trợ cả **x64** và **x86**). Phiên bản này cung cấp giải pháp **All-in-One** hoàn chỉnh:
- **Đọc trực tiếp file `.htaccess` và `.htpasswd` của Apache** ngay tại tầng Core C.
- **Tích hợp sâu với Windows Service** trực tiếp qua SCM (không cần NSSM hay wrapper ngoài).
- **Standalone 100% (Single Executable):** Toàn bộ thư viện OpenSSL 3.5.7, PCRE2 10.45 và zlib 1.3.1 đều được liên kết tĩnh (Static Linking). **Không cần copy kèm bất kỳ file DLL nào** (như `libcrypto-3-x64.dll`, `libssl-3-x64.dll`,...). Chỉ cần đúng 1 file `.exe` duy nhất là có thể mang sang bất kỳ máy tính Windows nào chạy ngay lập tức.

---

## 🚀 Tính năng nổi bật

- **Tầng Tương thích `.htaccess` Native:** Viết hoàn toàn bằng ngôn ngữ C ([`ngx_http_htaccess_module`](ngx_http_htaccess_module)), can thiệp vào vòng đời xử lý request của Nginx để đọc và áp dụng rules ngay lập tức tại cấp thư mục.
- **Không phụ thuộc DLL ngoài (100% Static Linking):** Đã nhúng tĩnh hoàn toàn `libssl_static.lib`, `libcrypto_static.lib`, `pcre2-8-static.lib`, `zlibstatic.lib`. Không lo thiếu file DLL hoặc xung đột phiên bản OpenSSL có sẵn trên hệ điều hành máy trạm.
- **Tích hợp đầy đủ mọi Module (All-in-One):** Đầy đủ 99 module: HTTP/2, SSL/TLS với SNI, Stream TCP/UDP proxy (kèm SSL & SNI Preread), Mail proxy (kèm SSL), WebDAV, MP4/FLV media streaming, Gunzip, Gzip Static, Secure Link, Auth Request, Upstream Zone,...
- **Native Windows Service:** Nginx master process giao tiếp trực tiếp với Windows Service Control Manager (SCM). Xử lý trơn tru các tín hiệu start/stop/restart, loại bỏ lỗi crash exit code 1.
- **Hỗ trợ Multiple Instances:** Cấu hình, cài đặt, và quản lý song song nhiều service Nginx với tên gọi và cấu hình riêng biệt trên cùng một máy chủ.
- **Kiến trúc Virtual Host chuẩn Debian:** Hỗ trợ sẵn thư mục `sites-available` và `sites-enabled` trên Windows, giúp quản lý cấu hình các website gọn gàng, khoa học.
- **Biên dịch bằng MSVC (Visual Studio 2019/2022):** Hiệu năng vượt trội và quản lý bộ nhớ ổn định hơn nhiều so với các bản build MinGW hay Cygwin.
- **An toàn luồng (Thread-Safe):** Bỏ qua các nút thắt bộ nhớ đệm cũ trên Windows. Cấp phát bộ nhớ qua memory pool nội bộ (`ngx_palloc`), không gây rò rỉ RAM (Zero Memory Leak).

---

## 📦 Danh sách Module All-in-One tích hợp

Cả 2 bản build `nginx.exe` (x64) và `nginx_x86.exe` (Win32) đều kích hoạt toàn bộ tham số cấu hình:

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

### Chi tiết các nhóm Module:
1. **Lõi & Native `.htaccess`:** `ngx_http_htaccess_module` hỗ trợ rewrite, phân quyền thư mục, basic auth, custom error pages và autoindex.
2. **Bảo mật SSL/TLS & HTTP/2:** Đầy đủ TLS 1.2, TLS 1.3, ALPN và SNI chạy trên nền tảng OpenSSL 3.5.7 tĩnh.
3. **Stream Layer 4 TCP/UDP:** Proxy & cân bằng tải TCP/UDP tốc độ cao, hỗ trợ SSL termination, SSL SNI preread định tuyến luồng mã hóa, RealIP và Upstream Zone.
4. **Mail Proxy:** Reverse proxy cho giao thức email POP3, IMAP, SMTP có mã hóa SSL/TLS.
5. **Truyền dẫn Media:** Pseudo-streaming cho video MP4 (`ngx_http_mp4_module`) và video FLV (`ngx_http_flv_module`).
6. **WebDAV:** Hỗ trợ đầy đủ các phương thức HTTP quản lý file từ xa: `PUT`, `DELETE`, `MKCOL`, `COPY`, `MOVE`.
7. **Nén dữ liệu & Tối ưu:** Giải nén động Gunzip, phục vụ file nén sẵn Gzip Static, chia nhỏ dải dữ liệu Byte-range Slice.

---

## 🛠️ Các lệnh `.htaccess` và Module Apache được hỗ trợ

Bản build này thay thế trực tiếp nhiều module lõi của Apache, thực thi mượt mà trong các Phase của Nginx:

### `mod_rewrite` & `mod_alias` (Định tuyến URL)
- `RewriteEngine On|Off`
- `RewriteCond` (Hỗ trợ biến Server như `%{HTTP_HOST}`, PCRE regex, và kiểm tra file `-f`, `-d`, `-s`)
- `RewriteRule` (Hỗ trợ đầy đủ regex capture `$1`, `%1`, và flags `[L, R, NC, QSA]`)
- `Redirect`, `RedirectMatch`

### `mod_headers` & `mod_expires` (Quản lý Header)
- *Can thiệp trực tiếp vào Output Filter Chain của Nginx.*
- `Header set|append|add|unset|echo`
- `ExpiresActive On|Off`, `ExpiresDefault`, `ExpiresByType`

### `mod_authz_host` & Core (Truy cập & Bảo mật)
- *Chặn request trái phép ngay tại Access Phase.*
- **Apache 2.4:** `Require all granted|denied`, `Require ip <CIDR>`
- **Apache 2.2:** `Allow from all|<CIDR>`, `Deny from all|<CIDR>`
- `ErrorDocument <Status> <Path>` (Tự động Internal Redirect)
- `DirectoryIndex <file1> <file2>`
- `Options +Indexes` / `-Indexes` (Hiển thị danh sách thư mục)

### `mod_authn_core` (Xác thực Basic)
- `AuthType Basic`, `AuthName`, `AuthUserFile`, `Require valid-user` (Tự động đọc và parse chuẩn `.htpasswd` bằng thuật toán hash của Nginx).

### Cấu trúc Khối (Logic Blocks)
- `<FilesMatch "regex"> ... </FilesMatch>` (Phạm vi thao tác file)
- `<IfModule mod_rewrite.c> ... </IfModule>`

---

## ⚙️ Hướng dẫn quản lý Windows Service (CLI)

Lệnh quản lý Service phải là **đối số đầu tiên** truyền cho `nginx.exe`. (Yêu cầu chạy CMD với quyền Administrator).

### 1. Cài đặt Service (`--service-install`)
Đăng ký Nginx thành Service tự khởi động (Auto-start).
```cmd
nginx.exe --service-install
nginx.exe --service-install --service-name nginx_api --service-display-name "Nginx API Server" -c conf/api.conf
```

### 2. Gỡ cài đặt Service (`--service-uninstall`)
Xoá Nginx khỏi Windows Service Registry.
```cmd
nginx.exe --service-uninstall
nginx.exe --service-uninstall --service-name nginx_api
```

### 3. Khởi động Service (`--service-start`)
```cmd
nginx.exe --service-start
```

### 4. Dừng Service (`--service-stop`)
Graceful shutdown service Nginx an toàn.
```cmd
nginx.exe --service-stop
```

### Các Tham Số Mở Rộng
- `--service-name <name>`: Tên định danh nội bộ cho service (Ví dụ: `nginx_app1`).
- `--service-display-name <name>`: Tên hiển thị đầy đủ trong `services.msc`.
- `--service-account <account>`: Tài khoản Windows chạy service. Mặc định là `NT AUTHORITY\NetworkService`.
- `--service-password <password>`: Mật khẩu của tài khoản.

---

## 🐛 Bản vá mã nguồn gốc (Native Bug Patches)

Bản build này đã vá 4 lỗi nghiêm trọng từ mã nguồn gốc Nginx trên Windows:
1. **Lỗi treo vòng lặp `WSASend`:** Nginx gốc sẽ bị treo vĩnh viễn khi đẩy file tĩnh lớn xuống socket nếu bật `sendfile on;` trên Windows. Bản build này ép tắt sendfile ảo ở mức lõi, giúp phục vụ tệp tĩnh tuyệt đối an toàn.
2. **Lỗi mất Chunked Header:** MSVC sinh sai cấu hình khiến `ngx_http_chunked_filter_module` bị ghi đè khởi tạo. Lỗi này đã được sửa ở `ngx_modules.c`, Nginx hiện có thể gửi đầy đủ header `Transfer-Encoding: chunked`, ngăn chặn lỗi treo trình duyệt khi tải danh sách thư mục Autoindex.
3. **Lỗi crash bộ nhớ khi khai báo `listen 443 ssl`:** Khắc phục lỗi mảng `ngx_modules[]` thiếu con trỏ `&ngx_http_ssl_module;`, loại bỏ hoàn toàn lỗi Access Violation (`0xC0000005`) khi cấu hình chứng chỉ SSL.
4. **Bổ sung Intrinsic Stub MSVC cho x86:** Tích hợp symbol `__dtoul3_legacy` trong `ngx_win32_init.c` để hỗ trợ liên kết tĩnh OpenSSL mượt mà trên nền tảng 32-bit.

---

## ⚙️ Cơ chế hoạt động (Dưới mui xe)

Khác với Lua script, trình thông dịch `.htaccess` được biên dịch trực tiếp vào lõi Nginx:
1. **Giai đoạn Access:** Nginx kiểm tra sự tồn tại của file `.htaccess` trong thư mục hiện hành.
2. **Lexing & Parsing:** Trình Lexer tuỳ chỉnh (C) dựng Cây cú pháp trừu tượng (AST).
3. **Thực thi:**
   - Auth/IP bị chặn sớm trả về `401/403`.
   - `RewriteRule` kích hoạt `ngx_http_internal_redirect` ngầm, không tốn tài nguyên gửi phản hồi vòng qua mạng.
   - `Header` được tiêm vào Output Filter Chain để thay đổi cả phản hồi từ PHP/FastCGI.

---

## 🛡️ Kiểm toán bảo mật (Security Audit)

- **Rò rỉ PCRE Regex:** Sửa lỗi module PCRE gốc của Nginx tràn mảng `captures`, từng gây lỗi cấp phát 140TB RAM dẫn tới crash Worker.
- **Integer Underflow:** Ngăn chặn tuyệt đối các phép trừ underflow khi xử lý đường dẫn `base_prefix` ngắn, triệt tiêu lỗi `Access Violation (C0000005)`.
- **An toàn vùng nhớ:** Toàn bộ dữ liệu đọc từ `.htaccess` được **Shallow Copy** vào vòng đời của một Request (`r->pool`) và tự huỷ khi request kết thúc. Zero rủi ro Double-Free.

---

## 📁 Cấu trúc thư mục
```text
WinNginx/
├── nginx-1.31.8/                  # Mã nguồn Nginx 1.31.8 (đã vá lỗi)
├── ngx_http_htaccess_module/      # Mã nguồn mô-đun .htaccess native
├── deps/                          # Thư viện phụ thuộc tĩnh (PCRE2, zlib, OpenSSL)
├── vs/                            # Solution và Project Visual Studio 2019
├── nginx-1.31.8-bin/              # Thư mục phân phối và chạy trực tiếp (Không cần kèm DLL)
│   ├── nginx.exe                  # Nginx binary 64-bit (Standalone tĩnh)
│   ├── nginx_x86.exe              # Nginx binary 32-bit (Standalone tĩnh)
│   ├── conf/                      # Cấu hình chính đang hoạt động
│   │   ├── nginx.conf
│   │   ├── mime.types
│   │   ├── sites-available/       # Khai báo Virtual Hosts
│   │   └── sites-enabled/         # Virtual Hosts đang kích hoạt
│   ├── conf-examples/             # Thư viện cấu hình mẫu chuẩn tối ưu (reverse proxy, PHP,...)
│   ├── html/                      # Thư mục web gốc
│   ├── logs/                      # Access & Error log
│   ├── temp/                      # Bộ nhớ đệm tạm
│   └── docs/                      # Tài liệu kỹ thuật & Dev guide
├── build_nginx_x64.bat            # Script biên dịch bản x64
├── build_nginx_x86.bat            # Script biên dịch bản x86
├── clean.bat                      # Script dọn dẹp file tạm trước khi đẩy Git
└── clean_for_git.bat              # Script dọn dẹp đệ quy toàn bộ file rác
```

---

## 📜 Biên dịch & Dọn dẹp

Để biên dịch lại cả 2 kiến trúc từ source:
```cmd
build_nginx_x64.bat
build_nginx_x86.bat
```

Kiểm tra thông tin bản build:
```cmd
nginx.exe -V
nginx_x86.exe -V
```

Dọn sạch file rác biên dịch trước khi push lên Git:
```cmd
clean.bat
```

## 📜 Bản quyền & Đóng góp
- **Base Nginx:** Giấy phép 2-clause BSD.
- **Enhanced by:** [TRONG.PRO](https://trong.pro)
- Toàn bộ các module tích hợp và bản vá tuân thủ giấy phép mã nguồn mở tương thích.
