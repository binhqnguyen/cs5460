#include <stdio.h>

int main(){
	#ifdef IS
	printf("abc\n");
	#endif
	printf("def\n");
	return 1;
}
