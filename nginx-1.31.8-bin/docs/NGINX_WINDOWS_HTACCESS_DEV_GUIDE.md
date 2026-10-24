# Nginx Windows `.htaccess` Compatibility Layer & Developer Guide

Tài liệu này ghi chú chi tiết kiến trúc, các vấn đề gặp phải, cách giải quyết và các bản vá lỗi native trên Nginx Windows (phiên bản 1.31.3) trong quá trình phát triển Nginx `.htaccess` Compatibility Module. Mục đích là để các lập trình viên dễ dàng tra cứu, nâng cấp và maintain source code trong tương lai.

---

## 1. Tổng quan Kiến trúc Module `.htaccess`

Module `ngx_http_htaccess_module` được thiết kế để phân tích và thực thi file `.htaccess` ngay tại thời điểm runtime (request time), tập trung xử lý ở **Access Phase**. Do tính chất bất biến của cấu hình Nginx (Immutable Configuration), module phải áp dụng các chiến thuật "can thiệp mềm" thay vì thay đổi trực tiếp cấu trúc toàn cục.

### Các tính năng đã hỗ trợ:
1. **Khối lệnh lồng nhau (Block Structures)**: 
   Sử dụng trình phân tích đệ quy (`ngx_htaccess_parse_ast_recursive`) để xây dựng Cây cú pháp trừu tượng (AST) lưu trực tiếp vào memory pool của request (`r->pool`).
2. **`<IfModule>`**: 
   Thực hiện đánh giá sớm các module phổ biến như `mod_rewrite.c`, `mod_authz_core.c`. Nếu điều kiện không thoả mãn, trình phân tích từ vựng sẽ bỏ qua việc phân tích AST bên trong để tiết kiệm bộ nhớ.
3. **`<FilesMatch>`**: 
   Biên dịch Regular Expression bằng `ngx_regex_compile`. Nếu URL request khớp với Regex, các rule bên trong mới được nối (merge) vào danh sách thực thi chính.
4. **`AuthBasic`**: 
   Đọc tệp tin `.htpasswd` và giải mã xác thực trực tiếp. Nếu không có header xác thực hoặc xác thực sai, module tự trả về response `401 Unauthorized` kèm header `WWW-Authenticate`.
5. **`ErrorDocument`**: 
   Khi gặp mã lỗi (ví dụ: 404), module tạo một shallow copy của `ngx_http_core_loc_conf_t`, tiêm danh sách `error_pages` tùy chỉnh vào, sau đó thay thế tạm thời `r->loc_conf` để Nginx tự động sinh trang lỗi nội bộ (Internal Redirect).
6. **`DirectoryIndex`**: 
   Bắt các URI kết thúc bằng dấu `/`. Sử dụng API native Windows `GetFileAttributesExW` (cùng bộ chuyển đổi chuỗi sang UTF-16) để quét ổ đĩa. Khi tìm thấy file chỉ mục (vd: `index.php`), kích hoạt `ngx_http_internal_redirect` ngầm để gọi lại file tĩnh.
7. **`Options +Indexes`**: 
   Tạo bản sao tạm thời của `ngx_http_autoindex_loc_conf_t`, tiêm cờ `enable = 1` và tráo đổi vào `r->loc_conf`.

---

## 2. Các vấn đề Kiến trúc (Đã khắc phục)

### A. Lỗi Access Violation (`C0000005`) trong Giai đoạn Keep-Alive
- **Vấn đề**: Trong quá trình test ban đầu, Nginx liên tục crash sập toàn bộ tiến trình Worker trong thời gian chờ Keep-Alive của request.
- **Nguyên nhân**: Thiết kế ban đầu sửa đổi trực tiếp vào biến toàn cục của file cấu hình, đồng thời lưu trữ cây AST vào một linked-list tĩnh (global cache) không có khóa Mutex (`ngx_htaccess_cache_insert`). Vì Nginx Windows sử dụng mô hình đa luồng (multi-threading với IOCP), việc thay đổi dữ liệu cấu hình hoặc truy xuất cache chung đã gây ra lỗi rác bộ nhớ (Heap Corruption).
- **Cách sửa**: 
  1. Gỡ bỏ hoàn toàn global cache. Quá trình đọc, phân tích AST được thực thi độc lập trên `r->pool` của mỗi request.
  2. Bất kỳ cấu hình nào cần can thiệp (như `ErrorDocument`, `Autoindex`) đều phải thông qua **Shallow Copy** (Tạo bản sao `r->loc_conf` vào `r->pool`, sửa trên bản sao, rồi cập nhật con trỏ). Nginx Core sẽ tự dọn dẹp khi request kết thúc.

