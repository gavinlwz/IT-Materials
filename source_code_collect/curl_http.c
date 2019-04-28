#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <arpa/inet.h>
#include <errno.h>
#include <arpa/inet.h>
#include <curl/curl.h>
#include <semaphore.h> 
#include <stdarg.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <sys/param.h>
#include <getopt.h>
#include <curl/curl.h>
#include <openssl/crypto.h>
 
 
#define curl_printf printf
 
#define CURL_BUF_MAX_LEN  1024
#define CURL_NAME_MAX_LEN 128
#define CURL_URL_MAX_LEN  128 
 
enum curl_method
{
    CURL_METHOD_GET  = 1,
    CURL_METHOD_POST = 2,
};
 
struct curl_http_args_st
{
    int  curl_method;    // curl 方法命令,enum curl_method
    char url[CURL_URL_MAX_LEN];        // URL 
    
    char file_name[CURL_NAME_MAX_LEN];    // 返回数据保存为文件
    FILE *file_fd;                        // 文件所指向的描述符, 用完后需要手动fclose
 
    int  data_len;                        // 文件数据保存在内存中的长度
    char *data;                            // 文件数据保存在内存中的指针, 用完后手动free 
 
    char post_data[CURL_BUF_MAX_LEN];    // POST 表单数据
    char post_file[CURL_NAME_MAX_LEN];    // POST 文件名
};
 
/* we have this global to let the callback get easy access to it */
static pthread_mutex_t *lockarray;
 
static void lock_callback(int mode, int type, char *file, int line)
{
	(void)file;
  (void)line;
  if (mode & CRYPTO_LOCK) {
    pthread_mutex_lock(&(lockarray[type]));
  }
  else {
    pthread_mutex_unlock(&(lockarray[type]));
  }
}
 
static unsigned long thread_id(void)
{
  unsigned long ret;
 
  ret=(unsigned long)pthread_self();
  return(ret);
}
 
static void init_locks(void)
{
  int i;
 
  lockarray=(pthread_mutex_t *)OPENSSL_malloc(CRYPTO_num_locks() *
                                            sizeof(pthread_mutex_t));
  for (i=0; i<CRYPTO_num_locks(); i++) {
    pthread_mutex_init(&(lockarray[i]),NULL);
  }
 
  CRYPTO_set_id_callback((unsigned long (*)())thread_id);
  CRYPTO_set_locking_callback((void (*)())lock_callback);
}
 
static void kill_locks(void)
{
  int i;
 
  CRYPTO_set_locking_callback(NULL);
  for (i=0; i<CRYPTO_num_locks(); i++)
    pthread_mutex_destroy(&(lockarray[i]));
 
  OPENSSL_free(lockarray);
}
 
 
size_t curl_write_data_cb(void *buffer, size_t size, size_t nmemb, void *stream)
{
    int len = size * nmemb;
    struct curl_http_args_st *args = (struct curl_http_args_st*)stream;    
    
    if (stream)
    {
        if (args->file_name[0])    // 要写文件
        {
            if (!args->file_fd)
            {
                args->file_fd = fopen(args->file_name, "wb");
                if (args->file_fd == NULL)
                {
                    curl_printf("%s[%d]: open file[%s] failed!!\n", __FUNCTION__, __LINE__, args->file_name);
                    return 0;
                }
            }
            fwrite(buffer, size, nmemb, args->file_fd);
        }
        args->data = realloc(args->data, args->data_len + len + 1);    // 多分配一个字节, 以保存\0 
        if (!args->data)
        {
            curl_printf("%s[%d]: realloc failed!!\n", __FUNCTION__, __LINE__);
            return 0;
        }
        memcpy(args->data + args->data_len, buffer, len);
        args->data_len += len;
    }
    
    return len;
}
 
// 创建一个目录，包括其父目录mkdir -p 
int create_dir(const char *sPathName)
{
    char dirname[CURL_NAME_MAX_LEN] = {0};
    int i, len = strlen(sPathName);
    
    strncpy(dirname, sPathName, sizeof(dirname));
 
    len = strlen(dirname);
    for (i = 1; i < len; i++) {
        if (dirname[i] == '/') {
            dirname[i] = 0;
            if (access(dirname, F_OK) != 0) { // 判断是否存在
                if (mkdir(dirname, 0777) == -1) {
                    perror("mkdir  error");
                    curl_printf("mkdir file: dirname=%s\n", dirname);
                    return -1;
                }
            }
            dirname[i] = '/';
        }
    }
 
    return 0;
}
 
