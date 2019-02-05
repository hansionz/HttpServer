#pragma once
#include <string>
#include <unordered_map>

namespace http_server{

// 请求和响应的header
typedef std::unordered_map<std::string,std::string> Header;

//请求报文
struct Request{
    std::string method;      //请求方法
    std::string url;         //url
    //形如http://www.baidu.com/index.html?kwd="cpp"
    std::string url_path;    //index.html
    std::string query_string;//参数
    Header header;           //header
    std::string body;        //http的请求body
};

//响应报文
struct Response{
    int code;        //状态码
    std::string desc;//状态码描述

    /*下面这两个变量专门给处理静态页面时使用的*/
    //当前请求如果是请求静态页面,这两个字段会被填充
    //并且cgi_resp字段为空
    Header header;   //响应报文中的header数据
    std::string body;//响应报文中的body数据

    /*下面这个变量专门给CGI来使用,如果当前请求时CGI*/
    //cgi_resp就会被CGI程序进行填充
    //header和body这两个字段为空
    std::string cgi_resp;
    //CGI程序返回给父进程的内容,包含了部分header和body引入这个变量是为了避免
    //解析CGI程序返回的内容,因为这部分内容可以直接写到socket中
};

//当前请求的上下文,包含了这次请求的所有需要的中间数据
//方便进行扩展,整个处理请求的过程中,每个环节都能够拿到
//所有和这次请求相关的数据
class HttpServer;
struct Context{
    Request req;
    Response resp;
    int new_sock;
    HttpServer* server;
};

//HTTP服务器核心流程的类
class HttpServer{
public:
    /*以下的几个函数,返回0表示成功,返回小于0的值表示执行失败*/
    int Start(const std::string& ip,short port);

private:
    //从socket中读取一个Request
    int ReadOneRequest(Context* context);
    //根据Response对象,拼接成一个字符串,写回到客户端
    int WriteOneResponse(Context* context);
    //根据Request对象,构造Response对象
    int HandlerRequest(Context* context);
    //构造404页面
    int Process404(Context* context);
    //处理静态页面
    int ProcessStaticFile(Context* context);
    //处理动态页面(CGI)
    int ProcessCGI(Context* context);
private:
    static void* ThreadEntry(void* arg);
    int ParseFirstLine(const std::string& first_line,std::string* method,std::string* url);
    int ParseUrl(const std::string& url,std::string* url_path,std::string* query_string);
    int ParseHeader(const std::string& header_line,Header* header);
    void GetFilePath(const std::string& url_path,std::string* file_path);
    //测试函数
    void PrintRequest(const Request& req);
};
} 
