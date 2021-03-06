#include<math.h>
#include<wiringPi.h>
#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<unistd.h>
#include<signal.h>
#include<sys/stat.h>



//......
struct stat st;
int file=0;
int size;
int bytepersample=0;
int samplepersecond=0;
//44100hz->23ms, 人耳分辨时间间隔0.1s->4410个采样点->一般取2048个采样点
unsigned char* buffer=0;
double real[2048];
double imag[2048];
double strength[2048];




#define PI 3.141592653
void fft(double real[], double imag[], int N, int k, int inv)
{
        int i, j, k1, k2, m, step, factor_step;
        double temp_real, temp_imag, factor_real, factor_imag;

        if(inv != 1 && inv != -1)
        {
                fprintf(stderr, "FFT transformation require: inv = 1\n");
                fprintf(stderr, "FFT inverse transformation require: inv = -1\n");
                return;
        }

        //invert order
        j = 0;
        for ( i = 0; i < N; i ++)
        {
                if(j > i)
                {
                        temp_real = real[j];
                        real[j] = real[i];
                        real[i] = temp_real;

                        temp_imag = imag[j];
                        imag[j] = imag[i];
                        imag[i] = temp_imag;
                }

                //?
                m = N/2;
                while(j >= m && m != 0)
                {
                        j-=m;
                        m>>=1;
                }
                j += m;
        }
        //butterfly computation
        for( i = 0; i < k; i++)
        {
                step = 1 << (i+1);
                factor_step = N >> (i+1);

                factor_real = 1.0;
                factor_imag = 0.0;
                for(j = 0; j < step/2; j++)
                {
                        for(k1 = j; k1 < N; k1 += step)
                        {
                                k2 = k1 + step/2;
                                temp_real = real[k1] + real[k2]*factor_real - imag[k2]*factor_imag;
                                temp_imag = imag[k1] + real[k2]*factor_imag + imag[k2]*factor_real;
                                real[k2] = real[k1] - (real[k2]*factor_real - imag[k2]*factor_imag);
                                imag[k2] = imag[k1] - (real[k2]*factor_imag + imag[k2]*factor_real);
                                real[k1] = temp_real;
                                imag[k1] = temp_imag;
                        }
                        factor_real = inv*cos(-2*PI*(j+1)*factor_step/N);
                        factor_imag = inv*sin(-2*PI*(j+1)*factor_step/N);
                }
        }
        if(inv == -1)
        {
                for(i = 0; i <= N-1; i++)
                {
                        real[i] = real[i]/N;
                        imag[i] = imag[i]/N;
                }
        }
        return;
}




void pwmplay(double real[],double imag[])
{
	int i;
	int maxindex=0;
	int maxvalue=0;
	double freq;
	double period;

	for(i=2;i<1000;i++)
	{
		strength[i]=sqrt(real[i]*real[i] + imag[i]*imag[i]);
		if(maxvalue < strength[i])
		{
			maxindex=i;
			maxvalue=strength[i];
		}
	}
	printf("%d	",(int)strength[maxindex]);

	freq=maxindex*44100.0/2048.0;
	printf("%dhz	",(int)freq);

	if(freq<40) period=25000;
	else if(freq>20000) period=100*1000;
	else period=1000.0*1000.0*1000.0/freq;

	pwmSetRange(period/207); //range at 1000 ticks (20ms)
	pwmWrite(1,period/2/207);
	printf( "%dms\n", (int)(period/1000) );

	usleep(25000);
}

/*
void pwmplay(double real[],double imag[])
{
	int i;
	int j;
	int playtime;
	int timespent;
	int maxindex=0;
	int maxvalue=0;
	double freq=0;
	double period=0;
	double sum=0;
	double time=0;

	for(i=2;i<1000;i++)
	{
		strength[i]=sqrt(real[i]*real[i] + imag[i]*imag[i]);
		if(maxvalue < strength[i])
		{
			maxindex=i;
			maxvalue=strength[i];
		}
	}

	for(i=2;i<1000;i++)
	{
		if(strength[i] < 100)continue;
		if(strength[i] < maxvalue )continue;

		sum += strength[i];
	}

	for(i=2;i<1000;i++)
	{
		if(strength[i] < 100)continue;
		if(strength[i] < maxvalue )continue;

		freq=i*44100.0/2048.0;
		printf("%dhz	",(int)freq);

		if(freq<40) period=25000;
		else if(freq>20000) period=100*1000;
		else period=1000.0*1000.0*1000.0/freq;
		printf( "%dms	", (int)(period/1000) );

		pwmSetRange(period/207); //range at 1000 ticks (20ms)
		pwmWrite(1,period/2/207);

		playtime=(int)( 38000 * strength[i] / sum );
		timespent+=playtime;
		usleep(playtime);
		printf("%dus\n",playtime);
	}

	pwmWrite(1,0);
	usleep(38000-timespent);
	printf("----%dus\n",38000-timespent);
}
*/




static void sig_int(int a)
{
	pwmSetClock(2); //clock at 50kHz (20us tick)
	pwmSetRange(1000); //range at 1000 ticks (20ms)
	pwmWrite(1,0);
	exit(-1);
}
int main(int argc,char** argv)
{
	int ii;
	int ret;
	int sampleleft;
	int sampleright;

	if(argc==1)
	{
		printf("./a.out 1.wav\n");
		return 0;
	}

	//size
	ret=stat(argv[1],&st);
	if(ret<0)
	{
		printf("no such file or directory\n");
		return 0;
	}
	size=st.st_size;
	printf("size=%d\n",size);

	//malloc
	buffer=(unsigned char*)malloc(size+0x1000);
	if(buffer==NULL)
	{
		printf("no enough memory\n");
		return 0;
	}

	//open
	file=open(argv[1],O_RDONLY);
	if(file<=0)
	{
		printf("error@open\n");
		goto freememory;
	}

	//read
	ret=read(file,buffer,size);
	if(ret!=size)
	{
		printf("error@read\n");
		goto closefile;
	}

	//
	samplepersecond=*(unsigned int*)(buffer+0x18);
	bytepersample = *(unsigned short*)(buffer+0x22);
	bytepersample = bytepersample/8;

	//
	wiringPiSetup();
	pinMode(1, PWM_OUTPUT);
	pwmSetMode(PWM_MODE_MS);

	signal(SIGINT,sig_int);
	pwmSetClock(2); //clock at 50kHz (20us tick)
	pwmSetRange(1000); //range at 1000 ticks (20ms)
	pwmWrite(1,0);

	//now it begins
	ret=44;
	while(1)
	{
		if(bytepersample==1)
		{
			sampleleft=buffer[ret];
			ret += 1;

			printf("%x\n",sampleleft);
		}
		else
		{
			for(ii=0;ii<2048;ii++)
			{
				sampleleft = *(signed short*)( buffer + ret );
				sampleright= *(signed short*)( buffer + ret + 2 );

				//printf("%x,%x\n",sampleleft,sampleright);
				real[ii]=sampleleft;
				imag[ii]=0;

				ret+=4;
			}
			ret-=4*512;
		}

		//
		fft(real,imag,2048,11,1);
		pwmplay(real,imag);
		//printf("@%x\n",ret);

		if(ret >= size)break;
	}

closefile:
	close(file);
freememory:
	free(buffer);
}
