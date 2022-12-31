CC=gcc
CFLAGS=-g3 -O0

all: counting-test ckpt readckpt restart libckpt.so

counting-test: counting-test.c 
	${CC} ${CFLAGS} -rdynamic -o $@ $<

#========================
ckpt: ckpt.c
	${CC} ${CFLAGS} -o $@ $<

libckpt.so: libckpt.o
	${CC} ${CFLAGS} -shared -fPIC -o $@ $< 

libckpt.o: libckpt.c
	${CC} ${CFLAGS} -fPIC -c $<  

readckpt: readckpt.c
	${CC} ${CFLAGS} -o $@ $<

restart: restart.c
	gcc -static \
	       -Wl,-Ttext-segment=5000000 -Wl,-Tdata=5100000 -Wl,-Tbss=5200000 \
	       -g3 -o $@ $<

check: all
	rm -f myckpt.dat
	./ckpt ./counting-test 17 &
	sleep 4
	kill -12 `pgrep --newest counting-test` 
	pkill -9 counting-test
	./restart

clean:
	rm -f a.out counting-test hello-test
	rm -f libckpt.so libckpt.o ckpt restart readckpt myckpt.dat
	rm -f a.out primes-test counting-test
	rm -f proc-self-maps save-restore-memory save-restore.dat

dist: clean
	dir=`basename $$PWD` && cd .. && tar czvf $$dir.tar.gz ./$$dir
	dir=`basename $$PWD` && ls -l ../$$dir.tar.gz

.PRECIOUS: libconstructor%.so constructor%.o
