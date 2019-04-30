#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define uni_strcmp  strcmp

static int _get_host_ip(char *buf,int size) {
  struct ifaddrs *ifc, *ifc1;
  int ret =-1;
  int family;
  if(0 != getifaddrs(&ifc)) {
    return ret;
  }
  ifc1 = ifc;
  for(; NULL != ifc; ifc = ifc->ifa_next) {
    if (ifc->ifa_addr == NULL)
      continue;
    family = ifc->ifa_addr->sa_family;
    if((AF_INET != family)&&(AF_INET6 != family)) {
      continue;
    }
    if(0 == uni_strcmp("wlan0",ifc->ifa_name)) {
#if 0
      getnameinfo(ifc->ifa_addr,(family == AF_INET) ? sizeof(struct sockaddr_in) :
                  sizeof(struct sockaddr_in6),
                  buf, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
#else
      if(AF_INET == family)
        inet_ntop(AF_INET, &(((struct sockaddr_in*)(ifc->ifa_addr))->sin_addr), buf, size);
      else
        inet_ntop(AF_INET6, &(((struct sockaddr_in6*)(ifc->ifa_addr))->sin6_addr), buf, size);

#endif
      ret =0;
      break;
    } else if(0 == uni_strcmp("eth0",ifc->ifa_name)) {
#if 0
      getnameinfo(ifc->ifa_addr,
                  (family == AF_INET) ? sizeof(struct sockaddr_in) :sizeof(struct sockaddr_in6),
                  buf, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
#else
      if(AF_INET == family)
        inet_ntop(AF_INET, &(((struct sockaddr_in*)(ifc->ifa_addr))->sin_addr), buf, size);
      else
        inet_ntop(AF_INET6, &(((struct sockaddr_in6*)(ifc->ifa_addr))->sin6_addr), buf, size);

#endif
      ret = 0;
      break;
    } else if(0 == uni_strcmp("ens33",ifc->ifa_name)) {
#if 0
      getnameinfo(ifc->ifa_addr,(family == AF_INET) ? sizeof(struct sockaddr_in) :
                  sizeof(struct sockaddr_in6),
                  buf, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
#else
      if(AF_INET == family)
        inet_ntop(AF_INET, &(((struct sockaddr_in*)(ifc->ifa_addr))->sin_addr), buf, size);
      else
        inet_ntop(AF_INET6, &(((struct sockaddr_in6*)(ifc->ifa_addr))->sin6_addr), buf, size);
#endif
      ret =0;
      break;
    }
  }

  freeifaddrs(ifc1);
  return ret;
}

int main(int argc , char argv[]) {
  int ret;
  char host[INET6_ADDRSTRLEN];
  memset(host,0,sizeof(host));
  ret = _get_host_ip(host,sizeof(host));
  printf("[ret=%d]%s\n",ret,host);

}

