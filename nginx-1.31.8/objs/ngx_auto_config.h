/* auto generated for nginx-1.31.8 Windows MSVC build */

#define NGX_CONFIGURE " --with-pcre --with-zlib --with-openssl --with-http_ssl_module --with-http_v2_module"

#define NGX_COMPILER   "MSVC 19.29"
#define NGX_BUILD      ""

#define NGX_HAVE_INT_SET_ERRNO  0

#define NGX_HAVE_GCC_VARIADIC_MACROS  0

#define NGX_HAVE_GCC_BSWAP64  0

#define NGX_HAVE_C99_VARIADIC_MACROS  1

#define NGX_HAVE_NONALIGNED  1

#ifdef _WIN64
#define NGX_PTR_SIZE  8
#define NGX_MAX_SIZE_T_VALUE  9223372036854775807LL
#define NGX_SIZE_T_LEN  (sizeof("-9223372036854775808") - 1)
#define NGX_SIZE_T_FMT  "Id"
#else
#define NGX_PTR_SIZE  4
#define NGX_MAX_SIZE_T_VALUE  2147483647L
#define NGX_SIZE_T_LEN  (sizeof("-2147483648") - 1)
#define NGX_SIZE_T_FMT  "d"
#endif

#define NGX_OFF_T_LEN  (sizeof("-9223372036854775808") - 1)
#define NGX_OFF_T_FMT  "I64d"
#define NGX_TIME_T_LEN  (sizeof("-9223372036854775808") - 1)
#define NGX_TIME_T_FMT  "I64d"
#define NGX_TIME_T_SIZE  8

#define NGX_HAVE_STRERROR_R  0
#define NGX_HAVE_LOCALTIME_R  0
#define NGX_HAVE_GMTIME_R  0
#define NGX_HAVE_GETTIMEOFDAY  0
#define NGX_HAVE_POLL  0
#define NGX_HAVE_SELECT  1

#define NGX_HAVE_MAP_ANON  0
#define NGX_HAVE_MAP_DEVZERO  0
#define NGX_HAVE_SYSVSHM  0
#define NGX_HAVE_POSIX_SEM  0
#define NGX_HAVE_MSGHDR_MSG_CONTROL  0

#define NGX_HAVE_FIONBIO  0
#define NGX_HAVE_FIONREAD  0
#define NGX_HAVE_SOCKET_CLOEXEC  0

#define NGX_HAVE_LITTLE_ENDIAN  1

#define NGX_MAX_UINT32_VALUE  (uint32_t) 0xffffffff
#define NGX_MAX_INT32_VALUE   0x7fffffff

#define NGX_HAVE_OPENSSL_MD5_H  0
#define NGX_HAVE_MD5_H  0
#define NGX_OPENSSL_MD5  1
#define NGX_HAVE_OPENSSL_SHA1_H  0
#define NGX_OPENSSL_SHA1  1

#define NGX_HAVE_OPENSSL  1
#define NGX_OPENSSL  1
#define NGX_SSL  1

#define NGX_HAVE_PCRE2  1
#define NGX_PCRE  1
#define NGX_PCRE2  1

#define NGX_ZLIB  1

#define NGX_HAVE_INET6  1
#define NGX_HAVE_ADDRINFO  1

#define NGX_HAVE_VARIADIC_MACROS  1

#define NGX_HTTP  1
#define NGX_CRYPT 1
#define NGX_HAVE_HTTP_CACHE  1
#define NGX_HTTP_CACHE  1
#define NGX_HTTP_GZIP  1
#define NGX_HTTP_SSI  1
#define NGX_HTTP_X_FORWARDED_FOR  1
#define NGX_HTTP_REALIP  1
#define NGX_HTTP_HEADERS  1
#define NGX_HTTP_DAV  0
#define NGX_HTTP_FLV  0
#define NGX_HTTP_MP4  0
#define NGX_HTTP_GZIP_STATIC  1
#define NGX_HTTP_AUTH_BASIC  1
#define NGX_HTTP_RANDOM_INDEX  0
#define NGX_HTTP_ACCESS  1
#define NGX_HTTP_AUTOINDEX  1
#define NGX_HTTP_STATUS  1
#define NGX_HTTP_CHARSET_FILTER  1
#define NGX_HTTP_USERID_FILTER  1
#define NGX_HTTP_AUTH_REQUEST  1
#define NGX_HTTP_LIMIT_CONN  1
#define NGX_HTTP_LIMIT_REQ  1
#define NGX_HTTP_UPSTREAM_ZONE  0
#define NGX_HTTP_UPSTREAM  1
#define NGX_HTTP_PROXY  1
#define NGX_HTTP_FASTCGI  1
#define NGX_HTTP_UWSGI  1
#define NGX_HTTP_SCGI  1
#define NGX_HTTP_MIRROR  1
#define NGX_HTTP_SPLIT_CLIENTS  1
#define NGX_HTTP_GEO  1
#define NGX_HTTP_MAP  1
#define NGX_HTTP_REFERER  1
#define NGX_HTTP_REWRITE  1
#define NGX_HTTP_BROWSER  1
#define NGX_HTTP_MEMCACHED  1
#define NGX_HTTP_EMPTY_GIF  1
#define NGX_HTTP_SSL  1
#define NGX_HTTP_V2  1
#define NGX_HTTP_V3  0

#define NGX_MAIL  0
#define NGX_STREAM  0

#define NGX_STAT_STUB  0

#define NGX_PREFIX  ""
#define NGX_CONF_PREFIX  "conf/"
#define NGX_SBIN_PATH  "nginx.exe"
#define NGX_CONF_PATH  "conf/nginx.conf"
#define NGX_PID_PATH   "logs/nginx.pid"
#define NGX_LOCK_PATH  "logs/nginx.lock"
#define NGX_ERROR_LOG_PATH  "logs/error.log"
#define NGX_HTTP_LOG_PATH   "logs/access.log"
#define NGX_HTTP_CLIENT_TEMP_PATH  "temp/client_body_temp"
#define NGX_HTTP_PROXY_TEMP_PATH   "temp/proxy_temp"
#define NGX_HTTP_FASTCGI_TEMP_PATH "temp/fastcgi_temp"
#define NGX_HTTP_UWSGI_TEMP_PATH   "temp/uwsgi_temp"
#define NGX_HTTP_SCGI_TEMP_PATH    "temp/scgi_temp"

#define NGX_SUPPRESS_WARN  1
#define NGX_WIN32  1

#define NGX_CPU_CACHE_LINE  64
