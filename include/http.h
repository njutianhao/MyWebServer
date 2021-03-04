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
#include<thread>
#include<mutex>
#define HTTP_BUFF_SIZE 1024
#define FILE_PATH_SIZE 256
class HttpConnection{
private:
enum ParseState{
        PARSE_REQUEST = 0,
        PARSE_HEADER,
        PARSE_CONTENT,
    };
    int fd;
    char buff[HTTP_BUFF_SIZE];
    char send_buff_header[HTTP_BUFF_SIZE];
    char send_buff_content[HTTP_BUFF_SIZE];
    int  recv_index;
    int parse_index;
    int current_content_size;
    int content_size;
    char file_path_prefix[FILE_PATH_SIZE];
    int file_path_prefix_length;
    off_t file_size;
    int file_fd;
    bool keepalive;
    iovec iov[2];
    std::string method;
    std::string url;
    std::string version;
    std::string content;
    ParseState state;
    std::map<std::string,std::string> headers;
    std::map<std::string,std::string> params;
    std::vector<std::string> split(char *,char,bool,bool);
public:
    std::mutex mutex;
    HttpConnection(int);
    void init();
    int run();
    int parse();
    void adjust_buff();
    int process();
    int send();
    void create_404_response();
    void create_400_response();
    void create_200_response();
};
#endif