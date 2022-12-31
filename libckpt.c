#define _GNU_SOURCE /* Required for 'constructor' attribute (GNU extension) */
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

static int in_handler = 0;

struct ckpt_seg {
  void *start;
  void *end;
  char rwxp[4];
  char name[NAME_LEN]; 
  int is_register; 
	int size;  
};

ucontext_t context;

int match_one_line(int proc_maps_fd, struct ckpt_seg *ckpt_seg, char *filename) {
  unsigned long int start, end;
  char rwxp[4];
  char tmp[10];

  int tmp_stdin = dup(0); 
  dup2(proc_maps_fd, 0); 
  int rc = scanf("%lx-%lx %4c %*s %*s %*[0-9 ]%[^\n]\n",
                 &start, &end, rwxp, filename);
  assert(fseek(stdin, 0, SEEK_CUR) == 0);
  dup2(tmp_stdin, 0); 
  close(tmp_stdin);
  if (rc == EOF || rc == 0) { 
    ckpt_seg -> start = NULL;
    ckpt_seg -> end = NULL;
	ckpt_seg->size = 0;
    return EOF;
  }
  if (rc == 3) { 
    strncpy(ckpt_seg -> name,
            "ANONYMOUS_SEGMENT", strlen("ANONYMOUS_SEGMENT")+1);
  } else {
    assert( rc == 4 );
    strncpy(ckpt_seg -> name, filename, NAME_LEN-1);
    ckpt_seg->name[NAME_LEN-1] = '\0';
  }
  ckpt_seg -> start = (void *)start;
  ckpt_seg -> end = (void *)end;
  ckpt_seg -> size = ckpt_seg -> end - ckpt_seg -> start;
  memcpy(ckpt_seg->rwxp, rwxp, 4);
  return 0;
}


int proc_self_maps(struct ckpt_seg proc_maps[]) {
  int proc_maps_fd = open("/proc/self/maps", O_RDONLY);
  char filename [100]; 
  int i = 0;
  int rc = -2; 
  for (i = 0; rc != EOF; i++) {
    rc = match_one_line(proc_maps_fd, &proc_maps[i], filename);
  }
  close(proc_maps_fd);
  return 0;
}

void readmem(ucontext_t *context_p) {
	struct ckpt_seg process[1000];
	assert(proc_self_maps(process) == 0);
	int fd = open("myckpt.dat", O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
	if (fd == -1) {	perror("open");	}

  struct ckpt_seg ck; 
	strcpy(ck.name, "context");
	ck.is_register = 1;
	ck.size = sizeof(ucontext_t);

  int wc = write(fd, (char*)&ck, sizeof(struct ckpt_seg));
	if (wc == -1) { perror("write"); }
	while (wc < sizeof(struct ckpt_seg)) {
		wc += write(fd, (char *)&ck + wc, sizeof(struct ckpt_seg) - wc);
	}
	assert(wc == sizeof(struct ckpt_seg));

  wc = write(fd, context_p, ck.size);
	if (wc == -1) { perror("write"); }
	while (wc < ck.size) {
		wc += write(fd, context_p + wc, ck.size - wc);
	}
	assert(wc == ck.size);

  int i;
	for (i = 0; process[i].start != NULL; i++) {
		if (strcmp(process[i].name, "[vsyscall]") == 0
				|| strcmp(process[i].name, "[vdso]") == 0
				|| strcmp(process[i].name, "[vvar]") == 0) {
			continue;
		}

		if (process[i].rwxp[0]=='-' 
				&& process[i].rwxp[1]=='-' 
				&& process[i].rwxp[2]=='-') {
			continue;
		}

		int wc = write(fd, &process[i], sizeof(struct ckpt_seg));
		if (wc == -1) { perror("write"); }
		while (wc < sizeof(struct ckpt_seg)) {
			wc += write(fd, (char *)&process[i] + wc, sizeof(struct ckpt_seg) - wc);
		}
		assert(wc == sizeof(struct ckpt_seg));

		wc = write(fd, (void*)process[i].start, process[i].end - process[i].start);
		if (wc == -1) { perror("write"); }
		while (wc < process[i].end - process[i].start) {
			wc += write(fd, (void*)process[i].start + wc,
					process[i].end - process[i].start - wc);
		}
		assert(wc == process[i].size);
	}
	close(fd);
}

void sig_handler(int signal) {
	static int restart = 0;
  	int cont = getcontext(&context);
	
  	if (restart == 1) {
    		restart = 0;
			printf("restarting \n");
    		return;
  	}
  	else {
    	restart = 1;
		printf("writing \n");
    	readmem(&context);
  	}
}

void __attribute__((constructor)) my_constructor() {
	signal(SIGUSR2, &sig_handler);
	fprintf(stderr, "running \n");
}


