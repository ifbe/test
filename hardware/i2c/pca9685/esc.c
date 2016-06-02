#define BYTE unsigned char
#include<errno.h>
#include<stdio.h>
#include<stdio.h>
#include<fcntl.h>
#include<unistd.h>




void initpca9685()
{
	unsigned char buf[16];
	int ret;

	//hardwarepwm(pca9685)
	buf[0]=0x31;	//sleep
	systemi2c_write(0x40, 0, buf, 1);

	buf[0]=18;		//prescale
	systemi2c_write(0x40, 0xfe, buf, 1);

	buf[0]=0xa1;	//wake
	systemi2c_write(0x40, 0, buf, 1);

	buf[0]=0x4;		//restart
	systemi2c_write(0x40, 1, buf, 1);

	buf[0]=0;		//T=3ms
	buf[1]=0;
	buf[2]=3000 & 0xff;
	buf[3]=3000 >> 8;

	buf[4]=0;		//T=3ms
	buf[5]=0;
	buf[6]=buf[2];
	buf[7]=buf[3];

	buf[8]=0;		//T=3ms
	buf[9]=0;
	buf[10]=buf[2];
	buf[11]=buf[3];

	buf[12]=0;		//T=3ms
	buf[13]=0;
	buf[14]=buf[2];
	buf[15]=buf[3];
	systemi2c_write(0x40, 6, buf, 16);

	int x,y;
	for(y=0;y<16;y++)
	{
		for(x=0;x<16;x++)
		{
			systemi2c_read(0x40, y*16+x, buf+x, 1);
			printf("%.2x ",buf[x]);
		}
		printf("\n");
	}
}

void main()
{
	int ret;
	unsigned char buf[16];

	systemi2c_init();
	initpca9685();

	while(1)
	{
		scanf("%d",&ret);
		buf[2]=ret & 0xff;
		buf[3]=ret >> 8;

		buf[4]=0;		//T=3ms
		buf[5]=0;
		buf[6]=buf[2];
		buf[7]=buf[3];

		buf[8]=0;		//T=3ms
		buf[9]=0;
		buf[10]=buf[2];
		buf[11]=buf[3];

		buf[12]=0;		//T=3ms
		buf[13]=0;
		buf[14]=buf[2];
		buf[15]=buf[3];

		printf("%d,%d\n",buf[2],buf[3]);
		systemi2c_write(0x40,0x8,buf+2,14);
	}
}

