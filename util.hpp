/*
 * 此源文件定义一些工具类
 * 
 */ 
#pragma once
#include <iostream>
#include <sys/socket.h>
#include <vector>
#include <sys/time.h>
#include <unordered_map>
#include <fstream>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

// 生成时间戳的类
// tv_sec s
// tv_usec ms
class TimeUtil{
public:
    //获取当前的秒级时间戳
    /*不建议使用无符号类型,因为当两个时间戳相减时,无符号数就会出问题*/
    static int64_t TimeStamp(){
        struct timeval tv;
        gettimeofday(&tv,NULL);
        return tv.tv_sec;//返回的当前时间戳
    }
    //获取当前的微秒级时间戳
    static int64_t TimeStampUS(){
        struct timeval tv;
        gettimeofday(&tv,NULL);
        return 1000*1000*tv.tv_sec + tv.tv_usec;
    }
};


//枚举日志级别,方便打印日志信息
enum LogLevel{
    DEBUG,   
    INFO,
    WARNING,
    ERROR,
    FATAL,// 致命错误
};

// 内联函数相当于一个宏,轻量级的函数使用内联替代,减少函数调用和返回的开销
inline std::ostream& Log(LogLevel level, const char* file, int line){
    // level:日志级别
    std::string prefix = "I";
    if(level == WARNING){
        prefix = "W";
    }else if(level == ERROR){
        prefix = "E";
    }else if(level == FATAL){
        prefix = "F";
    }else if(level == DEBUG){
        prefix = "D";
    }
    std::cout<< "[" << prefix << TimeUtil::TimeStamp() << " " << file << ":" << line << "]";
    return std::cout; 
}
// 不能使用函数,定义函数写死了文件和行号
//std::ostream& LOG(LogLevel,__FILE__,__LINE__){
//   return Log(level,__FILE__,__LINE__);
//}
//使用C风格的宏替换来实现重名名
#define LOG(level) Log(level, __FILE__, __LINE__)

// 处理文件的工具类
class FileUtil
{
public:
    //从文件描述符中读取一行,一行的界定标识\n \r \r\n
    //返回的line中是不包含界定标识的
    //例如:aaa\nbbb\nccc
    //调用Readline返回的line对象的内容为aaa，不包含\n
static int ReadLine(int fd,std::string* line)
{
   line->clear();
   while(true)
   {
      char c = '\0';
      // 每次只读一个字符
      ssize_t read_size = recv(fd, &c, 1, 0);
      if(read_size<=0)
      {
        return -1;
      }
      //如果当前字符是\r,就把这个情况处理成\n
      if(c=='\r')
      {
        // 设置MAG_PEEK参数表示从缓冲区读了一个字符,但是缓冲区并没有把该字符删除
        recv(fd, &c, 1, MSG_PEEK);
        if(c=='\n')
        {
            // 发现\r后面一个字符刚好就是\n，为了不影响下次循环，就需要把这样的字符从缓冲区干掉
             recv(fd,&c,1,0);
        }
        else
        {
            c = '\n';
        }
    }
    // 这个条件涵盖了\r和\r\n的情况
    if(c=='\n')
    {
       break;
    }
   line->push_back(c);
  }
    return 0;
}

// 一次读len个字符
static int ReadN(int fd,size_t len,std::string* output)
{
    output->clear();
    char c = '\0';
    for(size_t i=0;i < len;i++)
    {
        recv(fd,&c,1,0);
        output->push_back(c);
    }
    return 0;
}

static int ReadAll(int fd,std::string* output)
{
    while(true)
    {
        char buf[1024] = {0};
        ssize_t read_size = read(fd,buf,sizeof(buf)-1);
        if(read_size < 0)
        {
            perror("read");
            return -1;
        }
        if(read_size == 0)
        {
            //读完了
            return 0;
        }
        buf[read_size] = '\0';
          (*output) += buf;
    }
    return 0;
}

// 判断一个路径为目录还是文件
// 使用boost库中的filesystem判断
// 也可以使用系统调用stat判断
static bool IsDir(const std::string& file_path)
{
    return boost::filesystem::is_directory(file_path);
}

// 从文件中读取全部内容到std::string中
static int ReadAll(const std::string& file_path,std::string* output)
{
    std::ifstream file(file_path.c_str());
    if(!file.is_open())
    {
        LOG(ERROR) << "Open file error! file_path=" << file_path << "\n";
        return -1;
    }
    //seekg是调整文件指针的位置,此处是将文件指针调整到文件末尾
    file.seekg(0,file.end);
    //查询当前文件指针的位置,返回值就是文件指针位置相对于文件起始位置的偏移量,也就是文件的长度
    int length = file.tellg();
    //为了从头读取文件,需要把文件指针设置到开头位置
    file.seekg(0,file.beg);
    //读取完整的文件内容
    //string在resize时不用考虑'\0'
    output->resize(length);
    file.read(const_cast<char*>(output->c_str()),length);
    //ifstream对象会在析构的时候自动关闭文件描述符
    file.close();
    return 0;
   }
};

// 操作字符串的工具类
class StringUtil
{
public:
    //把一个字符串,按照split_char进行切分,分成的n个子串放到output数组中
    //aaa /3/3 bbb
    //token_compress_on:  aaa bbb
    //token_compress_off: aaa "" bbb
static int Split(const std::string& input, const std::string& split_char, std::vector<std::string>* output)
{
    boost::split(*output, input, boost::is_any_of(split_char), boost::token_compress_on);
    // is_any_of表示可以使用任意字符分割字符串
    return 0;
}
    
typedef std::unordered_map<std::string,std::string> UrlParam;
static int ParseUrlParam(const std::string& input,UrlParam* output)
{
    //1.先按照取地址符号切分成若干个k-v
    std::vector<std::string> params;
    Split(input,"&",&params);
    //2.在针对每一个k-v,按照=切分放到输出结果中
    for(auto item : params)
    {
        std::vector<std::string> kv;
        Split(item,"=",&kv);
        if(kv.size() != 2)
        {
            //说明参数非法
             LOG(WARNING)<<"kv format error! item="<<item<<"\n";
             continue;
        }
        (*output)[kv[0]] = kv[1];
    }
     return 0;
  }
};
