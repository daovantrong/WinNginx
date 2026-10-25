# Thư viện Cấu hình Mẫu NGINX (`conf-examples`)

Thư mục này chứa các mẫu cấu hình chuẩn mực, tối ưu hóa hiệu năng và bảo mật cao nhất dành riêng cho các kịch bản triển khai web phổ biến trên Windows.

> [!TIP]
> Xem bản tiếng Anh tại [English Documentation](README.md).

---

## 📂 Danh mục Cấu hình Mẫu

| Tên File | Mục đích & Ứng dụng | Điểm nổi bật |
| :--- | :--- | :--- |
| **[`reverse-proxy-apache.conf`](reverse-proxy-apache.conf)** | NGINX đứng trước làm Front-end cho Apache Back-end (`127.0.0.1:8080`) | NGINX phục vụ 100% file tĩnh & cache; PHP và logic động chuyển sang Apache; hỗ trợ WebSocket |
| **[`php-fastcgi.conf`](php-fastcgi.conf)** | NGINX thuần chạy trực tiếp với PHP-CGI / PHP-FPM (`127.0.0.1:9000`) | Kích hoạt module Native `.htaccess`, upstream cluster PHP, tối ưu hóa FastCGI buffer & timeout |
| **[`wordpress-optimized.conf`](wordpress-optimized.conf)** | Chuyên biệt cho mã nguồn WordPress trên Windows | Tự động đọc `.htaccess` của WP; chặn xmlrpc; bảo vệ wp-config; chặn chạy PHP trong uploads |
| **[`spa-vue-react.conf`](spa-vue-react.conf)** | Frontend Single Page Application (React, Vue, Vite, Angular) | HTML5 History Mode routing; no-cache cho `index.html`; cache vĩnh viễn `/assets/`; proxy `/api/` |
| **[`load-balancer-cluster.conf`](load-balancer-cluster.conf)** | Cân bằng tải cho cụm nhiều máy chủ (Cluster) | Thuật toán `least_conn`, `ip_hash`; Keepalive upstream; tự động failover khi backend lỗi |
| **[`ssl-tls-hardening.conf`](ssl-tls-hardening.conf)** | Bảo mật SSL/TLS A+ & HTTP/2 | Ép chuyển hướng HTTP -> HTTPS; TLS 1.3/1.2; OpenSSL 3.5.7 tĩnh; đầy đủ HSTS, CSP, X-Frame-Options |
| **[`stream-tcp-udp-proxy.conf`](stream-tcp-udp-proxy.conf)** | Layer 4 Stream Proxy (TCP & UDP) | Cân bằng tải MySQL (3306), Redis (6379), DNS UDP (53); SNI Passthrough (SSL Preread) |
| **[`webdav-file-server.conf`](webdav-file-server.conf)** | Máy chủ lưu trữ và chia sẻ file WebDAV | Mount thành ổ đĩa mạng Windows; hỗ trợ PUT/DELETE/MKCOL/COPY/MOVE; bảo mật Basic Auth |

---

## 🚀 Cách Sử dụng

Để sử dụng bất kỳ cấu hình mẫu nào cho website của bạn:

1. **Sao chép sang thư mục `sites-available`:**
   ```cmd
   copy conf-examples\reverse-proxy-apache.conf conf\sites-available\mysite.conf
   ```
2. **Chỉnh sửa các thông số thực tế trong `conf\sites-available\mysite.conf`:**
   - `server_name`: Tên miền của bạn (ví dụ: `mysite.local`, `example.com`).
   - `root`: Đường dẫn thư mục web thực tế (ví dụ: `C:/www/mysite/public` hoặc `D:/www/mysite/public`).
   - `access_log` & `error_log`: Đường dẫn lưu log.
3. **Kích hoạt Virtual Host:**
   - **Cách 1 (Dùng script PowerShell có sẵn):**
     ```powershell
     .\nginx-site.ps1 -Enable mysite
     ```
   - **Cách 2 (Copy thủ công sang `sites-enabled`):**
     ```cmd
     copy conf\sites-available\mysite.conf conf\sites-enabled\
     ```
4. **Kiểm tra cú pháp và tải lại NGINX:**
   ```cmd
   nginx.exe -t
   nginx.exe -s reload
   ```