/*
    http get func 
*/
int curl_http_get(struct curl_http_args_st *args)
{
    //创建curl对象 
    CURL *curl; 
    CURLcode return_code;
    int ret = -1;
 
    // 如果要保存为文件, 先建立文件目录
    if (args->file_name)
        create_dir(args->file_name);
    
    //curl初始化 
    curl = curl_easy_init(); 
    if (!curl)
    {
        curl_printf("%s[%d]: curl easy init failed\n", __FUNCTION__, __LINE__);
        return ret;;
    }
 
    if (strncmp(args->url, "https://", 8) == 0)
    {
        #if 1    
        // 方法1, 设定为不验证证书和HOST
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        #else
        // 方法2, 设定一个SSL判别证书, 未测试
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 1L)
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl,CURLOPT_CAINFO,"ca-cert.pem");     // TODO: 设置一个证书文件
        #endif 
    }
        
    curl_easy_setopt(curl,CURLOPT_HEADER,0);    //设置httpheader 解析, 不需要将HTTP头写传入回调函数
    
    curl_easy_setopt(curl, CURLOPT_URL,args->url);    //设置远端地址 
 
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);    // TODO: 打开调试信息
    
    curl_easy_setopt(curl,CURLOPT_FOLLOWLOCATION,1);    //设置允许302  跳转
    
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_data_cb);     //执行写入文件流操作 的回调函数
    
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, args);    // 设置回调函数的第4 个参数
    
    curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);    //设置为ipv4类型
    
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30);     //设置连接超时，单位s, CURLOPT_CONNECTTIMEOUT_MS 毫秒
 
    // curl_easy_setopt(curl,CURLOPT_TIMEOUT, 5);            // 整个CURL 执行的时间, 单位秒, CURLOPT_TIMEOUT_MS毫秒
    
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);        //linux多线程情况应注意的设置(防止curl被alarm信号干扰)
 
    return_code = curl_easy_perform(curl); 
    if (CURLE_OK != return_code)
    {
        curl_printf("curl_easy_perform() failed: %s\n", curl_easy_strerror(return_code));
        ret  = 0;
    }
 
    if (args->file_fd)        // 若需要再次处理写入的文件, 在此可以直接使用
    { 
        //关闭文件流
        fclose(args->file_fd); 
    } 
    if (args->data)        // 若要对返回的内容进行处理, 可在此处理
    {
        curl_printf("data_len:%d\n%s\n", args->data_len, args->data);
        free(args->data);
        args->data = NULL;
    }
 
    curl_easy_cleanup(curl);
 
    return ret;
}
 
 
/*
    http post func 
*/
int curl_http_post(struct curl_http_args_st *args)
{
    //创建curl对象 
    CURL *curl; 
    CURLcode return_code;
    struct curl_httppost *formpost = NULL;    // POST 需要的参数
    struct curl_httppost *lastptr  = NULL;
    int ret = -1;
    int post_type = 1; // POST 可以有三种方法
 
    // 如果要保存为文件, 先建立文件目录
    if (args->file_name)
        create_dir(args->file_name);
    
    //curl初始化 
    curl = curl_easy_init(); 
    if (!curl)
    {
        curl_printf("%s[%d]: curl easy init failed\n", __FUNCTION__, __LINE__);
        return ret;;
    }
 
    if (strncmp(args->url, "https://", 8) == 0)
    {
        #if 1    
        // 方法1, 设定为不验证证书和HOST
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        #else
        // 方法2, 设定一个SSL判别证书
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl,CURLOPT_CAINFO,"ca-cert.pem");     // TODO: 设置一个证书文件
        #endif 
    }
        
    curl_easy_setopt(curl,CURLOPT_HEADER,0);    //设置httpheader 解析, 不需要将HTTP头写传入回调函数
    
    curl_easy_setopt(curl, CURLOPT_URL,args->url);    //设置远端地址 
 
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);    // TODO: 打开调试信息
    
    curl_easy_setopt(curl,CURLOPT_FOLLOWLOCATION,1);    //设置允许302  跳转
    
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_data_cb);     //执行写入文件流操作 的回调函数
    
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, args);    // 设置回调函数的第4 个参数
    
    curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);    //设备为ipv4类型
    
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30);     //设置连接超时，单位s, CURLOPT_CONNECTTIMEOUT_MS 毫秒
 
    // curl_easy_setopt(curl,CURLOPT_TIMEOUT, 5);            // 整个CURL 执行的时间, 单位秒, CURLOPT_TIMEOUT_MS毫秒
    
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);        //linux多线程情况应注意的设置(防止curl被alarm信号干扰)
 
    if (post_type == 1)
    {
        // 方法1, 普通的POST , application/x-www-form-urlencoded
        curl_easy_setopt(curl,CURLOPT_POST, 1);        // 设置 为POST 方法
        curl_easy_setopt(curl,CURLOPT_POSTFIELDS, args->post_data);        // POST 的数据内容
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(args->post_data));    // POST的数据长度, 可以不要此选项
    }
    else if (post_type == 2)
    {
        //方法2, multipart/formdata请求, POST args->post_data 中的数据, 也可以是将文件内容读取到post_data中        
        curl_formadd(&formpost, &lastptr, CURLFORM_PTRNAME, "reqformat", CURLFORM_PTRCONTENTS, "plain", CURLFORM_END);    // 设置POST 参数
        curl_formadd(&formpost, &lastptr, CURLFORM_PTRNAME, "file", CURLFORM_PTRCONTENTS, args->post_data, CURLFORM_CONTENTSLENGTH, strlen(args->post_data), CURLFORM_END);    
        curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
    }
    /*else if (post_type == 3)
    {
        //添加内容Content-Disposition: form-data; name="reqformat"....plain 
        curl_formadd(&formpost, &lastptr, CURLFORM_PTRNAME, "reqformat", CURLFORM_PTRCONTENTS, "plain", CURLFORM_END);// 设置POST 参数
        // 添加上传文件,  Content-Disposition: form-data; name="file"; filename="1.jpg"; filename为默认的名字, content-type 为默认curl识别的
        //curl_formadd(&formpost, &lastptr, CURLFORM_PTRNAME, "file", CURLFORM_FILE, args->post_file, CURLFORM_END);
        // 添加上传文件,  //Content-Disposition: form-data; name="file"; filename="1.jpg".,   filename为指定的名字, content-type 为默认curl识别的
        //curl_formadd(&formpost, &lastptr, CURLFORM_PTRNAME, "file", CURLFORM_FILE, err_file, CURLFORM_FILENAME, "1.jpg", CURLFORM_END); 
        // 添加上传文件,  //Content-Disposition: form-data; name="file"; filename="1.jpg".,   filename为指定的名字, content-type为指定的类型
        curl_formadd(&formpost, &lastptr, CURLFORM_PTRNAME, "file", CURLFORM_FILE, err_file, CURLFORM_FILENAME, "1.jpg", CURLFORM_CONTENTTYPE, "image/jpeg", CURLFORM_END);
 
        // 引用页:  http://blog.csdn.net/zxgfa/article/details/8302059
        curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
    }*/
 
    return_code = curl_easy_perform(curl); 
    if (CURLE_OK != return_code)
    {
        curl_printf("curl_easy_perform() failed: %s\n", curl_easy_strerror(return_code));
        ret  = 0;
    }
 
    if (args->file_fd)        // 若需要再次处理写入的文件, 在此可以直接使用
    { 
        //关闭文件流
        fclose(args->file_fd); 
    } 
    if (args->data)        // 若要对返回的内容进行处理, 可在此处理
    {
        curl_printf("data_len:%d\n%s\n", args->data_len, args->data);
        free(args->data);
        args->data = NULL;
    }
 
    curl_easy_cleanup(curl);
    
    if (post_type == 2 || post_type == 3)    // 用这两种方法需要释放POST数据. 
        curl_formfree(formpost);
 
    return ret;
}
 
