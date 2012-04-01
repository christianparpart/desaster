#include <cstdio>
#include <net/if.h>

int main(int argc, char** argv)
{
	auto list = if_nameindex();

	for (int i = 0; list[i].if_index > 0; ++i) {
		printf("%d: [%d] %s\n", i, list[i].if_index, list[i].if_name);
	}

	return 0;
}
