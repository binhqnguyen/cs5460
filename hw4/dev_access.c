#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/time.h>
#include <string.h>


#ifndef TEN_MEGA
#define TEN_MEGA 10485760
#endif

#ifndef TEN_MIL
#define TEN_MIL 10000000
#endif

int main( int argc, char* argv[]){
	int opt = -1;
	//check the command line argument
	if (argc != 2){
		printf("Usage: %s <dev number> <write value>\n", argv[0]);
		return -1;
	}
	sscanf(argv[1],"%d", &opt);
	if (opt <= -1 ){
		printf("Parameter %s is not an integer (or less than 0)\n", argv[1]);
		return -1;
	}
	if (opt > 2 || opt < 0){
		printf("Parameter %s is out of range (0,2)\n", argv[1]);
		return -1;
	}

	//switch on the command line argument
	int r_filedesc = -1;	//reading file description
	int w_filedesc = -1;	//writing file description
	char buffer[1];		//1B buffer
	int ticket[1];		//4B buffer, used to store an integer
	char* k_buffer;		//dynamic buffer, use for /dev/null writing.
	int count = 0;		//counter counts up to 10MB
	int last_read = 0;	//number of bytes of read()
	struct timeval start, end;	//start/end time for copying 10MB data.
	long timelapsed = 0;	//Time lasped of the copying process.
	int flag = 0;		//distinguish the first read.

	switch (opt){
	case 0:
		r_filedesc = open("/dev/input/mouse0", O_RDONLY);	//read from /dev/input/mouse0
		if (r_filedesc<0){
			printf("Failed to open file /dev/input/mouse0");
			return -1;
		}
		while(1){
			if (read(r_filedesc, buffer, 1) < 0){
				printf("Error when reading file %d\n", r_filedesc);
				return -1;
			}
			else{
				printf("%d",buffer[0]);
				printf("\n");
			}
		}
		close(r_filedesc);
		break;
	case 1:
		gettimeofday(&start,NULL);
		r_filedesc = open("/dev/urandom", O_RDONLY);	//read from /dev/urandom
		w_filedesc = open("/dev/null", O_WRONLY);	//write to /dev/null
		if ( (w_filedesc < 0) || (w_filedesc < 0) ){
			printf("Failed to open file /dev/urandom or /dev/null\n");
			return -1;
		}
		//copying 10MB from /dev/urandom to /dev/null
		k_buffer = malloc(TEN_MIL*sizeof(char));
		while (count < TEN_MEGA){
				if (flag == 0){	//first read
					last_read = read(r_filedesc, k_buffer, TEN_MIL);
					flag++;
				}
				else{	//following reads, read the remainding bits.
					last_read = read(r_filedesc,k_buffer, TEN_MEGA-last_read);
				}
				if ( last_read < 0 ){	//check if error occurred during read
					printf("Error while reading /dev/urandom\n");
					last_read = 0;
				}
				else{	//successfully read "last_read" bytes.
					//increase the count.
					count += last_read;
					//write to /dev/null
					if ( write(w_filedesc, k_buffer, last_read ) != last_read ){
						printf("Error while writing /dev/null\n");
					}
				}
		}
		gettimeofday(&end,NULL);
		timelapsed = (end.tv_sec*1000000 + end.tv_usec) - (start.tv_sec*1000000 + start.tv_usec);	//get time lapsed.
		printf("Time lapsed %ld\n", timelapsed);
		close(r_filedesc);
		close(w_filedesc);
		break;
	case 2:
		r_filedesc = open("/dev/ticket0", O_RDONLY);
		if ( r_filedesc < 0 ){
			printf("Error while openning /dev/ticket0\n");
			return -1;
		}
		int i = 0;
		int read_retval = 0;
		for (i = 0; i < 10; ++i){
			read_retval = read(r_filedesc, ticket, 4);
			if ( read_retval != 4){
				printf("Error while reading /dev/ticket0, retval = %d\n", read_retval);
				continue;
			}
			printf("%d\n",ticket[0]);
			sleep(1);
		}
		break;
		close(r_filedesc);
	default:
		printf("Unrecognized parameter!\n");
		break;
	}
	return 0;
}




