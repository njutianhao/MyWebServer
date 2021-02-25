#ifndef HTTP_H
#define HTTP_H
#include<sys/socket.h>
#include<string>
#include<string.h>
#include<map>
#include<memory.h>
#include<vector>
#define HTTP_BUFF_SIZE 2048
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
    std::string method;
    std::string url;
    std::string version;
    ParseState state;
    std::map<std::string,std::string> headers;
    std::vector<std::string> split(char *,char);
public:
    HttpConnection(int);
    int read();
    int parse();
};
#endif