#include "WinService.h"

#define SERVICE_NAME L"Test Service"
#define SERVICE_DESC L"Test Service DESC"

namespace ds {

wchar_t WinService::service_name_[30] = SERVICE_NAME;
SERVICE_STATUS_HANDLE WinService::ssh_ = { 0 };


int WinService::ServiceRun()
{
  SERVICE_TABLE_ENTRY ste[] = { { WinService::service_name_, WinService::ServiceMain }, { NULL, NULL} };

  auto ret = StartServiceCtrlDispatcher(ste);
  if (!ret)
  {
    auto ec = GetLastError();
    if (ec == ERROR_FAILED_SERVICE_CONTROLLER_CONNECT)
    {
      // 콘솔로 실행(서비스로 실행되지 않았음)
      printf("========== start in console mode ==========");
      auto ret = SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);
      if (!ret)
      {
        printf("SetConsoleCtrlHandler failed");
      }

      //ds_ = std::make_shared<DesktopStreamer>();
      //ds_->Run(false);

      return 0;
    }
    printf("StartServiceCtrlDispatcher failed. ec : 0x%x", ec);
    return -1;
  }

  return 0;
}

void WinService::ServiceMain(DWORD argc, LPWSTR* argv)
{
  printf("========== start in service mode ==========");
  ssh_ = RegisterServiceCtrlHandlerEx(service_name_, WinService::ServiceHandler, NULL);

  if (!ssh_)
  {
    printf("RegisterServiceCtrlHandlerEx failed.");
    return;
  }

  SetServiceState(SERVICE_START_PENDING);
  SetServiceState(SERVICE_RUNNING);


}

DWORD WinService::ServiceHandler(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext)
{
  switch (dwControl)
  {
  case SERVICE_CONTROL_STOP:
    SetServiceState(SERVICE_STOP_PENDING);
    // 서비스를 멈춘다 (즉, 종료와 같은 의미)
    printf("SERVICE_CONTROL_STOP");
    //ds_->Stop();
    //ds_.reset();
    SetServiceState(SERVICE_STOPPED);
    break;
  case SERVICE_CONTROL_SESSIONCHANGE:
    onSessionChange(dwEventType, lpEventData);
    break;
  default:
    printf("DesktopStreamerService::ServiceHandler dwControl : 0x%x", dwControl);
    break;
  }

  return NO_ERROR;
}

void WinService::SetServiceState(DWORD dwStatus)
{
  SERVICE_STATUS ss = { 0 };
  ss.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
  ss.dwCurrentState = dwStatus;
  ss.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SESSIONCHANGE;

  auto ret = SetServiceStatus(ssh_, &ss);
  if (!ret)
  {
    auto ec = GetLastError();
    printf("SetServiceStatus failed. ec : 0x%x", ec);
    std::terminate();
  }
}

void WinService::onSessionChange(DWORD dwEventType, LPVOID lpEventData)
{
  switch (dwEventType)
  {
  case WTS_CONSOLE_CONNECT:     /*event_loop_->SessionChange();*/    printf("WTS_CONSOLE_CONNECT");          break;
  case WTS_CONSOLE_DISCONNECT:                                       printf("WTS_CONSOLE_DISCONNECT");       break;
  case WTS_REMOTE_CONNECT:                                           printf("WTS_REMOTE_CONNECT");           break;
  case WTS_REMOTE_DISCONNECT:                                        printf("WTS_REMOTE_DISCONNECT");        break;
  case WTS_SESSION_LOGON:                                            printf("WTS_SESSION_LOGON");            break;
  case WTS_SESSION_LOGOFF:                                           printf("WTS_SESSION_LOGOFF");           break;
  case WTS_SESSION_LOCK:                                             printf("WTS_SESSION_LOCK");             break;
  case WTS_SESSION_UNLOCK:                                           printf("WTS_SESSION_UNLOCK");           break;
  case WTS_SESSION_REMOTE_CONTROL:                                   printf("WTS_SESSION_REMOTE_CONTROL");   break;
  default:                                                           printf("default");                      break;
  }
}

BOOL WinService::ConsoleCtrlHandler(DWORD dwCtrlType)
{
  switch (dwCtrlType)
  {
  case CTRL_CLOSE_EVENT:    /*ds_->Stop();*/  return TRUE;  // Closing the console window
  case CTRL_C_EVENT:                                    // Ctrl+C
  case CTRL_BREAK_EVENT:                                // Ctrl+Break
  case CTRL_LOGOFF_EVENT:                               // User logs off. Passed only to services!
  case CTRL_SHUTDOWN_EVENT:                             // System is shutting down. Passed only to services!
  default:                                return FALSE;
  }

  return FALSE;
}

};  // namespace ds