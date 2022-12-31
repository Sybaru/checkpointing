
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>  
#include <sys/stat.h>
#include <unistd.h>  
#include <assert.h>

int main(int argc, char *argv[]){
	char **target = argv+1;
	struct stat st;

	char buf[1000];  
	buf[sizeof(buf)-1] = '\0';
	snprintf(buf, sizeof buf, "%s/libckpt.so", dirname(argv[0]));
	setenv("LD_PRELOAD", buf, 1);
	setenv("TARGET", argv[1], 1);
	int rc = execvp(target[0], target);	
	
	return 1;
}
