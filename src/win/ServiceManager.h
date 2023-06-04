#ifndef SERVICEMANAGER_H
#define SERVICEMANAGER_H
#include <windows.h>
#include <vector>
#include <string>

namespace bre
{
namespace platform {
using OS_SPECIFIC_INTEGER_TYPE = DWORD;
}
namespace win
{

class Service;
class ServiceManager
{
public:
    ServiceManager();
    ~ServiceManager();
    bool open(platform::OS_SPECIFIC_INTEGER_TYPE dwDesiredAccess = SC_MANAGER_ALL_ACCESS);
    bool install(Service&);
    bool install(Service&,std::vector<std::string>&);
    bool uninstall(Service&);
    bool status(Service&);
    bool start(Service&);
    bool stop(Service&);
    bool close();
protected:

private:
    SC_HANDLE schSCManager{nullptr};

};
} /// win
} /// bre

#endif // SERVICEMANAGER_H
