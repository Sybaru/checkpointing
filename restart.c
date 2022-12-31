#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <ucontext.h>
#include <fcntl.h>
#include <sys/types.h>
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

void restart() {
	ucontext_t old_context;

	int fd = open("myckpt.dat", O_RDONLY);
	if (fd == -1) { perror("open"); }

	struct ckpt_seg meta[1000];

	int i = 0;
	int rc = 0;
	while (1) {
		rc = read(fd, &meta[i], sizeof(meta[i]));
		if (rc == 0) { 
			break;
		}
		while (rc < sizeof(meta[i])) {
			rc += read(fd, &meta[i] + rc, sizeof(meta[i]) - rc);
		}
		assert(rc == sizeof(meta[i]));
		
		if (meta[i].is_register == 1) {
			rc = read(fd, &old_context, sizeof(old_context));
			while (rc < sizeof(old_context)) {
				rc += read(fd, &old_context + rc, sizeof(old_context) - rc);
			}
			assert(rc == sizeof(old_context));
		}
		else { 
			void *add = mmap(meta[i].start, meta[i].size, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_FIXED|MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
			if (add == MAP_FAILED) { perror("mmap"); }			
				
			rc = read(fd, (char *)add, meta[i].size);
			while (rc < meta[i].size) {
				rc += read(fd, (char *)add + rc, meta[i].size - rc);
			}
			assert(rc == meta[i].size);
		}
		i = i + 1;
	}
	close(fd);
	if (setcontext(&old_context) == -1) {
		perror("setcontext");
		exit(1);
	}
}

void recursive(int level) {
    if (level > 0) {
        recursive(level - 1);
    } else {
        restart();
    }
}

int main() {
    recursive(1000);
    return 0;
}