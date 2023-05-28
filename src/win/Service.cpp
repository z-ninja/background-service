#include "Service.h"
#include <map>

namespace bre
{
namespace win
{
Service::Service(std::string name):
    m_info{name},m_ssp{}
{

}
Service::Service(ServiceInfo&n):m_info{n},m_ssp{}
{

}
Service::~Service()
{
    //dtor
}
DWORD WINAPI Service::ServiceCtrlHandler (DWORD dwControl,DWORD /*dwEventType*/,LPVOID /*lpEventData*/,LPVOID lpContext)
{

    switch (dwControl)
    {
    case SERVICE_CONTROL_STOP :
    {

        Service*svc = reinterpret_cast<Service*>(lpContext);
        svc->stop();
    }
    return NO_ERROR;
    default:
    {
        break;
    }
    }

    return ERROR_CALL_NOT_IMPLEMENTED;

}
namespace
{
std::map<std::string,Service*> Services;

}
VOID WINAPI Service::ServiceMain (DWORD argc, char *argv[])
{
    auto it = Services.find(argv[0]);
    if(it != Services.end())
    {
        it->second->svcMain(argc,argv);
    }
}
bool Service::svcMain(DWORD,char**)
{

    m_handle = RegisterServiceCtrlHandlerEx (m_info.m_name.c_str(), Service::ServiceCtrlHandler,reinterpret_cast<LPVOID>(this));
    if (m_handle == nullptr)
    {
        // report error event
        return false;
    }

    ZeroMemory (&m_ss, sizeof (m_ss));
    m_ss.dwServiceType = m_info.m_ServiceType;
    m_ss.dwControlsAccepted = 0;
    m_ss.dwCurrentState = SERVICE_START_PENDING;
    m_ss.dwWin32ExitCode = 0;
    m_ss.dwServiceSpecificExitCode = 0;
    m_ss.dwCheckPoint = 0;

    if (SetServiceStatus (m_handle, &m_ss) == false)
    {
        return false;
    }


    m_ss.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    m_ss.dwCurrentState = SERVICE_RUNNING;
    m_ss.dwWin32ExitCode = 0;
    m_ss.dwCheckPoint = 0;

    if (SetServiceStatus (m_handle, &m_ss) == false)
    {
        return false;
    }

    DWORD exit_code = m_cb(); /// call task here

    m_ss.dwWin32ExitCode = exit_code;
    m_ss.dwControlsAccepted = 0;
    m_ss.dwCurrentState = SERVICE_STOPPED;
    m_ss.dwCheckPoint = 3;
    m_cb = nullptr;
    if (SetServiceStatus (m_handle, &m_ss) == false)
    {

        return false;
    }


    return true;
}

bool Service::stop()
{

    if(m_ss.dwCurrentState != SERVICE_RUNNING)
    {
        return true;
    }

    m_cb_on_stop_request();
    m_ss.dwControlsAccepted = 0;
    m_ss.dwCurrentState = SERVICE_STOP_PENDING;
    m_ss.dwWin32ExitCode = 0;
    m_ss.dwCheckPoint = 4;
    m_cb_on_stop_request = nullptr;
    if (SetServiceStatus (m_handle, &m_ss) == false)
    {
        return false;
    }
    return true;
}
bool Service::run(std::function<DWORD()>&& cb,std::function<void()>&& cb_on_stop_request)
{
    m_cb = cb;
    m_cb_on_stop_request = cb_on_stop_request;
    if(Services.find(m_info.m_name) == Services.end())
    {
        Services.insert(std::pair<std::string,Service*>(m_info.m_name,this));
    }
    SERVICE_TABLE_ENTRY ServiceTable[] =
    {
        {m_info.m_name.data(), (LPSERVICE_MAIN_FUNCTION) Service::ServiceMain},
        {nullptr, nullptr}
    };

    if (StartServiceCtrlDispatcher (ServiceTable) == false)
    {
        return false;
    }

    return true;
}
} /// win
} /// bre
