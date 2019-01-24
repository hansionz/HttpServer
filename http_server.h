#pragma once
#include <string>
#include <unordered_map>

// 定义命名空间，防止命名冲突
namespace http_server{

// 存放http消息报头
typedef std::unordered_map<std::string,std::string> Header;

struct Request{
    //例如:url为一个形如http://www.baidu.com/index.html?kwd="cpp"
    std::string method;
    std::string url;
    std::string url_path;    //index.html(服务器的某个路径,本质上是一个相对路径.相对于HTTP服务器自身的根目录，和Linux系统的根目录不相同,Http服务器根目录是把所有允许对外访问的文件放在某个目录下)
    std::string query_string;//kwd="cpp"
    //std::string version:   //暂时不考虑版本号
    Header header;           //一组字符串键值对
    std::string body;        //http的请求body
};

struct Response{
    int code;        //状态码
    std::string desc;//状态码描述

    // 1.下面这两个变量专门给处理静态页面时使用的
    // 如果当前请求是静态页面，这两个字段会被填充.并且cgi_resp字段为空
    Header header;   //响应报文中的header数据
    std::string body;//响应报文中的body数据


    // 2.下面这个变量专门给CGI来使用，并且如果当前请求是CGI的话
    // cgi_resp就会被CGI程序进行填充,并且，header和body这两个字段为空
    std::string cgi_resp;
    // CGI程序返回给父进程的内容,包含了部分header和body,引入这个变量，是为了避免解析CGI程序返回的内容，因为这部分内容可以直接写到socket中
};

//当前请求的上下文，包含了这次请求的所有需要的中间数据
//方便进行参数扩展，整个处理请求的过程中，每个环节都能够拿到
//所有和这次请求相关的数据
class HttpServer;
struct Context{
    Request req;   
    Response resp;
    int new_sock;
    HttpServer* server;
};

//实现核心流程的类
class HttpServer{
public:
    /*以下的几个函数，返回0表示成功
    返回小于0的值表示执行失败*/

    // 运行服务器
    int Start(const std::string& ip, uint32_t port);

private:
    //根据http请求字符串进行反序列化
    //从socket中读取一个字符串输出Request对象
    int ReadOneRequest(Context* context);
    //根据Response对象序列化拼接成一个字符串，写回到客户端
    int WriteOneResponse(Context* context);
    //根据Request对象，构造Response对象
    int HandlerRequest(Context* context);
    int Process404(Context* context);
    //服务器处理静态页面
    int ProcessStaticFile(Context* context);
    //服务器处理动态页面
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
