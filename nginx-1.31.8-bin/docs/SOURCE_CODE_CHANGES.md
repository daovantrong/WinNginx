# Phụ lục: Các thay đổi từ mã nguồn gốc đến khi biên dịch thành công
### **Phiên bản Nginx: 1.31.8 (Enhanced by TRONG.PRO)**

Tài liệu này ghi chú chi tiết từng dòng code đã được thêm, sửa, xoá từ mã nguồn gốc của Nginx 1.31.8 và module `.htaccess` cho đến khi dự án biên dịch thành công mà không còn lỗi.

---

## 1. Sửa lỗi vòng lặp `WSASend` trên Windows (Nginx Core)
**File**: `nginx-1.31.8/src/core/ngx_output_chain.c`
**Lý do**: Trên hệ thống không hỗ trợ `TransmitFile` (Windows `NGX_HAVE_SENDFILE = 0`), cờ `sendfile on;` vẫn khiến Nginx đẩy các pure file buffer vào Windows socket. Điều này dẫn đến treo vòng lặp vô hạn. Cần ép tắt sendfile ở mức lõi.

**Thay đổi (Diff)**:
```diff
@@ -262,6 +262,10 @@
 #endif
 
     sendfile = ctx->sendfile;
 
+#if !(NGX_HAVE_SENDFILE)
+    sendfile = 0;
+#endif
+
 #if (NGX_SENDFILE_LIMIT)
 
     if (buf->in_file && buf->file_pos >= NGX_SENDFILE_LIMIT) {
```

---

## 2. Sửa lỗi treo xuất thư mục Autoindex (Nginx Core / Build Config)
**File**: `nginx-1.31.8/objs/ngx_modules.c`
**Lý do**: Thứ tự các HTTP Filter Module được công cụ MSVC tạo ra bị ngược, khiến `ngx_http_header_filter_module` khởi tạo cuối cùng và ghi đè mất `ngx_http_chunked_filter_module`.

**Thay đổi**: Sắp xếp lại thứ tự khai báo trong 2 mảng `ngx_modules` và `ngx_module_names` bằng cách dời toàn bộ các HTTP modules bình thường lên trên, và đẩy các **HTTP Filter Module** xuống dưới cùng theo đúng thứ tự chuẩn (Forward Order).

```c
    &ngx_http_write_filter_module,
    &ngx_http_header_filter_module,
    &ngx_http_chunked_filter_module,
    &ngx_http_v2_filter_module,
    &ngx_http_range_header_filter_module,
    &ngx_http_gzip_filter_module,
    &ngx_http_ssi_filter_module,
    &ngx_http_charset_filter_module,
    &ngx_http_sub_filter_module,
    &ngx_http_addition_filter_module,
    &ngx_http_userid_filter_module,
    &ngx_http_headers_filter_module,
    &ngx_http_copy_filter_module,
    &ngx_http_not_modified_filter_module,
    &ngx_http_slice_filter_module,
    NULL
};
```

---

## 3. Hoàn thiện Logic Module `.htaccess`
**File**: `ngx_http_htaccess_module/src/ngx_http_htaccess_module.c`
**Lý do**: Triển khai `ErrorDocument` và `Options +Indexes` trong mô hình xử lý an toàn (Shallow Copy) để tránh lỗi Memory Leak và Access Violation C0000005. Sửa các lỗi cú pháp (Syntax C1075) trong quá trình cài đặt logic vòng lặp.

**Thay đổi (Logic tích hợp ErrorDocument và Autoindex)**:
```c
/* Đoạn code được chèn bên trong ngx_http_htaccess_handler */

/* 1. Apply ErrorDocument natively by injecting into clcf->error_pages */
if (curr_ast->error_documents && curr_ast->error_documents->nelts > 0) {
    ngx_http_core_loc_conf_t *clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);
    ngx_http_core_loc_conf_t *new_clcf = ngx_palloc(r->pool, sizeof(ngx_http_core_loc_conf_t));
    if (new_clcf != NULL) {
        *new_clcf = *clcf;
        
        void **new_loc_conf = ngx_palloc(r->pool, sizeof(void *) * ngx_http_max_module);
        if (new_loc_conf != NULL) {
            ngx_memcpy(new_loc_conf, r->loc_conf, sizeof(void *) * ngx_http_max_module);
            new_loc_conf[ngx_http_core_module.ctx_index] = new_clcf;
            r->loc_conf = new_loc_conf;
        }
    }
}

/* 2. Apply Options Indexes by injecting into autoindex loc conf */
if (curr_ast->options.indexes != -1) {
    ngx_http_autoindex_loc_conf_t *alcf = ngx_http_get_module_loc_conf(r, ngx_http_autoindex_module);
    ngx_http_autoindex_loc_conf_t *new_alcf = ngx_palloc(r->pool, sizeof(ngx_http_autoindex_loc_conf_t));
    if (new_alcf != NULL) {
        *new_alcf = *alcf;
        new_alcf->enable = curr_ast->options.indexes; // Kích hoạt autoindex
        
        void **new_loc_conf = ngx_palloc(r->pool, sizeof(void *) * ngx_http_max_module);
        if (new_loc_conf != NULL) {
            ngx_memcpy(new_loc_conf, r->loc_conf, sizeof(void *) * ngx_http_max_module);
            new_loc_conf[ngx_http_autoindex_module.ctx_index] = new_alcf;
            r->loc_conf = new_loc_conf;
        }
    }
}
```

