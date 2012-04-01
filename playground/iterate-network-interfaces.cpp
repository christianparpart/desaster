#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <cstring>

#include <list>
#include <string>
#include <sstream>
#include <list>

struct NetInterface
{
	std::string name;
	sockaddr ipaddr_;
	sockaddr brdaddr_;
	unsigned flags;

	std::string ipaddr_str() const { return inet_ntoa(((struct sockaddr_in *)&ipaddr_)->sin_addr); }
	std::string brdaddr_str() const { return inet_ntoa(((struct sockaddr_in *)&brdaddr_)->sin_addr); }

	std::string flags_str() const {
		std::stringstream sstr;
		static struct {
			const char* str;
			unsigned mask;
		} flagmap[] = {
			{ "UP", IFF_UP },
			{ "BROADCAST", IFF_BROADCAST },
			{ "LOOPBACK", IFF_LOOPBACK },
			{ "P-t-P", IFF_POINTOPOINT },
			{ "DEBUG", IFF_DEBUG },
			{ "NOTRAILERS", IFF_NOTRAILERS },
			{ "RUNNING", IFF_RUNNING },
			{ "NOARP", IFF_NOARP },
			{ "PROMISC", IFF_PROMISC },
			{ "ALLMULTI", IFF_ALLMULTI },
			{ "MASTER", IFF_MASTER },
			{ "SLAVE", IFF_SLAVE },
			{ "MULTICAST", IFF_MULTICAST },
			{ "PORTSEL", IFF_PORTSEL },
			{ "AUTOMEDIA", IFF_AUTOMEDIA },
			{ "DYNAMIC", IFF_DYNAMIC },
		};

		for (const auto& flag: flagmap) {
			if (flags & flag.mask) {
				if (sstr.tellp() > 0) sstr << ",";
				sstr << flag.str;
			}
		}

		return sstr.str();
	}
};

std::list<NetInterface> getNetworkInterfaces()
{
	std::list<NetInterface> result;

	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		perror("socket");
		return result;
	}

	/* Query available interfaces. */
	struct ifconf ifc;
	char buf[1024];
	ifc.ifc_len = sizeof(buf);
	ifc.ifc_buf = buf;
	if (ioctl(fd, SIOCGIFCONF, &ifc) < 0) {
		perror("ioctl(SIOCGIFCONF)");
		return result;
	}

	/* Iterate through the list of interfaces. */
	struct ifreq *ifr = ifc.ifc_req;
	int nInterfaces = ifc.ifc_len / sizeof(struct ifreq);
	for (int i = 0; i < nInterfaces; i++) {
		struct ifreq* item = &ifr[i];

		NetInterface ni;
		ni.name = item->ifr_name,
		memcpy(&ni.ipaddr_, &item->ifr_addr, sizeof(sockaddr));
		//ni.ipaddr_ = (((struct sockaddr_in *)&item->ifr_addr)->sin_addr);

		if (ioctl(fd, SIOCGIFFLAGS, item) >= 0)
			ni.flags = item->ifr_flags;

		if (ioctl(fd, SIOCGIFBRDADDR, item) >= 0)
			memcpy(&ni.brdaddr_, &item->ifr_broadaddr, sizeof(sockaddr));
			//ni.broadcast = inet_ntoa(((struct sockaddr_in *)&item->ifr_broadaddr)->sin_addr);

		result.push_back(std::move(ni));
	}

	return std::move(result);
}

int main(void)
{
	for (const auto& dev: getNetworkInterfaces()) {
		printf("%s: %s broadcast:%s flags:%s\n",
			dev.name.c_str(),
			dev.ipaddr_str().c_str(),
			dev.brdaddr_str().c_str(),
			dev.flags_str().c_str()
		);
	}

	return 0;
}
