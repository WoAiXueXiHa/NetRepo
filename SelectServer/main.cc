#include "SelectServer.hpp"

// ./select_server 8080
int main(int argc, char *argv[]){
    if(2 != argc){
        std::cout << "Usage: " << argv[0] << " port" << std::endl;
        return 1;
    }

    uint16_t local_port = std::stoi(argv[1]);

    std::unique_ptr<SelectServer> ssvr = std::make_unique<SelectServer>(local_port);
    ssvr->Init();
    ssvr->Loop();

    return 0;
}