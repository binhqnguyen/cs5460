#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/time.h>

int main( int argc, char* argv[]){
	int dev_n = -1;
	int write_val = -1;
	//check the command line argument
	if (argc != 3){
		printf("Usage: %s <dev number> <write value>\n", argv[0]);
		return -1;
	}
	sscanf(argv[1],"%d", &dev_n);
	sscanf(argv[2],"%d", &write_val);
	if (dev_n <= -1 ){
		printf("Device number %s is not an integer (or less than 0)\n", argv[1]);
		return -1;
	}
	if (dev_n < 0 || dev_n > 9){
		printf("Device number %s is out of range (0,9)\n", argv[1]);
		return -1;
	}

	char *k_buffer;
	int w_filedesc = -1;	//writing file description
	switch (dev_n){
	case 0:
		w_filedesc = open("/dev/sleepy0", O_WRONLY);	//write to /dev/sleepy0
		printf("%d\n",w_filedesc);
		if ( (w_filedesc < 0)){
			printf("Failed to open file /dev/sleepy0\n");
			return -1;
		}
		k_buffer = malloc(sizeof(char));
		//write to /dev/sleepy0
		if ( write(w_filedesc, k_buffer, 1) != 1){
			printf("Error while writing /dev/sleepy0\n");
		}
		close(w_filedesc);
		break;
	case 1:
		w_filedesc = open("/dev/sleepy1", O_WRONLY);	//write to /dev/sleepy0
		if ( (w_filedesc < 0) || (w_filedesc < 0) ){
			printf("Failed to open file /dev/sleepy1\n");
			return -1;
		}
		k_buffer = malloc(1*sizeof(char));
		//write to /dev/sleepy1
		if ( write(w_filedesc, k_buffer, 1) != 1 ){
			printf("Error while writing /dev/sleepy1\n");
		}
		close(w_filedesc);
		break;
	case 2:
		w_filedesc = open("/dev/sleepy2", O_WRONLY);	//write to /dev/sleepy2
		if ( (w_filedesc < 0) || (w_filedesc < 0) ){
			printf("Failed to open file /dev/sleepy2\n");
			return -1;
		}
		k_buffer = malloc(1*sizeof(char));
		//write to /dev/sleepy2
		if ( write(w_filedesc, k_buffer, 1) != 1 ){
			printf("Error while writing /dev/sleepy2\n");
		}
		close(w_filedesc);
		break;
	case 3:
		w_filedesc = open("/dev/sleepy3", O_WRONLY);	//write to /dev/sleepy3
		if ( (w_filedesc < 0) || (w_filedesc < 0) ){
			printf("Failed to open file /dev/sleepy3\n");
			return -1;
		}
		k_buffer = malloc(1*sizeof(char));
		//write to /dev/sleepy3
		if ( write(w_filedesc, k_buffer, 1) != 1 ){
			printf("Error while writing /dev/sleepy3\n");
		}
		close(w_filedesc);
		break;
	case 4:
		w_filedesc = open("/dev/sleepy4", O_WRONLY);	//write to /dev/sleepy4
		if ( (w_filedesc < 0) || (w_filedesc < 0) ){
			printf("Failed to open file /dev/sleepy4\n");
			return -1;
		}
		k_buffer = (char*) malloc(1*sizeof(char));
		//write to /dev/sleepy4
		if ( write(w_filedesc, k_buffer, 1) != 1){
			printf("Error while writing /dev/sleepy4\n");
		}
		close(w_filedesc);
		break;
	case 9:
		w_filedesc = open("/dev/sleepy9", O_WRONLY);	//write to /dev/sleepy9
		if ( (w_filedesc < 0) || (w_filedesc < 0) ){
			printf("Failed to open file /dev/sleepy9\n");
			return -1;
		}
		k_buffer = malloc(1*sizeof(char));
		//write to /dev/sleepy9
		if ( write(w_filedesc, k_buffer, 1) != 1 ){
			printf("Error while writing /dev/sleepy9\n");
		}
		close(w_filedesc);
		break;
	default:
		printf("no writing (5,8)\n");
		break;
	}
	return 0;
}




