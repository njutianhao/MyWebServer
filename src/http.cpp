#include"http.h"
HttpConnection::HttpConnection(int f){
    fd = f;
    recv_index = 0;
    parse_index = 0;
    state = PARSE_REQUEST;
    current_content_size = 0;
    content_size = 0;
    strcpy(file_path_prefix,"/home/th/Desktop/MyWebServer/root/");
    file_path_prefix_length = strlen(file_path_prefix);
    keepalive = false;
    memset(buff,0,HTTP_BUFF_SIZE);
}

std::vector<std::string> HttpConnection::split(char *str,char delim,bool take_delim,bool last_substring){
    std::vector<std::string> vec;
    char *head = str;
    int a = take_delim?1:0;
    while(*str != '\0')
    {
        if(*str == delim)
        {
            vec.push_back(std::string(head,str + a - head));
            head = str + 1;
        }
    }
    if(last_substring && head != str)
    {
        vec.push_back(std::string(head,str));
    }
    return vec;
}

int HttpConnection::run(){
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

void HttpConnection::adjust_buff(){
    for(int i = 0;i < recv_index - parse_index;i++)
    {
        buff[i] = buff[parse_index + i];
    }
    recv_index -= parse_index;
    parse_index = 0;
    headers.clear();
    params.clear();
    keepalive = false;
    state = PARSE_REQUEST;
}

int HttpConnection::parse(){
    std::vector<std::string> vec = split(buff + parse_index,'\n',true,false);
    std::vector<std::string> line_vec;
    char tmp[HTTP_BUFF_SIZE];
    std::size_t i = 0;
    while(i != vec.size())
    {
        switch(state){
        case PARSE_REQUEST:
            strncpy(tmp,vec[i].c_str(),vec[i].size() - 1);
            line_vec = split(tmp,' ',false,true);
            if(line_vec.size() == 3)
            {
                state = PARSE_HEADER;
                method = line_vec[0];
                url = line_vec[1];
                std::size_t para_pos = url.find('?');
                if(para_pos != std::string::npos)
                {
                    strcpy(tmp,url.c_str()+para_pos+1);
                    url = url.substr(0,para_pos);
                    std::vector<std::string> param_raw = split(tmp,'&',false,true);
                    for(std::string i : param_raw)
                    {
                        strcpy(tmp,i.c_str());
                        std::vector<std::string> param_tmp = split(tmp,'=',false,true);
                        params[param_tmp[0]] = param_tmp[1];
                    }
                }
                version = line_vec[2];
                parse_index += vec[i].size();
                i++;
            }
            else
            {
                parse_index += vec[i].size();
                adjust_buff();
                send_400_response();
                return -1;
            }
            break;
        case PARSE_HEADER: 
            if(vec[i] == "\r\n")
            {
                if(headers.find("Content-length") == headers.end() || headers["Content-length"] == "0")
                {
                    parse_index += vec[i].size();
                    int ret = process();
                    adjust_buff();
                    return ret;
                }
                else
                {
                    content_size = atoi(headers["Content-length"].c_str());
                    state = PARSE_CONTENT;
                    parse_index += vec[i].size();
                    i++;
                }
            }
            else
            {
                strncpy(tmp,vec[i].c_str(),vec[i].size() - 1);
                line_vec = split(tmp,':',false,true);
                if(line_vec.size() == 2)
                {
                    headers[line_vec[0]] = line_vec[1];
                    parse_index += vec[i].size();
                    i++;
                }
                else
                {
                    parse_index += vec[i].size();
                    adjust_buff();
                    send_400_response();
                    return -1;
                }
            }
            break;
        case PARSE_CONTENT:
            content.append(vec[i]);
            parse_index += vec[i].size();
            current_content_size += vec[i].size();
            i++;
            if(current_content_size >= content_size)
            {
                parse_index += vec[i].size();
                int ret = process();
                adjust_buff();
                return ret;
            }
            break;
        default:
            adjust_buff();
            send_400_response();
            return -1;
        }
    }
    return 0;
}

void HttpConnection::send_404_response(){
    char response[HTTP_BUFF_SIZE];
    char content_404[] = "404 Not Found\r\n";
    sprintf(response,"\
HTTP/1.1 404 Not Found\r\n\
Content-Length:%d\r\n\
Connection:%s\r\n\
\r\n\
%s",sizeof(content_404),keepalive==true?"keep-alive":"close",content_404);
    write(fd,response,strlen(response));
}

void HttpConnection::send_400_response(){
    char response[HTTP_BUFF_SIZE];
    char content_400[] = "Bad Request\r\n";
    sprintf(response,"\
HTTP/1.1 400 Bad Request\r\n\
Content-Length:%d\r\n\
Connection:%s\r\n\
\r\n\
%s",sizeof(content_400),keepalive==true?"keep-alive":"close",content_400);
    write(fd,response,strlen(response));
}

void HttpConnection::send_200_response(int file_fd,struct stat file_stat){
    char response_head[HTTP_BUFF_SIZE];
    sprintf(response_head,"\
HTTP/1.1 200 OK\r\n\
Content-Length:%d\r\n\
Connection:%s\r\n\
\r\n",file_stat.st_size,keepalive==true?"keep-alive":"close");
    iovec iov[2];
    iov[0].iov_base = response_head;
    iov[0].iov_len = strlen(response_head);
    iov[1].iov_base = mmap(NULL,file_stat.st_size,PROT_READ,MAP_SHARED,file_fd,0);
    iov[1].iov_len = file_stat.st_size;
    writev(fd,iov,2);
}

int HttpConnection::process(){
    if(headers.find("Connection") != headers.end() && headers["Connection"] == "keep-alive")
        keepalive = true;
    if(method == "GET")
    {
        std::regex pattern("^\\w+://[a-z0-9.]+/");
        std::smatch match;
        std::regex_search(url,match,pattern);
        std::string file_name(match.suffix().str());
        if(file_name.empty())
            file_name = "index.html";
        else if(file_name == url)
        {
            send_404_response();
            adjust_buff();
            return 0;
        }
        char file_path[FILE_PATH_SIZE];
        strcpy(file_path,file_path_prefix);
        strcat(file_path,file_name.c_str());
        struct stat file_stat;
        if(stat(file_path,&file_stat) < 0)
        {
            send_404_response();
            adjust_buff();
            return 0;
        }
        int file_fd = open(file_path,O_RDONLY);
        adjust_buff();
        send_200_response(file_fd,file_stat);
        return 1;
    }
    else
    {
        send_404_response();
        adjust_buff();
        return -1;
    }
}