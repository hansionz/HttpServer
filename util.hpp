//此源文件实现一些工具类
#pragma once
#include <iostream>
#include <sys/socket.h>
#include <sys/stat.h>
#include <string.h>
#include <vector>
#include <sys/time.h>
#include <unordered_map>
#include <fstream>
#include <unistd.h>
//boost库
//#include <boost/algorithm/string.hpp>
//#include <boost/filesystem.hpp>

//时间戳的类
class TimeUtil{
public:
    //获取当前的秒级时间戳
    //不能使用无符号类型,因为时间戳是需要相减的,无符号就会出问题
    static int64_t TimeStamp(){
        struct timeval tv;
        gettimeofday(&tv,NULL);
        return tv.tv_sec;
    }
    //获取当前的微秒级时间戳
    static int64_t TimeStampUS(){
        struct timeval tv;
        gettimeofday(&tv,NULL);
        return 1000*1000*tv.tv_sec + tv.tv_usec;
    }
};

//枚举日志级别
enum LogLevel{
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    CRITICAL,//致命错误
};

//使用内联函数相当于C风格宏,减少函数调用和返回的开销
inline std::ostream& Log(LogLevel level, const char* file, int line){
    //prefix 记录日志级别
    std::string prefix = "I";
    if(level == WARNING){
        prefix = "W";
    }else if(level == ERROR){
        prefix = "E";
    }else if(level == CRITICAL){
        prefix = "C";
    }else if(level == DEBUG){
        prefix = "D";
    }
    std::cout<< "[" << prefix << TimeUtil::TimeStamp() << " " << file << ":" << line << "]";
    return std::cout; 
}

//不能使用函数,定义函数就写死了文件和行号
//返回的文件和行号就成了该函数所处的文件和行号
//std::ostream& LOG(LogLevel,__FILE__,__LINE__){
//   return Log(level,__FILE__,__LINE__);
//}
#define LOG(level) Log(level, __FILE__, __LINE__)

// 处理文件的工具类
class FileUtil
{
public:
    //从文件描述符中读取一行,一行的界定标识\n \r \r\n,返回的line中是不包含界定标识的
    //例如:aaa\nbbb\nccc
    //调用Readline返回的line对象的内容为aaa不包含\n
   static int ReadLine(int fd, std::string* line)
   {
       line->clear();
       while(true)
       {
           // 每次只读一个字符
           char c = '\0';
           ssize_t read_size = recv(fd, &c, 1, 0);
           if(read_size <= 0)
           {
               return -1;
           }
           //如果当前字符是\r就把这个情况处理成\n
           if(c=='\r')
           {
               //MSG_PEEK表示虽然从缓冲区读了一个字符但是缓冲区并没有删掉 
               recv(fd,&c,1,MSG_PEEK);
               if(c=='\n')
               {
                   //发现\r后面一个字符刚好就是\n,
                   //为了不影响下次循环就需要把这样的字符从缓冲区拿掉
                   recv(fd,&c,1,0);
               }
               else
               {
                    c = '\n';
               }
           }
           //这个条件涵盖了\r和\r\n的情况
           if(c=='\n')
           {
               break;
           }
           line->push_back(c);
       }
       return 0;
   }

   static int ReadN(int fd, size_t len, std::string* output)
   {
        output->clear();
        char c = '\0';
        for(size_t i = 0;i < len;i++)
        {
            recv(fd, &c, 1, 0);
            output->push_back(c);
        }
        return 0;
   }

   static int ReadAll(int fd,std::string* output)
   {
        while(true)
        {
            char buf[1024] = {0};
            ssize_t read_size = read(fd, buf, sizeof(buf)-1);
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

   static bool IsDir(const std::string& file_path)
   {
      struct stat buf; 
      int ret = stat(file_path.c_str(), &buf); 
      if( (buf.st_mode&__S_IFMT) == __S_IFDIR )
      {
        return true;
      }
      return false;
   }

   //从文件中读取全部内容到std::string中
   static int ReadAll(const std::string& file_path, std::string* output)
   {
        std::ifstream file(file_path.c_str());
        if(!file.is_open())
        {
            LOG(ERROR)<<"Open file error! file_path="<<file_path<<"\n";
            return -1;
        }
        //seekg调整文件指针的位置，此处是将文件指针调整到文件末尾
        file.seekg(0,file.end);
        //查询当前文件指针的位置，返回值就是文件指针位置相对于文件
        //起始位置的偏移量
        int length = file.tellg();
        //为了从头读取文件，需要把文件指针设置到开头位置
        file.seekg(0,file.beg);
        //读取完整的文件内容
        output->resize(length);
        file.read(const_cast<char*>(output->c_str()),length);
        //万一忘记写close，问题不大，
        //因为ifstream会在析构的时候自动关闭文件描述符
        file.close();
        return 0;
   }
};

// 处理字符串的工具类
class StringUtil
{
public:
    //把一个字符串，按照split_char进行切分，分成的n个子串，放到output数组中
    static int Split(const std::string& input,const std::string& split_char,std::vector<std::string>* output)
    {
        //boost::split(*output,input,boost::is_any_of(split_char),boost::token_compress_on);
        char buf[1024];
        strcpy(buf, input.c_str());
        int i=0;
        //字符串分割函数
        char* tmp =strtok(buf, split_char.c_str());
        if(tmp != NULL)
          output->push_back(tmp);
        while(1)
        {
          tmp = strtok(NULL,split_char.c_str());
          if(tmp==NULL)
          {
            break;
          }
          output->push_back(tmp);
        }
        return 0;
    }
    
    typedef std::unordered_map<std::string,std::string> UrlParam;
    static int ParseUrlParam(const std::string& input, UrlParam* output)
    {
        //1.先按照取地址符号切分成若干个k-v
        std::vector<std::string> params;
        Split(input,"&", &params);
        //2.在针对每一个k-v，按照 = 切分，放到输出结果中
        for(auto item : params)
        {
            std::vector<std::string> kv;
            Split(item,"=",&kv);
            if(kv.size() != 2)
            {
                LOG(WARNING)<<"kv format error! item="<<item<<"\n";
                continue;
            }
            (*output)[kv[0]] = kv[1];
        }
        return 0;
    }
};
