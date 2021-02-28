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

void HttpConnection::adjust_buff(){
    for(int i = 0;i < recv_index - parse_index;i++)
    {
        buff[i] = buff[parse_index + i];
    }
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
                return -1;
            }
            break;
        case PARSE_HEADER: 
            if(vec[i] == "\r\n")
            {
                if(headers.find("Content-length") == headers.end() || headers["Content-length"] == "0")
                {
                    parse_index += vec[i].size();
                    adjust_buff();
                    return process();
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
                adjust_buff();
                return process();
            }
            break;
        default:return -1;
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
            return 0;
        }
        char file_path[FILE_PATH_SIZE];
        strcpy(file_path,file_path_prefix);
        strcat(file_path,file_name.c_str());
        struct stat file_stat;
        if(stat(file_path,&file_stat) < 0)
        {
            send_404_response();
            return 0;
        }
        int fd = open(file_path,O_RDONLY);
        send_200_response(fd);
        return 1;
    }
    else
        return -1;
}