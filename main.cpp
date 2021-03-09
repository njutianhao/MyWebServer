#include"server.h"
#include<unistd.h>
int main(int argc,char *argv[]){
    char ch = 0;
    Server server;
    while((ch = getopt(argc,argv,":p") != -1)){
        switch(ch)
        {
            case 'p':server.set_port(atoi(optarg));
        }
    }
    server.start_listen();
    server.event_loop();
    return 0;
}