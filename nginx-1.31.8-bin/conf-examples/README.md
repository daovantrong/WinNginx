# NGINX Configuration Examples (`conf-examples`)

This directory contains production-ready, security-hardened, and performance-optimized configuration templates for common web workloads on Windows.

> [!TIP]
> Available in [Tiếng Việt](README.vi.md).

---

## 📂 Configuration Catalog

| Configuration Template | Target Architecture & Use Case | Key Features & Highlights |
| :--- | :--- | :--- |
| **[`reverse-proxy-apache.conf`](reverse-proxy-apache.conf)** | NGINX Front-end + Apache Back-end (`127.0.0.1:8080`) | NGINX serves 100% of static files & caches for 30d; dynamic fallback via `try_files $uri @apache;`; passes `.php` & WebSockets to Apache; blocks hidden files |
| **[`php-fastcgi.conf`](php-fastcgi.conf)** | Standalone NGINX + PHP-CGI / PHP-FPM (`127.0.0.1:9000`) | Native `.htaccess` support enabled; upstream PHP cluster pool; FastCGI 502 buffer hardening; prevents PHP execution in upload dirs |
| **[`wordpress-optimized.conf`](wordpress-optimized.conf)** | Specialized for WordPress on Windows | Native `.htaccess` support (SEO plugins & permalinks); blocks `xmlrpc.php`, `wp-config.php`; forbids PHP execution in `wp-content/uploads/` |
| **[`spa-vue-react.conf`](spa-vue-react.conf)** | Single Page Application (React, Vue, Vite, Angular) | HTML5 History Mode routing (`try_files $uri $uri/ /index.html;`); zero-cache for `index.html`; immutable cache for `/assets/*`; reverse proxy `/api/` |
| **[`load-balancer-cluster.conf`](load-balancer-cluster.conf)** | Multi-backend Load Balancer & Cluster | `least_conn`, `ip_hash`, and weighted round-robin; upstream keepalive connections; automatic failover & passive health checks |
| **[`ssl-tls-hardening.conf`](ssl-tls-hardening.conf)** | Grade A+ SSL / TLS & HTTP/2 Hardening | Automatic HTTP to HTTPS (Port 80 -> 443) redirection; TLS 1.3 & 1.2; Mozilla intermediate ciphers; HSTS, CSP, X-Frame-Options |
| **[`stream-tcp-udp-proxy.conf`](stream-tcp-udp-proxy.conf)** | Layer 4 Stream Proxy (TCP & UDP) | Load balancing for MySQL (`3306`), Redis (`6379`), DNS UDP (`53`); SSL Preread / SNI Passthrough without certificate decryption |
| **[`webdav-file-server.conf`](webdav-file-server.conf)** | WebDAV Remote File Storage & Sync Server | Mounts as Windows Map Network Drive; supports `PUT`, `DELETE`, `MKCOL`, `COPY`, `MOVE`; secured with Basic Authentication |

---

## 🚀 How to Use

To use any of these templates for your website:

1. **Copy the template into `conf/sites-available/`:**
   ```cmd
   copy conf-examples\reverse-proxy-apache.conf conf\sites-available\mysite.conf
   ```

2. **Customize paths in `conf\sites-available\mysite.conf`:**
   - `server_name`: Your domain name or IP (e.g., `mysite.local`, `example.com`).
   - `root`: Your website's public document root (e.g., `C:/www/mysite/public` or `D:/www/mysite/public`).
   - `access_log` & `error_log`: Desired log file paths.

3. **Enable the site:**
   - **Method A (Using the included PowerShell tool):**
     ```powershell
     .\nginx-site.ps1 -Enable mysite
     ```
   - **Method B (Manual copy to `sites-enabled`):**
     ```cmd
     copy conf\sites-available\mysite.conf conf\sites-enabled\
     ```

4. **Verify syntax and reload NGINX:**
   ```cmd
   nginx.exe -t
   nginx.exe -s reload
   ```
