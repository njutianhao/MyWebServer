#include"http.h"
HttpConnection::HttpConnection(int f){
    fd = f;
    recv_index = 0;
    parse_index = 0;
    state = PARSE_REQUEST;
    memset(buff,0,HTTP_BUFF_SIZE);
}

std::vector<string> HttpConnection::split(char *str,char delim){
    std::vector<std::string> vec;
    char *head = str;
    int flag = false;
    while(*str != '\0')
    {
        if(*str == delim && flag == false)
        {
            vec.push_back(std::string(head,str - head));
            flag = true;
        }
        else if(flag == true && *str != delim)
        {
            flag = false;
            head = str;
        }
    }
    if(flag == false)
        vec.push_back(std::string(head,str-head));
    return vec;
}

int HttpConnection::read(){
    int ret;
    while(1)
    {
        ret = recv(fd,buff + recv_index,HTTP_BUFF_SIZE- recv_index,0);
        if(ret <= 0)
            break;
        recv_index =recv_index + ret;
        parse();
    }
}

int HttpConnection::parse(){
    std::vector<std::string> vec = split(buff,'\n');
    while(!vec.empty())
    {
        switch(state){
        case PARSE_REQUEST:
            
            break;
        case PARSE_HEADER: 
            
            break;
        case PARSE_CONTENT: break;
        default:return -1;
    }
    }
    
}