#include "Service.h"
#include <sstream>
#include <iomanip>

namespace bre
{
namespace win
{
ServiceManager::ServiceManager()
{
    //ctor
}

ServiceManager::~ServiceManager()
{
    //dtor
    close();
}

bool ServiceManager::status(Service&svc)
{

    SERVICE_STATUS_PROCESS& ssp = svc.m_ssp;
    DWORD dwBytesNeeded = 0;
    SC_HANDLE schService = OpenService(
                               schSCManager,         // SCM database
                               svc.m_info.m_name.c_str(),            // name of service
                               SERVICE_QUERY_STATUS |
                               SERVICE_ENUMERATE_DEPENDENTS);

    if (schService == nullptr)
    {
        return false;
    }

    // Make sure the service is not already stopped.

    if ( !QueryServiceStatusEx(
                schService,
                SC_STATUS_PROCESS_INFO,
                (LPBYTE)&ssp,
                sizeof(SERVICE_STATUS_PROCESS),
                &dwBytesNeeded ) )
    {
        CloseServiceHandle(schService);
        return false;
    }

    CloseServiceHandle(schService);
    return true;
}
bool ServiceManager::close()
{
    if(schSCManager == nullptr)
    {
        return true;
    }
    auto ret = CloseServiceHandle(schSCManager);
    if(ret)
    {
        schSCManager = nullptr;
    }
    return ret;
}
bool ServiceManager::open(platform::OS_SPECIFIC_INTEGER_TYPE dwDesiredAccess)
{
    if(schSCManager)
    {
        return true;
    }
    schSCManager = OpenSCManager(
                       nullptr,                    // local computer
                       nullptr,                    // ServicesActive database
                       dwDesiredAccess);  // full access rights
    if (nullptr == schSCManager)
    {
        return false;
    }
    return true;
}

bool ServiceManager::uninstall(Service&svc)
{

    SC_HANDLE schService = OpenService(
                               schSCManager,       // SCM database
                               svc.m_info.m_name.c_str(),          // name of service
                               DELETE);            // need delete access

    if (schService == nullptr)
    {
        return false;
    }

    // Delete the service.

    if (! DeleteService(schService) )
    {
        CloseServiceHandle(schService);
        return false;
    }
    CloseServiceHandle(schService);

    return true;
}

bool ServiceManager::install(Service&svc)
{
    std::vector<std::string>args;
    return install(svc,args);
}
bool ServiceManager::install(Service&svc,std::vector<std::string>&args)
{

    SC_HANDLE schService;
    TCHAR szUnquotedPath[MAX_PATH];

    if( !GetModuleFileName( nullptr, szUnquotedPath, MAX_PATH ) )
    {
        return false;
    }

    std::stringstream stream;
    stream << std::quoted(szUnquotedPath);
    for(auto it = args.begin(); it != args.end(); it++)
    {
        stream << " " << (*it);
    }
    std::string szPath = stream.str();


    schService = CreateService(
                     schSCManager,              // SCM database
                     svc.m_info.m_name.c_str(),                   // name of service
                     svc.m_info.m_name.c_str(),                   // service name to display
                     svc.m_info.m_DesiredAccess,        // desired access
                     svc.m_info.m_ServiceType,// | SERVICE_INTERACTIVE_PROCESS, // service type
                     svc.m_info.m_StartType,      // start type
                     svc.m_info.m_ErrorControl,      // error control type
                     szPath.c_str(),                 // path to service's binary
                     svc.m_info.m_LoadOrderGroup,                      // no load ordering group
                     ((svc.m_info.m_LoadOrderGroup == nullptr)?nullptr:&svc.m_info.m_TagId),                      // no tag identifier
                     svc.m_info.m_Dependencies,                      // no dependencies
                     svc.m_info.m_ServiceStartName,                      // LocalSystem account
                     svc.m_info.m_Password);                     // no password

    if (schService == nullptr)
    {
        return false;
    }

    CloseServiceHandle(schService);

    return true;
}

bool ServiceManager::start(Service&svc)
{

    SERVICE_STATUS_PROCESS& ssStatus = svc.m_ssp;
    DWORD dwOldCheckPoint;
    DWORD dwStartTickCount;
    DWORD dwWaitTime;
    DWORD dwBytesNeeded;

    SC_HANDLE schService = OpenService(
                               schSCManager,         // SCM database
                               svc.m_info.m_name.c_str(),            // name of service
                               SERVICE_ALL_ACCESS);  // full access

    if (schService == nullptr)
    {
        return false;
    }

    // Check the status in case the service is not stopped.

    if (!QueryServiceStatusEx(
                schService,                     // handle to service
                SC_STATUS_PROCESS_INFO,         // information level
                (LPBYTE) &ssStatus,             // address of structure
                sizeof(SERVICE_STATUS_PROCESS), // size of structure
                &dwBytesNeeded ) )              // size needed if buffer is too small
    {
        CloseServiceHandle(schService);
        return false;
    }

    // Check if the service is already running. It would be possible
    // to stop the service here, but for simplicity this example just returns.

    if(ssStatus.dwCurrentState != SERVICE_STOPPED && ssStatus.dwCurrentState != SERVICE_STOP_PENDING)
    {
        CloseServiceHandle(schService);
        return true;
    }

    // Save the tick count and initial checkpoint.

    dwStartTickCount = GetTickCount();
    dwOldCheckPoint = ssStatus.dwCheckPoint;

    // Wait for the service to stop before attempting to start it.

    while (ssStatus.dwCurrentState == SERVICE_STOP_PENDING)
    {
        // Do not wait longer than the wait hint. A good interval is
        // one-tenth of the wait hint but not less than 1 second
        // and not more than 10 seconds.

        dwWaitTime = ssStatus.dwWaitHint / 10;

        if( dwWaitTime < 1000 )
            dwWaitTime = 1000;
        else if ( dwWaitTime > 10000 )
            dwWaitTime = 10000;

        Sleep( dwWaitTime );

        // Check the status until the service is no longer stop pending.

        if (!QueryServiceStatusEx(
                    schService,                     // handle to service
                    SC_STATUS_PROCESS_INFO,         // information level
                    (LPBYTE) &ssStatus,             // address of structure
                    sizeof(SERVICE_STATUS_PROCESS), // size of structure
                    &dwBytesNeeded ) )              // size needed if buffer is too small
        {
            CloseServiceHandle(schService);
            return false;
        }

        if ( ssStatus.dwCheckPoint > dwOldCheckPoint )
        {
            // Continue to wait and check.

            dwStartTickCount = GetTickCount();
            dwOldCheckPoint = ssStatus.dwCheckPoint;
        }
        else
        {
            if(GetTickCount()-dwStartTickCount > ssStatus.dwWaitHint)
            {
                CloseServiceHandle(schService);
                return false;
            }
        }
    }

    // Attempt to start the service.
    // const char* args[] = { {svc.m_info.m_name.c_str()} };
    if (!StartService(
                schService,  // handle to service
                0,           // number of arguments
                nullptr) )      // no arguments
    {
        CloseServiceHandle(schService);
        return false;
    }

    // Check the status until the service is no longer start pending.

    if (!QueryServiceStatusEx(
                schService,                     // handle to service
                SC_STATUS_PROCESS_INFO,         // info level
                (LPBYTE) &ssStatus,             // address of structure
                sizeof(SERVICE_STATUS_PROCESS), // size of structure
                &dwBytesNeeded ) )              // if buffer too small
    {
        CloseServiceHandle(schService);
        return false;
    }

    // Save the tick count and initial checkpoint.

    dwStartTickCount = GetTickCount();
    dwOldCheckPoint = ssStatus.dwCheckPoint;

    while (ssStatus.dwCurrentState == SERVICE_START_PENDING)
    {
        // Do not wait longer than the wait hint. A good interval is
        // one-tenth the wait hint, but no less than 1 second and no
        // more than 10 seconds.

        dwWaitTime = ssStatus.dwWaitHint / 10;

        if( dwWaitTime < 1000 )
            dwWaitTime = 1000;
        else if ( dwWaitTime > 10000 )
            dwWaitTime = 10000;

        Sleep( dwWaitTime );

        // Check the status again.

        if (!QueryServiceStatusEx(
                    schService,             // handle to service
                    SC_STATUS_PROCESS_INFO, // info level
                    (LPBYTE) &ssStatus,             // address of structure
                    sizeof(SERVICE_STATUS_PROCESS), // size of structure
                    &dwBytesNeeded ) )              // if buffer too small
        {
            CloseServiceHandle(schService);
            return false;
        }

        if ( ssStatus.dwCheckPoint > dwOldCheckPoint )
        {
            // Continue to wait and check.

            dwStartTickCount = GetTickCount();
            dwOldCheckPoint = ssStatus.dwCheckPoint;
        }
        else
        {
            if(GetTickCount()-dwStartTickCount > ssStatus.dwWaitHint)
            {
                // No progress made within the wait hint.
                break;
            }
        }
    }

    // Determine whether the service is running.
    CloseServiceHandle(schService);
    if (ssStatus.dwCurrentState == SERVICE_RUNNING)
    {
        return true;
    }


    return false;
}

bool ServiceManager::stop(Service&svc)
{

    SERVICE_STATUS_PROCESS& ssp = svc.m_ssp;
    DWORD dwStartTime = GetTickCount();
    DWORD dwBytesNeeded;
    DWORD dwTimeout = 30000; // 30-second time-out
    DWORD dwWaitTime;

    SC_HANDLE schService = OpenService(
                               schSCManager,         // SCM database
                               svc.m_info.m_name.c_str(),            // name of service
                               SERVICE_STOP |
                               SERVICE_QUERY_STATUS |
                               SERVICE_ENUMERATE_DEPENDENTS);

    if (schService == nullptr)
    {
        return false;
    }

    // Make sure the service is not already stopped.

    if ( !QueryServiceStatusEx(
                schService,
                SC_STATUS_PROCESS_INFO,
                (LPBYTE)&ssp,
                sizeof(SERVICE_STATUS_PROCESS),
                &dwBytesNeeded ) )
    {
        CloseServiceHandle(schService);
        return false;
    }

    if ( ssp.dwCurrentState == SERVICE_STOPPED )
    {
        CloseServiceHandle(schService);
        return true;
    }

    // If a stop is pending, wait for it.

    while ( ssp.dwCurrentState == SERVICE_STOP_PENDING )
    {

        // Do not wait longer than the wait hint. A good interval is
        // one-tenth of the wait hint but not less than 1 second
        // and not more than 10 seconds.

        dwWaitTime = ssp.dwWaitHint / 10;

        if( dwWaitTime < 1000 )
            dwWaitTime = 1000;
        else if ( dwWaitTime > 10000 )
            dwWaitTime = 10000;

        Sleep( dwWaitTime );

        if ( !QueryServiceStatusEx(
                    schService,
                    SC_STATUS_PROCESS_INFO,
                    (LPBYTE)&ssp,
                    sizeof(SERVICE_STATUS_PROCESS),
                    &dwBytesNeeded ) )
        {
            CloseServiceHandle(schService);
            return false;
        }

        if ( ssp.dwCurrentState == SERVICE_STOPPED )
        {
            CloseServiceHandle(schService);
            return true;
        }

        if ( GetTickCount() - dwStartTime > dwTimeout )
        {
            CloseServiceHandle(schService);
            return false;
        }
    }

    // If the service is running, dependencies must be stopped first.

    //StopDependentServices();

    // Send a stop code to the service.

    if ( !ControlService(
                schService,
                SERVICE_CONTROL_STOP,
                (LPSERVICE_STATUS) &ssp ) )
    {
        CloseServiceHandle(schService);
        return false;
    }

    // Wait for the service to stop.

    while ( ssp.dwCurrentState != SERVICE_STOPPED )
    {
        Sleep( ssp.dwWaitHint );
        if ( !QueryServiceStatusEx(
                    schService,
                    SC_STATUS_PROCESS_INFO,
                    (LPBYTE)&ssp,
                    sizeof(SERVICE_STATUS_PROCESS),
                    &dwBytesNeeded ) )
        {
            CloseServiceHandle(schService);
            return false;
        }
        if ( ssp.dwCurrentState == SERVICE_STOPPED )
        {
            break;
        }
        if ( GetTickCount() - dwStartTime > dwTimeout )
        {
            break;
        }
    }

    CloseServiceHandle(schService);
    if ( ssp.dwCurrentState == SERVICE_STOPPED )
    {
        return true;
    }


    return false;

}
} /// win
} ///bre
