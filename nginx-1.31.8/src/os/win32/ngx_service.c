/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_service.h>
#include <ngx_process_cycle.h>

#include <windows.h>

#define NGX_SERVICE_NAME "nginx"
#define NGX_SERVICE_DISPLAY_NAME "Nginx Web Server"

static SERVICE_STATUS_HANDLE ngx_service_status_handle;
static SERVICE_STATUS ngx_service_status;

static int ngx_service_argc;
static char **ngx_service_argv;

ngx_uint_t ngx_is_service = 0;

static DWORD WINAPI ngx_service_handler(DWORD control, DWORD event_type, LPVOID event_data, LPVOID context);
static VOID WINAPI ngx_service_main(DWORD argc, LPTSTR *argv);
static void ngx_service_get_custom_name(int argc, char **argv, char *name_buf, size_t name_len, const char *default_name);

static SERVICE_TABLE_ENTRY ngx_service_table[] = {
    { NGX_SERVICE_NAME, ngx_service_main },
    { NULL, NULL }
};


ngx_int_t
ngx_service_dispatcher(int argc, char **argv)
{
    ngx_service_argc = argc;
    ngx_service_argv = argv;
    ngx_is_service = 1;

    if (!StartServiceCtrlDispatcher(ngx_service_table)) {
        /* This will fail if not run by SCM */
        return NGX_ERROR;
    }

    return NGX_OK;
}


static VOID WINAPI
ngx_service_main(DWORD argc, LPTSTR *argv)
{
    int rc;

    freopen("c:\\nginx_x64\\logs\\service_error.log", "w", stderr);
    freopen("c:\\nginx_x64\\logs\\service_out.log", "w", stdout);

    /* SCM passes the service name as argv[0] */
    ngx_service_status_handle = RegisterServiceCtrlHandlerEx(
        (argc > 0 && argv && argv[0]) ? argv[0] : NGX_SERVICE_NAME,
        ngx_service_handler,
        NULL
    );

    if (ngx_service_status_handle == NULL) {
        return;
    }

    ngx_memzero(&ngx_service_status, sizeof(SERVICE_STATUS));

    ngx_service_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    ngx_service_status.dwCurrentState = SERVICE_START_PENDING;
    ngx_service_status.dwControlsAccepted = 0;
    ngx_service_status.dwWin32ExitCode = NO_ERROR;
    ngx_service_status.dwCheckPoint = 1;
    ngx_service_status.dwWaitHint = 10000;

    SetServiceStatus(ngx_service_status_handle, &ngx_service_status);

    /*
     * Run nginx main logic.
     * This does not return until the master lifecycle has completed.
     */
    rc = ngx_main_run(ngx_service_argc, ngx_service_argv);

    ngx_service_status.dwCurrentState = SERVICE_STOPPED;
    ngx_service_status.dwControlsAccepted = 0;
    ngx_service_status.dwWin32ExitCode = rc == 0 ? NO_ERROR : ERROR_SERVICE_SPECIFIC_ERROR;
    ngx_service_status.dwCheckPoint = 0;
    ngx_service_status.dwWaitHint = 0;

    SetServiceStatus(ngx_service_status_handle, &ngx_service_status);
}


static DWORD WINAPI
ngx_service_handler(DWORD control, DWORD event_type, LPVOID event_data, LPVOID context)
{
    switch (control) {

    case SERVICE_CONTROL_STOP:
        ngx_service_report_stopping(30000);
        ngx_process_service_stop();
        return NO_ERROR;

    case SERVICE_CONTROL_PRESHUTDOWN:
        ngx_service_report_stopping(30000);
        ngx_process_service_quit();
        return NO_ERROR;

    case SERVICE_CONTROL_SHUTDOWN:
        ngx_service_report_stopping(30000);
        ngx_process_service_quit();
        return NO_ERROR;

    case SERVICE_CONTROL_INTERROGATE:
        return NO_ERROR;

    case SERVICE_CONTROL_PARAMCHANGE:
        ngx_process_service_reload();
        return NO_ERROR;

    default:
        return ERROR_CALL_NOT_IMPLEMENTED;
    }
}


void
ngx_service_report_running(void)
{
    if (ngx_service_status_handle == NULL) {
        return;
    }

    ngx_service_status.dwCurrentState = SERVICE_RUNNING;
#ifdef SERVICE_ACCEPT_PRESHUTDOWN
    ngx_service_status.dwControlsAccepted = SERVICE_ACCEPT_STOP
                                          | SERVICE_ACCEPT_SHUTDOWN
                                          | SERVICE_ACCEPT_PRESHUTDOWN
                                          | SERVICE_ACCEPT_PARAMCHANGE;
#else
    ngx_service_status.dwControlsAccepted = SERVICE_ACCEPT_STOP
                                          | SERVICE_ACCEPT_SHUTDOWN
                                          | SERVICE_ACCEPT_PARAMCHANGE;
#endif
    ngx_service_status.dwWin32ExitCode = NO_ERROR;
    ngx_service_status.dwServiceSpecificExitCode = 0;
    ngx_service_status.dwCheckPoint = 0;
    ngx_service_status.dwWaitHint = 0;

    SetServiceStatus(ngx_service_status_handle, &ngx_service_status);
}


