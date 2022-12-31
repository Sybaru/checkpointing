#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <ucontext.h>
#include <assert.h>
#include <sys/mman.h>

#define NAME_LEN 80

struct ckpt_seg {
  void *start;
  void *end;
  char rwxp[4];
  char name[NAME_LEN]; 
  int is_register; 
	int size;  
};

void readckpt(int fd, struct ckpt_seg meta[]) {
	int rc = read(fd, &meta[0], sizeof(meta[0]));
	if (rc == -1) {
		perror("read");
		exit(1);
	}
	while (rc < sizeof(meta[0])) {
		rc += read(fd, &meta[0] + rc, sizeof(meta[0]) - rc);
	}
	assert(rc == sizeof(meta[0]));

	int i = 1;
	while (rc != 0) {
		lseek(fd, meta[i-1].size, SEEK_CUR);

		rc = read(fd, &meta[i], sizeof(meta[i]));
		if (rc == 0) {
			break;
		}
		while (rc < sizeof(meta[i])) {
			rc += read(fd, &meta[i] + rc, sizeof(meta[i]) - rc);
		}
		assert(rc == sizeof(meta[i]));
	
		i = i + 1;
	}

	int j;
	for (j = 0; j < i; j++) {
		if (meta[j].is_register == 1) {
			printf("%d; name: %s\n", j, meta[j].name);
			printf("\tregister_context: %d\n", meta[j].is_register);
		}
		else {
			printf("%d; name: %s\n", j, meta[j].name);
			printf("\tstart and end: %p - %p\n", meta[j].start, meta[j].end);
			printf("\trwxp: %c%c%c\n", meta[j].rwxp[0], 
				meta[j].rwxp[1], 
				meta[j].rwxp[2]);
			printf("\tis_register: %d\n",
					meta[j].is_register);
			printf("\tsize: %d\n", meta[j].size);
		}
	}
	
	close(fd);
}

int main() {
	struct ckpt_seg meta[1000];
	int fd = open("myckpt.dat", O_RDONLY);
	if (fd == -1) { perror("open"); }
	readckpt(fd, meta); 
	return 0;
}