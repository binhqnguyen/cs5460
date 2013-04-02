#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/time.h>
#include <string.h>

int main( int argc, char* argv[]){
	int dev_n = -1;
	int write_val;
	char dev_name[20];
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

	int w_filedesc = -1;	//writing file description
	strcpy(dev_name,"/dev/sleepy");
	printf ("%s\n",dev_name);
	char d_n[2];
	sprintf(d_n,"%d",dev_n);
	strcat (dev_name,d_n);
	printf ("%s\n",dev_name);
	w_filedesc = open(dev_name, O_WRONLY);	//write to /dev/sleepy0
	//printf("%d\n",w_filedesc);
	if ( (w_filedesc < 0)){
		printf("Failed to open file %s\n",dev_name);
		return -1;
	}	
	//write to /dev/
	int w_ret = write(w_filedesc, &write_val,4);
	if ( w_ret < 0){
		printf("Error while writing %s. Return %d\n",dev_name,w_ret);
	}
	else if (w_ret == 0){/*sleepy dev woken up normally*/
		printf("%s woken up normally\n",dev_name);
	} else {
		printf("%s woken up abnormally. Time left = %d\n",dev_name, w_ret);
	}
	close(w_filedesc);
	return 0;
}