---

## 3. Các Lỗi Native Nginx Core trên Windows (Bugs & Patches)

Trong quá trình test file tĩnh và hiển thị thư mục (Autoindex), đã phát hiện **2 bug nghiêm trọng từ chính bản thân mã nguồn lõi của Nginx Windows 1.31.3** (không liên quan đến module htaccess). Dưới đây là thông tin và cách vá khi update Nginx lên bản mới:

### Bug 1: Treo vòng lặp vô hạn `WSASend` (Sendfile Bug)
- **Triệu chứng**: Khi cấu hình `sendfile on;` được kích hoạt trong `nginx.conf`, việc gọi các file tĩnh dung lượng lớn khiến kết nối Nginx bị treo vĩnh viễn (Infinite loop).
- **Nguyên nhân**: Nginx Windows 1.31.3 nội bộ đã vô hiệu hoá hàm `TransmitFile` native (`NGX_HAVE_SENDFILE = 0`). Tuy nhiên, do cờ `sendfile on;` vẫn được đọc bình thường, hàm `ngx_output_chain_as_is` lọt các buffer có cờ `in_file=1` xuống socket writer của Windows (`ngx_wsasend_chain`). Socket này chỉ nhận memory buffer, dẫn đến việc tính toán độ dài dữ liệu `size = 0` và rơi vào vòng lặp lặp lại mãi mãi.
- **Bản vá (Cách nâng cấp/Fix)**:
  Sửa trong file `src/core/ngx_output_chain.c` (Hàm `ngx_output_chain_as_is`):
  Cần tiêm cấu trúc `#if !(NGX_HAVE_SENDFILE)` để ép vô hiệu hoá `sendfile`.
  ```c
      sendfile = ctx->sendfile;

  #if !(NGX_HAVE_SENDFILE)
      sendfile = 0; /* Patch: Ép tắt sendfile trên OS không hỗ trợ */
  #endif

  #if (NGX_SENDFILE_LIMIT)
  ```

### Bug 2: Lỗi sập luồng hiển thị Autoindex (Thiếu Filter Chunked)
- **Triệu chứng**: Trình duyệt hoặc lệnh `curl` bị treo cứng (hang) không bao giờ kết thúc response khi hiển thị danh sách thư mục (`Options +Indexes`).
- **Nguyên nhân**: Phản hồi HTTP thiếu header `Transfer-Encoding: chunked`. Lý do sâu xa nằm ở **thứ tự khởi tạo các filter** trong file sinh tự động của MSVC (`objs/ngx_modules.c`). Trình tự filter module bị sắp xếp ngược (backward). Module `ngx_http_header_filter_module` nằm bét bảng, do đó nó khởi tạo cuối cùng, ghi đè biến con trỏ filter `ngx_http_top_header_filter` và tước bỏ tất cả filter đứng trước nó (bao gồm cả chunked module).
- **Bản vá (Cách nâng cấp/Fix)**:
  Khi dùng lệnh gen project cho Windows, phải can thiệp vào `objs/ngx_modules.c`. Cần sắp xếp mảng `ngx_modules[]` và `ngx_module_names[]` để các HTTP filter chạy đúng quy trình tiêu chuẩn (Forward Order). Đặc biệt, `header_filter` phải đứng gần trên cùng:
  ```c
    &ngx_http_write_filter_module,
    &ngx_http_header_filter_module,
    &ngx_http_chunked_filter_module,
    &ngx_http_v2_filter_module,
    ...
    &ngx_http_slice_filter_module,
  ```

---

## 4. Hướng dẫn Compile & Build Nginx
- **Môi trường**: MSVC 2019/2022 (Visual Studio C++ Build Tools).
- **Thư mục source**: `c:\nginx_x64\`
- **Cách Build**:
  Chạy lệnh `build_nginx_x64.bat` trong command prompt hoặc powershell.
  ```cmd
  cd c:\nginx_x64\
  build_nginx_x64.bat
  ```
- **Kết quả Build**: Nằm ở `c:\nginx_x64\build\Release\nginx.exe`.
