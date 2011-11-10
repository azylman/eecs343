###############################################################################
#
# Description:  Makefile
# Author:       Stefan Birrer
#               Northwestern Systems Research Group
#               Department of Computer Science
#               Northwestern University
#
# (C) Copyright 2004, Northwestern University, all rights reserved.
#
###############################################################################

# handin info
TEAM = `whoami`
VERSION = `date +%Y%m%d%H%M%S`
PROJ = kma

COMPETITION = KMA_P2FL

CC = gcc
MV = mv
CP = cp
RM = rm
MKDIR = mkdir
TAR = tar cvf
COMPRESS = gzip
CFLAGS = -g -Wall -O2 -D_GNU_SOURCE -lm

DELIVERY = Makefile *.h *.c DOC
PROGS = kma_dummy kma_rm kma_p2fl kma_mck2 kma_bud kma_lzbud
SRCS = kma.c kpage.c kma_dummy.c kma_rm.c kma_p2fl.c kma_mck2.c kma_bud.c kma_lzbud.c
OBJS = ${SRCS:.c=.o}

all: ${PROGS} competition

competition:
	echo "Using ${COMPETITION} for competition"
	${CC} ${CFLAGS} -DCOMPETITION -D${COMPETITION} -o kma_competition ${SRCS}

competitionAlgorithm:
	echo ${COMPETITION}

analyze:
	gnuplot kma_output.plt

test-reg: handin
	HANDIN=`pwd`/${TEAM}-${VERSION}-${PROJ}.tar.gz;\
	cd testsuite;\
	sh ./run_testcase.sh $${HANDIN};

handin: cleanAll
	${TAR} ${TEAM}-${VERSION}-${PROJ}.tar ${DELIVERY}
	${COMPRESS} ${TEAM}-${VERSION}-${PROJ}.tar

.o:
	${CC} *.c

kma_dummy: ${SRCS}
	${CC} ${CFLAGS} -DKMA_DUMMY -o $@ ${SRCS}

kma_rm: ${SRCS}
	${CC} ${CFLAGS} -DKMA_RM -o $@ ${SRCS}

kma_p2fl: ${SRCS}
	${CC} ${CFLAGS} -DKMA_P2FL -o $@ ${SRCS}

kma_mck2: ${SRCS}
	${CC} ${CFLAGS} -DKMA_MCK2 -o $@ ${SRCS}

kma_bud: ${SRCS}
	${CC} ${CFLAGS} -DKMA_BUD -o $@ ${SRCS}

kma_lzbud: ${SRCS}
	${CC} ${CFLAGS} -DKMA_LZBUD -o $@ ${SRCS}

leak: $(TARGET)
	for exec in ${PROGS}; do \
		echo "Checking $${exec} (press ENTER to start)";\
		read;\
		valgrind -v --show-reachable=yes --leak-check=yes $${exec}; \
	done

clean:
	${RM} -f *.o *~

cleanAll: clean
	${RM} -f ${PROGS} kma_competition kma_output.dat kma_output.png kma_waste.png	
