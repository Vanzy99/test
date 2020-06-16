#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

char c[10] = "========\n";

#define BUFF_SIZE 15
 
void log_content(int log_fd, char *msg, ssize_t *poffset){

	char buff[BUFF_SIZE];

	for (size_t idx = 0; idx < strlen(msg) / sizeof(char); ++idx) {
		
		if (idx%20 == 0){
			snprintf(buff, sizeof(buff), "%08ld", idx);
	   		pwrite(log_fd, buff, strlen(buff), *(poffset));
	    	*(poffset) += strlen(buff);
		}

		if (((idx+1)%20 == 0)||((idx+1) == strlen(msg))) {
			snprintf(buff, sizeof(buff), " %02x\n", msg[idx]);
		}else{
			snprintf(buff, sizeof(buff), " %02x", msg[idx]);
		}
	    	
	    	pwrite(log_fd, buff, strlen(buff), *(poffset));
	    	*(poffset) += strlen(buff);
	}	
}

int main(){
	
	int fd = open("txt", O_CREAT|O_WRONLY|O_TRUNC, 0644);
	printf("%d\n", fd);

	char a[9] = "abcdefg\n";
	char b[7] = "defgg\n";
	
	printf("a_len:%ld b_len:%ld\n", strlen(a), strlen(b));

	//int num = 8;



	ssize_t offset = 0;
	//offset += strlen(a);

	//pwrite(fd, b, strlen(b), offset);
	//pwrite(fd, a, strlen(a), 0);
	printf("%d\n", (40%20));

	char mesag[] = "Hello; this is a small test file jjjjjjjjjjjjjjjj aaaaaaaaaaaaaaaaaaaaaaa";
   	
   	//int shown = 0;
   	log_content(fd, mesag, &offset);
   	printf("%zd %ld\n", offset, strlen(mesag));
   	ssize_t conlen = 0; //strlen(mesag);
   	double lineN = conlen / 20.0;
   	printf("%f~%fx(8+1)+lnx3 = %d\n", lineN, ceil(lineN), (int)((ceil(lineN)*9)+(conlen*3)) );
   	

   	
   	//pwrite(fd, c, strlen(c), offset);
   
	
	return 0;
}