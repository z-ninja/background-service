#include <iostream>
#include <win/Service.h>

#define SERVICE_NAME "Bre Win Service Name"
void displayCheckPoints(const SERVICE_STATUS_PROCESS&st)
{

    std::cout << "Check Point: " << st.dwCheckPoint << std::endl;
    std::cout << "Wait Hint: " << st.dwWaitHint << std::endl;

}
int main(int argc,char*argv[])
{

    bre::win::ServiceManager smgr;
    bre::win::Service svc{SERVICE_NAME};

    for(int i=1; i<argc; i++)
    {
        std::string arg = argv[i];
        if(arg.compare("start") == 0|| arg.compare("stop") == 0 || arg.compare("install") == 0 || arg.compare("uninstall") == 0)
        {
            smgr.close();
            if(!smgr.open())
            {
                std::cout << "Open service manager failed with code: " << GetLastError() << std::endl;
                return 1;
            }
            break;
        }
        else if(arg.starts_with("--h") == 0)
        {

            std::cout << argv[0] << " install|start|stop|uninstall" << std::endl;
            std::cout << "install <list of argumets for starting service>" << std::endl;
            return 0;
        }
    }
    for(int i=1; i<argc; i++)
    {
        std::string arg = argv[i];

        if(arg.compare("service") == 0)
        {
            if(!svc.run([&]()->bre::platform::OS_SPECIFIC_INTEGER_TYPE
        {

            /// service main task execution
            /// return code
            return 0;
        },[&]()->void
        {
            /// on service stop request
            /// called before service main task return
        }))
            {
                /// Error running service
                std::cout << "Run service failed with code: " << GetLastError() << std::endl;
                return 1;
            }
            return 0;
        }
        else if(arg.compare("start") == 0)
        {
            if(!smgr.start(svc))
            {
                std::cout << "Start service failed with code: " << GetLastError() << std::endl;
                return 1;
            }
            std::cout << "Service started successfully" << std::endl;
        }
        else if(arg.compare("stop") == 0)
        {
            if(!smgr.stop(svc))
            {
                std::cout << "Start service failed with code: " << GetLastError() << std::endl;
                return 1;
            }
            std::cout << "Service stopped successfully" << std::endl;
        }
        else if(arg.compare("install") == 0)
        {

            std::vector<std::string> args{"service"};
            for(int c=i+1; c<argc; c++)
            {
                args.push_back(argv[c]);
            }
            if(!smgr.install(svc, args))
            {
                std::cout << "Install service failed with code: " << GetLastError() << std::endl;
                return 1;
            }
            std::cout << "Service '" << svc.getName() << "' installed successfully" << std::endl;
        }
        else if(arg.compare("uninstall") == 0)
        {
            if(!smgr.uninstall(svc))
            {
                std::cout << "Uninstall service failed with code: " << GetLastError() << std::endl;
                return 1;
            }
            std::cout << "Service '" << svc.getName() <<"' uninstalled successfully" << std::endl;

        }
        else if(arg.compare("status") == 0)
        {

            if(!smgr.status(svc))
            {
                std::cout << "Status service failed with code: " << GetLastError() << std::endl;
                return 1;
            }
            const SERVICE_STATUS_PROCESS& st = svc.proc_status();

            switch(st.dwCurrentState)
            {
            case SERVICE_START_PENDING:
            {
                std::cout << "Service start pending" << std::endl;
                displayCheckPoints(st);

            }
            break;
            case SERVICE_RUNNING:
            {
                std::cout << "Service is running\n" << std::endl;
                displayCheckPoints(st);
            }
            break;
            case SERVICE_STOP_PENDING:
            {
                std::cout << "Service stop pending\n" << std::endl;
                displayCheckPoints(st);
            }
            break;
            case SERVICE_STOPPED:
            {
                std::cout << "Service inactive" << std::endl;
                std::cout << "Exit Code: " << st.dwWin32ExitCode << std::endl;
                displayCheckPoints(st);

            }
            break;
            default:
            {
                std::cout << "Unknown service satus code: " << st.dwCurrentState << std::endl;
                break;
            }


            }


        }


    }


    return 0;
}
