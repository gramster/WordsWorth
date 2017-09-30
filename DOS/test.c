#include <stdio.h>

main()
{
	unsigned int i;
	for (i=0;i<65000;i+=250)
		printf("%d,%ld\n", (i&0xE000)>>13, ((unsigned long)(i))&0x1FFF);
}


