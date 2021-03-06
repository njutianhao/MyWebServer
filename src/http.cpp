#include"http.h"
HttpConnection::HttpConnection(int f){
    fd = f;
    recv_index = 0;
    parse_index = 0;
    state = PARSE_REQUEST;
    current_content_size = 0;
    content_size = 0;
    mmaped = false;
    assert(getcwd(file_path_prefix,FILE_PATH_SIZE) != NULL);
    strcat(file_path_prefix,"/root");
    //strcpy(file_path_prefix,"/home/th/Desktop/MyWebServer/root/");
    file_path_prefix_length = strlen(file_path_prefix);
    keepalive = false;
    file_fd = -1;
    file_size = 0;
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
        str++;
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
        if(ret < 0)
        {
            if(errno == EAGAIN || errno == EWOULDBLOCK)
            {
                break; // read again
            }
            else   
            {
                return -1;  // something wrong happened
            }
        }
        else if(ret == 0)
        {
            return -1; // connection end
        }
        recv_index =recv_index + ret;
    }
    ret = parse();
    if(ret == 1)
    {
        ret = process();
    }
    set_keepalive();
    switch(ret){
        case -2: create_404_response();break;
        case -1: create_400_response();break;
        case 0:return 0;
        case 1: create_200_response();break;
        default:create_400_response();
    }
    return 1;
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
    current_content_size = 0;
    content_size = 0;
    file_fd = -1;
    file_size = 0;
}

// 400:-1 404:-2 again:0 finish:1
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
                return -1;
            break;
        case PARSE_HEADER: 
            if(vec[i] == "\r\n")
            {
                if(headers.find("Content-length") == headers.end() || headers["Content-length"] == "0")
                {
                    parse_index += vec[i].size();
                    return 1;
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
                int pos = vec[i].find(':');
                if(pos != std::string::npos)
                {
                    std::cout << "debug:"<<vec[i].substr(0,pos)<<"|"<<vec[i].substr(pos+2,vec[i].size() - pos) <<std::endl;
                    headers[vec[i].substr(0,pos)] = vec[i].substr(pos+2,vec[i].size() - pos);
                    parse_index += vec[i].size();
                    i++;
                }
                else
                    return -1;
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
                return 1;
            }
            break;
        default:
            return -1;
        }
    }
    return 0;
}

void HttpConnection::create_404_response(){
    strcpy(send_buff_content,"404 Not Found\r\n");
    sprintf(send_buff_header,"\
HTTP/1.1 404 Not Found\r\n\
Content-Length: %ld\r\n\
Connection: %s\r\n\
\r\n",sizeof(send_buff_content),"close");
    iov[0].iov_base = send_buff_header;
    iov[0].iov_len = strlen(send_buff_header);
    iov[1].iov_base = send_buff_content;
    iov[1].iov_len = strlen(send_buff_content);
}

void HttpConnection::create_400_response(){
    strcpy(send_buff_content,"Bad Request\r\n");
    sprintf(send_buff_header,"\
HTTP/1.1 400 Bad Request\r\n\
Content-Length: %ld\r\n\
Connection: %s\r\n\
\r\n",sizeof(send_buff_content),"close");
    iov[0].iov_base = send_buff_header;
    iov[0].iov_len = strlen(send_buff_header);
    iov[1].iov_base = send_buff_content;
    iov[1].iov_len = strlen(send_buff_content);
}

void HttpConnection::create_200_response(){
    sprintf(send_buff_header,"\
HTTP/1.1 200 OK\r\n\
Content-Length: %ld\r\n\
Connection: %s\r\n\
\r\n",file_size,keepalive==true?"keep-alive":"close");
    iov[0].iov_base = send_buff_header;
    iov[0].iov_len = strlen(send_buff_header);
    mmaped = true;
    mapaddr = mmap(NULL,file_size,PROT_READ,MAP_PRIVATE,file_fd,0);
    iov[1].iov_base = mapaddr;
    iov[1].iov_len = file_size;
    std::cout << "debug:"<< iov[1].iov_base << " " << iov[1].iov_len << std::endl;
}

void HttpConnection::set_keepalive(){
    if(headers.find("Connection") != headers.end() && headers["Connection"] == "keep-alive\r\n")
        keepalive = true;
}

int HttpConnection::send(){
    while(1)
    {
        int ret = writev(fd,iov,2);
        if(ret == -1)
        {
            if(errno == EAGAIN || errno == EWOULDBLOCK)
                return 0;
            else
            {
                if(mmaped)
                {
                    mmaped = false;
                    if(munmap(mapaddr,file_size)==-1)
                    {
                        perror("Error");
                    }
                }
                return -1;
            }
        }
        else
        {
            if(iov[0].iov_len >= ret)
            {
                iov[0].iov_base = (char *)iov[0].iov_base + ret;
                iov[0].iov_len -= ret;
            }
            else
            {
                iov[1].iov_base = (char *)iov[1].iov_base + ret - iov[0].iov_len;
                iov[1].iov_len -= ret - iov[0].iov_len;
                iov[0].iov_len = 0;
            }
            if(iov[1].iov_len <= 0)
                break;
        }
    }
    if(mmaped)
    {
        mmaped = false;
        assert(munmap(mapaddr,file_size)==0);
    }
    if(!keepalive)
        return -1;
    else
    {
        adjust_buff();
        return 1;
    }
}

int HttpConnection::process(){
    if(method == "GET")
    {
        if(url.empty())
            return -2;
        else if(url == "/")
            url += "index.html";
        char file_path[FILE_PATH_SIZE];
        strcpy(file_path,file_path_prefix);
        strcat(file_path,url.c_str());
        struct stat file_stat;
        if(stat(file_path,&file_stat) < 0)
            return -2;
        file_fd = open(file_path,O_RDONLY);
        if(file_fd == -1)
            return -2; 
        file_size = file_stat.st_size;
        return 1;
    }
    else
        return -1;
}