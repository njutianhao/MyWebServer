#ifndef HTTP_H
#define HTTP_H
#include<sys/socket.h>
#include<string>
#include<string.h>
#include<map>
#include<memory.h>
#include<vector>
#include<regex>
#include<fcntl.h>
#include<unistd.h>
#include<sys/stat.h>
#include<sys/mman.h>
#include<sys/uio.h>
#include<error.h>
#include<pthread.h>
#define HTTP_BUFF_SIZE 2048
#define FILE_PATH_SIZE 2048
class HttpConnection{
private:
enum ParseState{
        PARSE_REQUEST = 0,
        PARSE_HEADER,
        PARSE_CONTENT,
    };
    int fd;
    char buff[HTTP_BUFF_SIZE];
    int  recv_index;
    int parse_index;
    int current_content_size;
    int content_size;
    char file_path_prefix[FILE_PATH_SIZE];
    int file_path_prefix_length;
    bool keepalive;
    std::string method;
    std::string url;
    std::string version;
    std::string content;
    ParseState state;
    std::map<std::string,std::string> headers;
    std::map<std::string,std::string> params;
    std::vector<std::string> split(char *,char,bool,bool);
public:
    pthread_mutex_t mutex;
    HttpConnection(int);
    void init();
    int run();
    int parse();
    void adjust_buff();
    int process();
    void send_404_response();
    void send_400_response();
    void send_200_response(int,struct stat);
};
#endif