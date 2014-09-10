CC=gcc
INCLUDE_DIR =inc
CC_FLAG=-Wall -g
SRC=code/src code/util .
OBJ_FLAGS=-Wall -c
LDFLAGS +=-lz
EXECUTABLES=a.out
SOURCES=$(foreach DIR, $(SRC),$(wildcard $(DIR)/*.c))
INCLUDE += $(foreach includedir, $(INCLUDE_DIR),-I $(includedir))
INCLUDE_FILES += $(foreach includedir, $(INCLUDE_DIR),$(includedir)/*.h)
OBJS_FOLDER=.objs
OBJECTS := $(addprefix ${OBJS_FOLDER}/,$(SOURCES:.c=.o))

all: ${EXECUTABLES}

.PHONY: all clean distclean

${OBJS_FOLDER}:
	mkdir -p ${foreach s, ${SRC}, ${OBJS_FOLDER}/${s}}

${EXECUTABLES}: ${OBJS_FOLDER} ${OBJECTS} ${INCLUDE_FILES}
	${CC} ${INCLUDE} ${OPT} ${CC_FLAG} ${LDFLAGS}  ${OBJECTS} -o $@  && ctags -R --c-kinds=+p --fields=+S

#if any header files gets modified compile the whole project again
.objs/%.o: %.c ${INCLUDE_FILES}
	${CC} ${INCLUDE} ${OPT} ${OBJ_FLAGS} $< -o $@

clean:
	 rm -rf ${EXECUTABLES} ${OBJS_FOLDER}
cleanscm:
	rm -rf .scm

distclean: clean

