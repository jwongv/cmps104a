

GCC        = g++ -g -O0 -Wall -Wextra -std=gnu++0x
MKDEPS     = g++ -MM -std=gnu++0x
VALGRIND   = valgrind --leak-check=full --show-reachable=yes

MKFILE     = Makefile
DEPSFILE   = Makefile.deps
SOURCES    = auxlib.cc stringset.cc oc.cc
HEADERS    = auxlib.h stringset.h
OBJECTS    = ${SOURCES:.cc=.o}
EXECBIN    = oc
SRCFILES   = ${HEADERS} ${SOURCES} ${MKFILE}
SMALLFILES = ${DEPSFILE} 
CHECKINS   = ${SRCFILES} ${SMALLFILES}

all : ${EXECBIN}

${EXECBIN} : ${OBJECTS}
	${GCC} -o${EXECBIN} ${OBJECTS}

%.o : %.cc
	${GCC} -c $<

ci :
	cid + ${CHECKINS}
	checksource ${CHECKINS}

clean :
	- rm ${OBJECTS}

spotless : clean
	- rm ${EXECBIN}

${DEPSFILE} :
	${MKDEPS} ${SOURCES} >${DEPSFILE}

deps :
	- rm ${DEPSFILE}
	${MAKE} --no-print-directory ${DEPSFILE}