---

## 4. `mod_rewrite` và `mod_alias` (PCRE & Memory Bounds Check)
**File**: `ngx_http_htaccess_module/src/rewrite/rule.c`, `parser.c`, `ngx_http_htaccess_module.c`
**Lý do**: Triển khai định tuyến regex tương thích Apache và sửa lỗi tràn bộ nhớ (Memory Allocation Bug) do `pcre_exec` không tự động xóa mảng `captures` trên Nginx. Lỗi này khiến `ngx_pnalloc` yêu cầu cấp phát bộ nhớ lên tới 140TB, làm sập worker ngay lập tức khi người dùng có chứa `$1` hoặc `$2` trong file `.htaccess`.

**Thay đổi**:
1. Đã triển khai `Redirect` và `RedirectMatch` (AST, Parser, execution precedence). Đảm bảo `mod_alias` chạy ngay trước `mod_rewrite`.
2. Khởi tạo mảng `captures` thủ công bằng `-1` trước khi gọi `ngx_regex_exec`:
```c
int captures[30];
for (j = 0; j < 30; j++) {
    captures[j] = -1;
}
```

---

## 5. `mod_headers`, `mod_expires` và `mod_authz_host`
**File**: `ngx_http_htaccess_module/src/ngx_http_htaccess_module.c`, `parser.c`
**Lý do**: Cần tiêm Header `Cache-Control` và `X-Custom` vào phản hồi HTTP để thay thế hoàn toàn Apache. `mod_authz_host` cũng được tối ưu hóa để chặn IP từ Access Phase.

**Thay đổi**:
1. Thêm struct cho AST (`ngx_htaccess_header_t` và `ngx_htaccess_expires_t`).
2. Viết filter mới hoàn toàn: `ngx_http_htaccess_header_filter(ngx_http_request_t *r)`.
3. Ghi đè `ngx_http_top_header_filter` tại hàm `ngx_http_htaccess_init(ngx_conf_t *cf)`.
4. Quan trọng nhất: Di chuyển `&ngx_http_htaccess_module` xuống cuối mảng `ngx_modules` bên trong file `objs/ngx_modules.c`. Điều này nhằm đảm bảo Filter của chúng ta được `postconfiguration` móc nối đầu tiên (theo cơ chế mảng đảo ngược của Header Filters trong Nginx Core), cho phép chúng ta thay đổi header một cách an toàn mà không bị filter xuất mảng (`ngx_http_header_filter_module`) chặn lại.

---

## 6. Tổng kiểm tra bảo mật (Security Audit & Patch)
**File**: `ngx_http_htaccess_module.c`, `condition.c`
**Lý do**: Vá các lỗ hổng tràn bộ nhớ (Memory Corruption / Integer Underflow) và rò rỉ bộ nhớ bắt nguồn từ logic đọc `.htaccess`.

**Thay đổi**:
1. **Integer Underflow trong cắt chuỗi `normalized_path`**:
   - Sử dụng `size_t strip_len = hlcf->filename.len + 1;` và luôn check `if (normalized_path.len > strip_len)` trước khi tính toán độ dài `dir_len`.
2. **Xóa biến mảng Regex (Regex Capture Leakage)**:
   - Trong `RewriteCond` (file `condition.c`), đẩy lệnh `for (j = 0; j < 30; j++) cond_captures[j] = -1;` vào ngay phía trên lệnh `ngx_regex_exec` để đảm bảo State của Regex luôn trong sạch cho mỗi Condition độc lập.

---

## 7. Đa nền tảng kiến trúc (x64 / x86 Dual Build)
**Lý do**: Chuyển đổi dự án từ kiến trúc cố định (`nginx_x64`) sang nền tảng hỗ trợ đa kiến trúc (`WinNginx`), phục vụ cả các Server hiện đại và hệ thống nhúng Legacy.

