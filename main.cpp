#include "server.h"
int main(){
    Server server;
    server.start_listen();
    server.event_loop();
    return 0;
}