#include"http.h"
HttpConnection::HttpConnection(int f){
    fd = f;
    recv_index = 0;
    parse_index = 0;
    state = PARSE_REQUEST;
    current_content_size = 0;
    content_size = 0;
    memset(buff,0,HTTP_BUFF_SIZE);
}

//string before each delim.ps:substring not ended with delim will not be included.
std::vector<std::string> HttpConnection::split(char *str,char delim,bool take_delim){
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
    std::vector<std::string> vec = split(buff + parse_index,'\n',true);
    std::vector<std::string> line_vec;
    char tmp[HTTP_BUFF_SIZE];
    std::size_t i = 0;
    while(i != vec.size())
    {
        switch(state){
        case PARSE_REQUEST:
            strncpy(tmp,vec[i].c_str(),vec[i].size() - 1);
            line_vec = split(tmp,' ',false);
            if(line_vec.size() == 3)
            {
                state = PARSE_HEADER;
                method = line_vec[0];
                url = line_vec[1];
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
            if(vec[i] == "\n")
            {
                if(headers.find("Content-length") == headers.end() || headers["Content-length"] == "0")
                {
                    process();
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
                strncpy(tmp,vec[i].c_str(),vec[i].size() - 1);
                line_vec = split(tmp,' ',false);
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
                process();
                return 1;
            }
            break;
        default:return -1;
        }
    }
    return 0;
}