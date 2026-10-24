# Nginx cho Windows tích hợp `.htaccess` & Windows Service Native

Đây là phiên bản biên dịch tùy chỉnh và tối ưu hóa cao của **Nginx (phiên bản 1.31.8)** dành cho môi trường Windows (hỗ trợ cả x64 và x86). Điểm đặc biệt của phiên bản này là **hỗ trợ đọc trực tiếp file `.htaccess` và `.htpasswd` của Apache**, đồng thời **tích hợp sâu với Windows Service** mà không cần dùng công cụ bên thứ ba như NSSM.

Dự án này là cầu nối hoàn hảo giữa kiến trúc bất đồng bộ siêu tốc của Nginx và sự tiện lợi trong cấu hình theo thư mục của Apache, giúp các dự án cũ chạy mượt mà trên Windows.

---

## 🚀 Tính năng nổi bật
- **Tầng Tương thích `.htaccess` Native:** Viết hoàn toàn bằng C (`ngx_http_htaccess_module`), can thiệp vào vòng đời xử lý request của Nginx để đọc và áp dụng rules ngay lập tức.
- **Native Windows Service:** Nginx master process giao tiếp trực tiếp với Windows Service Control Manager (SCM). Xử lý trơn tru các tín hiệu start/stop/restart, loại bỏ lỗi crash exit code 1.
- **Hỗ trợ Multiple Instances:** Cấu hình, cài đặt, và quản lý song song nhiều service Nginx với tên gọi và cấu hình riêng biệt trên cùng một server.
- **Kiến trúc Virtual Host chuẩn Debian:** Hỗ trợ sẵn thư mục `sites-available` và `sites-enabled` trên Windows, giúp quản lý domain gọn gàng.
- **Biên dịch bằng MSVC (Visual Studio):** Đem lại hiệu suất và độ ổn định vượt trội so với MinGW/Cygwin.
- **Bảo mật và Hiện đại:** Tích hợp PCRE2 (10.45), Zlib 1.3.1 và OpenSSL 3.5.x LTS. Hỗ trợ HTTP/2 và TLS 1.3.
- **An toàn luồng (Thread-Safe):** Bỏ qua các nút thắt bộ nhớ đệm cũ trên Windows. Cấp phát bộ nhớ qua memory pool nội bộ (`ngx_palloc`), không gây rò rỉ (Memory Leak).

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
- `--service-name <name>`: Tên định danh nội bộ cho service. (Ví dụ: `nginx_app1`).
- `--service-display-name <name>`: Tên hiển thị đầy đủ trong `services.msc`.
- `--service-account <account>`: Tài khoản Windows chạy service. Mặc định là `NT AUTHORITY\NetworkService`.
- `--service-password <password>`: Mật khẩu của tài khoản.

---

## 🐛 Bản vá mã nguồn gốc (Native Bug Patches)

Bản build này đã vá 2 lỗi nghiêm trọng từ mã nguồn gốc của Nginx 1.31.8 Windows:
1. **Lỗi treo vòng lặp `WSASend`:** Nginx gốc sẽ bị treo vĩnh viễn (Infinite loop) khi đẩy file tĩnh lớn xuống socket nếu bật `sendfile on;` trên Windows. Bản build này ép tắt sendfile ảo ở mức lõi, giúp phục vụ tệp tĩnh tuyệt đối an toàn.
2. **Lỗi mất Chunked Header:** MSVC sinh sai cấu hình khiến `ngx_http_chunked_filter_module` bị ghi đè khởi tạo. Lỗi này đã được sửa ở `ngx_modules.c`, Nginx hiện có thể gửi đầy đủ header `Transfer-Encoding: chunked`, ngăn chặn lỗi treo trình duyệt khi tải danh sách thư mục Autoindex.

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

## 📦 Danh sách Module tích hợp
Các module tiêu biểu bao gồm: `ngx_core`, `ngx_http_ssl`, `ngx_http_v2`, `ngx_http_realip`, `ngx_http_gzip_static`, `ngx_http_sub_filter`, `ngx_http_proxy_module`, `ngx_http_fastcgi_module` và toàn bộ các Upstream module chuẩn.

---

## 📁 Cấu trúc thư mục
```text
WinNginx/
├── nginx-1.31.8/                  # Mã nguồn Nginx 1.31.8 (đã vá lỗi)
├── ngx_http_htaccess_module/      # Mã nguồn mô-đun .htaccess native
├── deps/                          # Thư viện phụ thuộc đã build (PCRE2, zlib, OpenSSL)
├── vs/                            # Solution và Project Visual Studio 2019
├── build/                         # File tạm trong quá trình biên dịch
├── nginx-1.31.8-bin/              # Thư mục đầu ra và chạy trực tiếp
│   ├── nginx.exe                  # Nginx binary 64-bit
│   ├── nginx_x86.exe              # Nginx binary 32-bit
│   ├── conf/                      # Cấu hình chính
│   │   ├── nginx.conf
│   │   ├── sites-available/       # Khai báo Virtual Hosts
│   │   └── sites-enabled/         # Virtual Hosts đang kích hoạt
│   ├── html/                      # Thư mục web gốc
│   ├── logs/                      # Access & Error log
│   ├── temp/                      # Bộ nhớ đệm tạm
│   └── docs/                      # Tài liệu kỹ thuật & Dev guide
├── build_nginx_x64.bat            # Script biên dịch bản x64
└── build_nginx_x86.bat            # Script biên dịch bản x86
```
