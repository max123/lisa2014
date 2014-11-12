
TOP= ../..

include $(TOP)/Makefile.master

.KEEP_STATE:

MOD=stack

OBJS=	stack.o

MOD_CONF= $(MOD:%=%.conf)

include $(TOP)/Makefile.driver

all: $(OBJ32) $(INST)/$(OBJ32)/stacktest

CFLAGS += -D$(INST) $(ENVCPPFLAGS1) $(ENVCPPFLAGS2)

DDICTFLAGS += -D_SYSCALL32

LINTFLAGS += -D_SYSCALL32

$(INST)/$(OBJ32)/stacktest: stacktest.c stack.h
	cc -O stacktest.c -o $(INST)/$(OBJ32)/stacktest

