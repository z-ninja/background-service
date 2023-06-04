#ifndef SERVICE_H
#define SERVICE_H
#include "ServiceManager.h"
#include <functional>

namespace bre
{
namespace win
{
struct ServiceInfo
{
    std::string m_name{};
    platform::OS_SPECIFIC_INTEGER_TYPE      m_DesiredAccess{SERVICE_ALL_ACCESS};
    platform::OS_SPECIFIC_INTEGER_TYPE      m_ServiceType{SERVICE_WIN32_OWN_PROCESS};
    platform::OS_SPECIFIC_INTEGER_TYPE      m_StartType{SERVICE_AUTO_START};
    platform::OS_SPECIFIC_INTEGER_TYPE      m_ErrorControl{SERVICE_ERROR_NORMAL};
    const char*m_LoadOrderGroup{nullptr};
    const char*m_Dependencies{nullptr};
    const char*m_ServiceStartName{nullptr};
    const char*m_Password{nullptr};
    platform::OS_SPECIFIC_INTEGER_TYPE      m_TagId{0};
};

class Service
{
public:
    explicit Service(std::string);
    explicit Service(ServiceInfo&);
    ~Service();
    void setName(std::string name) noexcept
    {
        m_info.m_name = name;
    }
    std::string getName() const noexcept
    {
        return m_info.m_name;
    }
    const SERVICE_STATUS_PROCESS& proc_status() noexcept
    {
        return m_ssp;
    }
    const SERVICE_STATUS&status() noexcept
    {
        return m_ss;
    }
    bool run(std::function<platform::OS_SPECIFIC_INTEGER_TYPE()>&&,std::function<void()>&&);

protected:

private:
    static DWORD WINAPI ServiceCtrlHandler (DWORD dwControl,DWORD dwEventType,LPVOID lpEventData,LPVOID lpContext);
    static VOID WINAPI ServiceMain (DWORD argc, char *argv[]);
    bool svcMain(DWORD argc,char* argv[]);
    bool stop();
    friend ServiceManager;
    ServiceInfo m_info;
    SERVICE_STATUS_PROCESS m_ssp;
    SERVICE_STATUS m_ss;
    SERVICE_STATUS_HANDLE m_handle;
    std::function<platform::OS_SPECIFIC_INTEGER_TYPE()> m_cb;
    std::function<void()> m_cb_on_stop_request;
};
} /// win
} /// bre

#endif // SERVICE_H