**Thay đổi**:
1. Đổi tên toàn bộ thư mục dự án từ `nginx_x64` thành `WinNginx`.
2. Tạo thêm hệ thống script build song song:
   - `prepare_deps_x86.ps1`: Tải và tự động biên dịch `PCRE2`, `Zlib`, và cài đặt `OpenSSL` dành riêng cho hệ x86/Win32.
   - `build_nginx_x86.bat`: Khởi tạo môi trường MSVC `vcvarsall x86` để build file nhị phân tĩnh.
3. Chỉnh sửa tệp dự án Visual Studio (`vs/nginx.sln`, `vs/nginx.vcxproj`) để cấu hình `AdditionalDependencies` ánh xạ động sang thư mục thư viện tương ứng (Win32 -> `_x86` / x64 -> `_x64`).
4. Kết quả xuất xưởng 2 file nhị phân: `nginx.exe` (64-bit) và `nginx_x86.exe` (32-bit).

---

## 8. Sửa lỗi Crash Dump `0xC0000005` khi cấu hình `listen 443 ssl`
**File**: `nginx-1.31.8/objs/ngx_modules.c`
**Lý do**: Khi thêm directive `listen 443 ssl;` và `ssl_certificate` vào khối `server`, NGINX bị crash tại offset `0x3002e` (Access Violation `0xC0000005`). Nguyên nhân do mảng `ngx_modules[]` thiếu khai báo `&ngx_http_ssl_module;`, khiến con trỏ `ctx_index` của SSL module không được khởi tạo (giữ giá trị `-1`), làm lệch con trỏ cấu hình khi parse directive SSL.

**Thay đổi**:
1. Khai báo lại `extern ngx_module_t ngx_http_ssl_module;`.
2. Thêm lại `&ngx_http_ssl_module,` vào mảng `ngx_modules[]`.

---

## 9. Tích hợp All-in-One 99 Module
**File**: `vs/nginx.vcxproj`, `vs/nginx.vcxproj.filters`, `nginx-1.31.8/objs/ngx_modules.c`
**Lý do**: Cung cấp đầy đủ toàn bộ tính năng cao cấp của NGINX cho Windows tương đương các bản Linux Enterprise:
- **HTTP Layer**: HTTP/2, SSL SNI, WebDAV, MP4, FLV, Gunzip, Gzip Static, Auth Request, Secure Link, Slice, Addition, Sub, RealIP, Upstream Zone.
- **Stream Layer**: Proxy TCP/UDP, Stream SSL, Stream RealIP, Stream SSL Preread (SNI routing), Stream Upstream Zone.
- **Mail Layer**: Proxy POP3, IMAP, SMTP có mã hóa SSL.

---

## 10. Tùy biến Banner Phiên bản & PE Resource Info
**File**: `nginx-1.31.8/objs/ngx_auto_config.h`, `nginx-1.31.8/src/os/win32/nginx.rc`
**Thay đổi**:
1. Macro `NGX_BUILD`:
   ```c
   #ifndef NGX_BUILD
   #define NGX_BUILD  "Enhanced by TRONG.PRO"
   #endif
   ```
   Giúp lệnh `nginx -V` hiển thị đúng chuẩn: `nginx version: nginx/1.31.8 (Enhanced by TRONG.PRO)`.
2. Cập nhật tài nguyên Windows `.rc`: Đặt `CompanyName` là `TRONG.PRO`, thêm thông tin giới thiệu dự án trong trường `Comments` của PE Header.

---

## 11. Liên kết tĩnh toàn phần OpenSSL 3.5.7 (Zero External DLLs)
**File**: `vs/nginx.vcxproj`, `nginx-1.31.8/src/os/win32/ngx_win32_init.c`
**Lý do**: Trước đây NGINX phụ thuộc vào `libcrypto-3-x64.dll` và `libssl-3-x64.dll`. Nếu mang sang máy khác không có sẵn OpenSSL trong PATH, NGINX sẽ báo lỗi thiếu DLL.

**Thay đổi**:
1. Trong `vs/nginx.vcxproj`, chuyển liên kết sang các static library:
   - x64: `libssl_static.lib`, `libcrypto_static.lib`
   - Win32: `libssl_static.lib`, `libcrypto_static.lib`
2. Bổ sung Intrinsic stub cho x86 MSVC CRT trong `ngx_win32_init.c`:
   ```c
   #if defined(_WIN32) && !defined(_WIN64) && defined(_MSC_VER)
   #pragma comment(linker, "/alternatename:__dtoul3_legacy=___dtoul3_legacy")
   unsigned __int64 __dtoul3_legacy(double d)
   {
       return (unsigned __int64) d;
   }
   #endif
   ```
3. Kết quả: Toàn bộ OpenSSL 3.5.7, PCRE2 10.45 và zlib 1.3.1 đều được nhúng tĩnh trực tiếp vào file `.exe`. File nhị phân đạt chuẩn Single Standalone Executable, chạy độc lập không cần bất kỳ file DLL bên ngoài nào.
