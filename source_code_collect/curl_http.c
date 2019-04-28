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
    int  curl_method;    // curl ��������,enum curl_method
    char url[CURL_URL_MAX_LEN];        // URL 
    
    char file_name[CURL_NAME_MAX_LEN];    // �������ݱ���Ϊ�ļ�
    FILE *file_fd;                        // �ļ���ָ���������, �������Ҫ�ֶ�fclose
 
    int  data_len;                        // �ļ����ݱ������ڴ��еĳ���
    char *data;                            // �ļ����ݱ������ڴ��е�ָ��, ������ֶ�free 
 
    char post_data[CURL_BUF_MAX_LEN];    // POST ������
    char post_file[CURL_NAME_MAX_LEN];    // POST �ļ���
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
        if (args->file_name[0])    // Ҫд�ļ�
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
        args->data = realloc(args->data, args->data_len + len + 1);    // �����һ���ֽ�, �Ա���\0 
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
 
// ����һ��Ŀ¼�������丸Ŀ¼mkdir -p 
int create_dir(const char *sPathName)
{
    char dirname[CURL_NAME_MAX_LEN] = {0};
    int i, len = strlen(sPathName);
    
    strncpy(dirname, sPathName, sizeof(dirname));
 
    len = strlen(dirname);
    for (i = 1; i < len; i++) {
        if (dirname[i] == '/') {
            dirname[i] = 0;
            if (access(dirname, F_OK) != 0) { // �ж��Ƿ����
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
    //����curl���� 
    CURL *curl; 
    CURLcode return_code;
    int ret = -1;
 
    // ���Ҫ����Ϊ�ļ�, �Ƚ����ļ�Ŀ¼
    if (args->file_name)
        create_dir(args->file_name);
    
    //curl��ʼ�� 
    curl = curl_easy_init(); 
    if (!curl)
    {
        curl_printf("%s[%d]: curl easy init failed\n", __FUNCTION__, __LINE__);
        return ret;;
    }
 
    if (strncmp(args->url, "https://", 8) == 0)
    {
        #if 1    
        // ����1, �趨Ϊ����֤֤���HOST
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        #else
        // ����2, �趨һ��SSL�б�֤��, δ����
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 1L)
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl,CURLOPT_CAINFO,"ca-cert.pem");     // TODO: ����һ��֤���ļ�
        #endif 
    }
        
    curl_easy_setopt(curl,CURLOPT_HEADER,0);    //����httpheader ����, ����Ҫ��HTTPͷд����ص�����
    
    curl_easy_setopt(curl, CURLOPT_URL,args->url);    //����Զ�˵�ַ 
 
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);    // TODO: �򿪵�����Ϣ
    
    curl_easy_setopt(curl,CURLOPT_FOLLOWLOCATION,1);    //��������302  ��ת
    
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_data_cb);     //ִ��д���ļ������� �Ļص�����
    
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, args);    // ���ûص������ĵ�4 ������
    
    curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);    //����Ϊipv4����
    
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30);     //�������ӳ�ʱ����λs, CURLOPT_CONNECTTIMEOUT_MS ����
 
    // curl_easy_setopt(curl,CURLOPT_TIMEOUT, 5);            // ����CURL ִ�е�ʱ��, ��λ��, CURLOPT_TIMEOUT_MS����
    
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);        //linux���߳����Ӧע�������(��ֹcurl��alarm�źŸ���)
 
    return_code = curl_easy_perform(curl); 
    if (CURLE_OK != return_code)
    {
        curl_printf("curl_easy_perform() failed: %s\n", curl_easy_strerror(return_code));
        ret  = 0;
    }
 
    if (args->file_fd)        // ����Ҫ�ٴδ���д����ļ�, �ڴ˿���ֱ��ʹ��
    { 
        //�ر��ļ���
        fclose(args->file_fd); 
    } 
    if (args->data)        // ��Ҫ�Է��ص����ݽ��д���, ���ڴ˴���
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
    //����curl���� 
    CURL *curl; 
    CURLcode return_code;
    struct curl_httppost *formpost = NULL;    // POST ��Ҫ�Ĳ���
    struct curl_httppost *lastptr  = NULL;
    int ret = -1;
    int post_type = 1; // POST ���������ַ���
 
    // ���Ҫ����Ϊ�ļ�, �Ƚ����ļ�Ŀ¼
    if (args->file_name)
        create_dir(args->file_name);
    
    //curl��ʼ�� 
    curl = curl_easy_init(); 
    if (!curl)
    {
        curl_printf("%s[%d]: curl easy init failed\n", __FUNCTION__, __LINE__);
        return ret;;
    }
 
    if (strncmp(args->url, "https://", 8) == 0)
    {
        #if 1    
        // ����1, �趨Ϊ����֤֤���HOST
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        #else
        // ����2, �趨һ��SSL�б�֤��
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl,CURLOPT_CAINFO,"ca-cert.pem");     // TODO: ����һ��֤���ļ�
        #endif 
    }
        
    curl_easy_setopt(curl,CURLOPT_HEADER,0);    //����httpheader ����, ����Ҫ��HTTPͷд����ص�����
    
    curl_easy_setopt(curl, CURLOPT_URL,args->url);    //����Զ�˵�ַ 
 
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);    // TODO: �򿪵�����Ϣ
    
    curl_easy_setopt(curl,CURLOPT_FOLLOWLOCATION,1);    //��������302  ��ת
    
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_data_cb);     //ִ��д���ļ������� �Ļص�����
    
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, args);    // ���ûص������ĵ�4 ������
    
    curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);    //�豸Ϊipv4����
    
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30);     //�������ӳ�ʱ����λs, CURLOPT_CONNECTTIMEOUT_MS ����
 
    // curl_easy_setopt(curl,CURLOPT_TIMEOUT, 5);            // ����CURL ִ�е�ʱ��, ��λ��, CURLOPT_TIMEOUT_MS����
    
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);        //linux���߳����Ӧע�������(��ֹcurl��alarm�źŸ���)
 
    if (post_type == 1)
    {
        // ����1, ��ͨ��POST , application/x-www-form-urlencoded
        curl_easy_setopt(curl,CURLOPT_POST, 1);        // ���� ΪPOST ����
        curl_easy_setopt(curl,CURLOPT_POSTFIELDS, args->post_data);        // POST ����������
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(args->post_data));    // POST�����ݳ���, ���Բ�Ҫ��ѡ��
    }
    else if (post_type == 2)
    {
        //����2, multipart/formdata����, POST args->post_data �е�����, Ҳ�����ǽ��ļ����ݶ�ȡ��post_data��        
        curl_formadd(&formpost, &lastptr, CURLFORM_PTRNAME, "reqformat", CURLFORM_PTRCONTENTS, "plain", CURLFORM_END);    // ����POST ����
        curl_formadd(&formpost, &lastptr, CURLFORM_PTRNAME, "file", CURLFORM_PTRCONTENTS, args->post_data, CURLFORM_CONTENTSLENGTH, strlen(args->post_data), CURLFORM_END);    
        curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
    }
    /*else if (post_type == 3)
    {
        //�������Content-Disposition: form-data; name="reqformat"....plain 
        curl_formadd(&formpost, &lastptr, CURLFORM_PTRNAME, "reqformat", CURLFORM_PTRCONTENTS, "plain", CURLFORM_END);// ����POST ����
        // ����ϴ��ļ�,  Content-Disposition: form-data; name="file"; filename="1.jpg"; filenameΪĬ�ϵ�����, content-type ΪĬ��curlʶ���
        //curl_formadd(&formpost, &lastptr, CURLFORM_PTRNAME, "file", CURLFORM_FILE, args->post_file, CURLFORM_END);
        // ����ϴ��ļ�,  //Content-Disposition: form-data; name="file"; filename="1.jpg".,   filenameΪָ��������, content-type ΪĬ��curlʶ���
        //curl_formadd(&formpost, &lastptr, CURLFORM_PTRNAME, "file", CURLFORM_FILE, err_file, CURLFORM_FILENAME, "1.jpg", CURLFORM_END); 
        // ����ϴ��ļ�,  //Content-Disposition: form-data; name="file"; filename="1.jpg".,   filenameΪָ��������, content-typeΪָ��������
        curl_formadd(&formpost, &lastptr, CURLFORM_PTRNAME, "file", CURLFORM_FILE, err_file, CURLFORM_FILENAME, "1.jpg", CURLFORM_CONTENTTYPE, "image/jpeg", CURLFORM_END);
 
        // ����ҳ:  http://blog.csdn.net/zxgfa/article/details/8302059
        curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
    }*/
 
    return_code = curl_easy_perform(curl); 
    if (CURLE_OK != return_code)
    {
        curl_printf("curl_easy_perform() failed: %s\n", curl_easy_strerror(return_code));
        ret  = 0;
    }
 
    if (args->file_fd)        // ����Ҫ�ٴδ���д����ļ�, �ڴ˿���ֱ��ʹ��
    { 
        //�ر��ļ���
        fclose(args->file_fd); 
    } 
    if (args->data)        // ��Ҫ�Է��ص����ݽ��д���, ���ڴ˴���
    {
        curl_printf("data_len:%d\n%s\n", args->data_len, args->data);
        free(args->data);
        args->data = NULL;
    }
 
    curl_easy_cleanup(curl);
    
    if (post_type == 2 || post_type == 3)    // �������ַ�����Ҫ�ͷ�POST����. 
        curl_formfree(formpost);
 
    return ret;
}
 
/*
    1, �Ӳ����д������ѡ��.
    2. �����߳���Ҫ�õ�HTTPS , ��ο�allexamples/threaded-ssl.c �ļ�ʹ��
*/
int main(int argc, char **argv)
{
    struct curl_http_args_st curl_args;
    memset(&curl_args, 0x00, sizeof(curl_args));
 
    /* Must initialize libcurl before any threads are started */
    curl_global_init(CURL_GLOBAL_ALL);
    /* ���߳�ʹ��SSLʱ, ��Ҫ�ȳ�ʼ����*/
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
    strncpy(curl_args.post_data, "aa=111111111111111", sizeof(curl_args.post_data)); // ��ͨpost 1 ok 
    // strncpy(curl_args.post_file, "./xx.mp4", sizeof(curl_args.post_file)); // POST �ļ�OK , �÷���3
    strncpy(curl_args.post_file, "./post_file.txt", sizeof(curl_args.post_file)); // POST �ļ�OK 
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
    /* �˳�ʱ, �ͷ���*/
    kill_locks();
    return 0;
}