/*
    1, 从参数中传入操作选项.
    2. 若在线程中要用到HTTPS , 请参看allexamples/threaded-ssl.c 文件使用
*/
int main(int argc, char **argv)
{
    struct curl_http_args_st curl_args;
    memset(&curl_args, 0x00, sizeof(curl_args));
 
    /* Must initialize libcurl before any threads are started */
    curl_global_init(CURL_GLOBAL_ALL);
    /* 多线程使用SSL时, 需要先初始化锁*/
    init_locks();
  
    #if 1 // GET
    curl_args.curl_method = CURL_METHOD_GET;
    //strncpy(curl_args.url, "http://new.baidu.com/index.html", sizeof(curl_args.url)); // http  test ok 
    strncpy(curl_args.url, "http://www.baidu.com/index.html", sizeof(curl_args.url)); // https test ok 
    strncpy(curl_args.file_name, "/tmp/curl/index.html", sizeof(curl_args.file_name));
    #endif 
 
    #if 0 // POST 
    curl_args.curl_method = CURL_METHOD_POST;
    strncpy(curl_args.url, "http://www.wx.com:8080/test.php", sizeof(curl_args.url)); 
    strncpy(curl_args.file_name, "/tmp/curl/index.html", sizeof(curl_args.file_name));
    strncpy(curl_args.post_data, "aa=111111111111111", sizeof(curl_args.post_data)); // 普通post 1 ok 
    // strncpy(curl_args.post_file, "./xx.mp4", sizeof(curl_args.post_file)); // POST 文件OK , 用方法3
    strncpy(curl_args.post_file, "./post_file.txt", sizeof(curl_args.post_file)); // POST 文件OK 
    #endif 
 
    
    switch(curl_args.curl_method)
    {
        case CURL_METHOD_GET:
        {
            curl_http_get(&curl_args);
            break;
        }
        case CURL_METHOD_POST:
        {
            curl_http_post(&curl_args);
            break;
        }
        default:
        {
            curl_printf("curl method error:%d\n", curl_args.curl_method);
            break;
        }
    }
    /* 退出时, 释放锁*/
    kill_locks();
    return 0;
}
