#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define x 5

typedef struct sss_t
{
	int *ar;
	int ii;
	char car[3];
}ST;

void dothis(void* obj){
	int* l = (int*) obj;
	for (int i = 0; i < 5; ++i)
	{
		printf("%d ", l[i]);
	}
	printf("dothis done\n");

}

void dothat(void* obj){
	ST* stl = (ST*) obj;
	for (int i = 0; i < 5; ++i)
	{
		printf("%d ", stl[i].ii);
	}
	printf("dothat done\n");

}

int main()
{	

	ST ss;
	ST sp;	

	//stuct test array
	int a[5] ={1,2,3,4,5};
	printf("%ld\n", sizeof(a));
	dothis(&a);

	ST stlist[5];

	for (int i = 0; i < 5; ++i)
	{
		stlist[i].ii = a[i];
		printf("%d ", stlist[i].ii);
	}
	printf("done loop\n");
	dothat(&stlist);

	//a[4] = 888;
	ss.ar = &a[0];
	printf("%ld\n",sizeof(&ss.ar));
	sp.ar = a;
	ss.ar[0] = 1;
	sp.ar[1] = 965;
	printf("%d %d %d\n", ss.ar[0], ss.ar[1], ss.ar[4]);
	printf("%d %d %d\n", sp.ar[0], sp.ar[1], sp.ar[4]);

	///struct test :int
	char buff[4096];
	int in = open("abc", O_RDONLY);
	
	ss.ii = in;
	printf("%d\n", ss.ii);
	int r = read(ss.ii, buff, sizeof(buff));
	write(1, buff, r);

	/* global test
	printf("%d\n", x);
	#undef x
	#define x sp.ar[1]
	//x = 6;
	printf("%d\n", x);

	*/
	return 0;
}