void
ngx_service_report_stopping(DWORD wait_hint)
{
    if (ngx_service_status_handle == NULL) {
        return;
    }

    ngx_service_status.dwCurrentState = SERVICE_STOP_PENDING;
    ngx_service_status.dwControlsAccepted = 0;
    ngx_service_status.dwCheckPoint++;
    ngx_service_status.dwWaitHint = wait_hint;

    SetServiceStatus(ngx_service_status_handle, &ngx_service_status);
}


void
ngx_service_report_stopped(DWORD error)
{
    if (ngx_service_status_handle == NULL) {
        return;
    }

    ngx_service_status.dwCurrentState = SERVICE_STOPPED;
    ngx_service_status.dwControlsAccepted = 0;
    ngx_service_status.dwWin32ExitCode = error;
    ngx_service_status.dwCheckPoint = 0;
    ngx_service_status.dwWaitHint = 0;

    SetServiceStatus(ngx_service_status_handle, &ngx_service_status);
}


/* CLI parsing helpers */

static char *
ngx_service_get_arg_value(int argc, char **argv, const char *name)
{
    int i;
    for (i = 1; i < argc - 1; i++) {
        if (ngx_strcmp(argv[i], name) == 0) {
            return argv[i + 1];
        }
    }
    return NULL;
}

static void
ngx_service_get_custom_name(int argc, char **argv, char *name_buf, size_t name_len, const char *default_name)
{
    char *val = ngx_service_get_arg_value(argc, argv, "--service-name");
    if (val) {
        ngx_snprintf((u_char *)name_buf, name_len, "%s%Z", val);
    } else {
        ngx_snprintf((u_char *)name_buf, name_len, "%s%Z", default_name);
    }
}

int
ngx_service_install(int argc, char **argv)
{
    SC_HANDLE  schSCManager;
    SC_HANDLE  schService;
    char       szPath[MAX_PATH];
    char       szCmd[1024];
    char       svcName[256];
    char       svcDisp[256];
    char      *account;
    char      *password;
    char      *val;
    int        i;
    u_char    *p;
#ifdef SERVICE_CONFIG_PRESHUTDOWN_INFO
    SERVICE_PRESHUTDOWN_INFO info;
#endif
    SERVICE_FAILURE_ACTIONS  failure;
    SC_ACTION                actions[3];

    ngx_service_get_custom_name(argc, argv, svcName, sizeof(svcName), NGX_SERVICE_NAME);

    val = ngx_service_get_arg_value(argc, argv, "--service-display-name");
    if (val) {
        ngx_snprintf((u_char *)svcDisp, sizeof(svcDisp), "%s%Z", val);
    } else {
        ngx_snprintf((u_char *)svcDisp, sizeof(svcDisp), "%s%Z", NGX_SERVICE_DISPLAY_NAME);
    }

    account = ngx_service_get_arg_value(argc, argv, "--service-account");
    password = ngx_service_get_arg_value(argc, argv, "--service-password");

    if (GetModuleFileName(NULL, szPath, MAX_PATH) == 0) {
        printf("GetModuleFileName failed (%d)\n", GetLastError());
        return 1;
    }

    p = ngx_snprintf((u_char *) szCmd, sizeof(szCmd), "\"%s\" -service", szPath);

    /* Append remaining arguments to szCmd */
    for (i = 2; i < argc; i++) {
        if (ngx_strcmp(argv[i], "--service-name") == 0 ||
            ngx_strcmp(argv[i], "--service-display-name") == 0 ||
            ngx_strcmp(argv[i], "--service-account") == 0 ||
            ngx_strcmp(argv[i], "--service-password") == 0)
        {
            i++; /* skip value */
            continue;
        }
        p = ngx_snprintf(p, sizeof(szCmd) - (p - (u_char *)szCmd), " \"%s\"", argv[i]);
    }
    *p = '\0';

    schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (schSCManager == NULL) {
        printf("OpenSCManager failed (%d)\n", GetLastError());
        return 1;
    }

    schService = CreateService(
        schSCManager,              /* SCM database */
        svcName,                   /* name of service */
        svcDisp,                   /* service name to display */
        SERVICE_ALL_ACCESS,        /* desired access */
        SERVICE_WIN32_OWN_PROCESS, /* service type */
        SERVICE_AUTO_START,        /* start type */
        SERVICE_ERROR_NORMAL,      /* error control type */
        szCmd,                     /* path to service's binary */
        NULL,                      /* no load ordering group */
        NULL,                      /* no tag identifier */
        NULL,                      /* no dependencies */
        account,                   /* account */
        password);                 /* password */

    if (schService == NULL) {
        printf("CreateService failed (%d)\n", GetLastError());
        CloseServiceHandle(schSCManager);
        return 1;
    }

    printf("Service '%s' installed successfully\n", svcName);

#ifdef SERVICE_CONFIG_PRESHUTDOWN_INFO
    info.dwPreshutdownTimeout = 30000; /* 30 seconds */
    if (!ChangeServiceConfig2(schService, SERVICE_CONFIG_PRESHUTDOWN_INFO, &info)) {
        printf("ChangeServiceConfig2 (PRESHUTDOWN) failed (%d)\n", GetLastError());
    }
#endif

    /* Setup Failure Actions */
    actions[0].Type = SC_ACTION_RESTART;
    actions[0].Delay = 5000;
    actions[1].Type = SC_ACTION_RESTART;
    actions[1].Delay = 10000;
    actions[2].Type = SC_ACTION_RESTART;
    actions[2].Delay = 30000;

    failure.dwResetPeriod = 86400; /* 1 day */
    failure.lpRebootMsg = NULL;
    failure.lpCommand = NULL;
    failure.cActions = 3;
    failure.lpsaActions = actions;

    if (!ChangeServiceConfig2(schService, SERVICE_CONFIG_FAILURE_ACTIONS, &failure)) {
        printf("ChangeServiceConfig2 (FAILURE_ACTIONS) failed (%d)\n", GetLastError());
    }

    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);

    return 0;
}

