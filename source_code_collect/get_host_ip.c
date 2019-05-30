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

char *get_ipaddr(char *ifname)
{
	struct ifreq ifr;
	int skfd = 0;
	static char if_addr[16];

	if((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		fprintf(stderr, "%s: open socket error\n", __func__);
		return "";
	}

	strncpy(ifr.ifr_name, ifname, 16);
	if (ioctl(skfd, SIOCGIFADDR, &ifr) < 0) {
		close(skfd);
		fprintf(stderr, "%s: ioctl SIOCGIFADDR error for %s\n", __func__, ifname);
		return "";
	}
	strcpy(if_addr, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
	close(skfd);

	return if_addr;
}

char *get_boardcast_ipaddr(char *ifname)
{
	struct ifreq ifr;
	int skfd = 0;
	static char if_addr[16];

	if((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		fprintf(stderr, "%s: open socket error\n", __func__);
		return "";
	}

	strncpy(ifr.ifr_name, ifname, 16);
	if (ioctl(skfd, SIOCGIFBRDADDR, &ifr) < 0) {
		close(skfd);
		fprintf(stderr, "%s: ioctl SIOCGIFBRDADDR error for %s\n", __func__, ifname);
		return "";
	}
	strcpy(if_addr, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
	close(skfd);

	return if_addr;
}

char *get_macaddr(char *ifname)
{
	struct ifreq ifr;
	char *ptr;
	int skfd;
	static char if_hw[18] = {0};

	if((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		fprintf(stderr, "%s: open socket error\n", __func__);
		return "";
	}
	strncpy(ifr.ifr_name, ifname, 16);
	if(ioctl(skfd, SIOCGIFHWADDR, &ifr) < 0) {
		close(skfd);
		fprintf(stderr, "%s: ioctl fail\n", __func__);
		return "";
	}

	ptr = (char *)&ifr.ifr_addr.sa_data;
	sprintf(if_hw, "%02X%02X%02X%02X%02X%02X",
			(ptr[0] & 0377), (ptr[1] & 0377), (ptr[2] & 0377),
			(ptr[3] & 0377), (ptr[4] & 0377), (ptr[5] & 0377));
	close(skfd);

	return if_hw;
}

int main(int argc , char argv[]) {
  int ret;
  char host[INET6_ADDRSTRLEN];
  memset(host,0,sizeof(host));
  ret = _get_host_ip(host,sizeof(host));
  printf("[ret=%d]%s\n",ret,host);

}

