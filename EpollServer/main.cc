#include "PollServer.hpp"

int main(int argc, char *argv[]){
    if(2 != argc){
        std::cout << "Usage: " << argv[0] << " port" << std::endl;
        return 1;
    }

    uint16_t local_port = std::stoi(argv[1]);

    std::unique_ptr<PollServer> psvr = std::make_unique<PollServer>(local_port);
    psvr->Init();
    psvr->Loop();

    return 0;
}