int
ngx_service_uninstall(int argc, char **argv)
{
    SC_HANDLE  schSCManager;
    SC_HANDLE  schService;
    char       svcName[256];

    ngx_service_get_custom_name(argc, argv, svcName, sizeof(svcName), NGX_SERVICE_NAME);

    schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (schSCManager == NULL) {
        printf("OpenSCManager failed (%d)\n", GetLastError());
        return 1;
    }

    schService = OpenService(schSCManager, svcName, DELETE);
    if (schService == NULL) {
        printf("OpenService failed (%d)\n", GetLastError());
        CloseServiceHandle(schSCManager);
        return 1;
    }

    if (!DeleteService(schService)) {
        printf("DeleteService failed (%d)\n", GetLastError());
        CloseServiceHandle(schService);
        CloseServiceHandle(schSCManager);
        return 1;
    }

    printf("Service '%s' uninstalled successfully\n", svcName);

    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);

    return 0;
}

int
ngx_service_start(int argc, char **argv)
{
    SC_HANDLE  schSCManager;
    SC_HANDLE  schService;
    char       svcName[256];

    ngx_service_get_custom_name(argc, argv, svcName, sizeof(svcName), NGX_SERVICE_NAME);

    schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (schSCManager == NULL) {
        printf("OpenSCManager failed (%d)\n", GetLastError());
        return 1;
    }

    schService = OpenService(schSCManager, svcName, SERVICE_START);
    if (schService == NULL) {
        printf("OpenService failed (%d)\n", GetLastError());
        CloseServiceHandle(schSCManager);
        return 1;
    }

    if (!StartService(schService, 0, NULL)) {
        printf("StartService failed (%d)\n", GetLastError());
        CloseServiceHandle(schService);
        CloseServiceHandle(schSCManager);
        return 1;
    }

    printf("Service '%s' started successfully\n", svcName);

    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);

    return 0;
}

int
ngx_service_stop(int argc, char **argv)
{
    SC_HANDLE  schSCManager;
    SC_HANDLE  schService;
    char       svcName[256];
    SERVICE_STATUS status;

    ngx_service_get_custom_name(argc, argv, svcName, sizeof(svcName), NGX_SERVICE_NAME);

    schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (schSCManager == NULL) {
        printf("OpenSCManager failed (%d)\n", GetLastError());
        return 1;
    }

    schService = OpenService(schSCManager, svcName, SERVICE_STOP);
    if (schService == NULL) {
        printf("OpenService failed (%d)\n", GetLastError());
        CloseServiceHandle(schSCManager);
        return 1;
    }

    if (!ControlService(schService, SERVICE_CONTROL_STOP, &status)) {
        printf("ControlService failed (%d)\n", GetLastError());
        CloseServiceHandle(schService);
        CloseServiceHandle(schSCManager);
        return 1;
    }

    printf("Service '%s' stop signal sent successfully\n", svcName);

    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);

    return 0;
}
