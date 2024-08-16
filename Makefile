TARGET = repl
SRCS = main.c db.c init.c recv.c util.c
OBJS = $(SRCS:.c=.o)
CC = gcc

LIB_DIR = /home/kim/libeluon/lib
DB_LIB_DIR = /usr/lib64/
INC_DIR1 = /home/kim/libeluon/include
INC_DIR2 = /home/kim/project/dbrepl_server/include
INC_DIR3 = /usr/include/mysql
CFLAGS = -c $(IFLAGS)
#LDFLAGS = -L$(LIB_DIR) -leluonutil
#LDFLAGS = $(mysql_config --libs)
LDFLAGS = -L$(DB_LIB_DIR) -lmariadb -L$(LIB_DIR) -leluonutil
#IFLAGS = -I$(INC_DIR1) -I$(INC_DIR2)
IFLAGS = -I$(INC_DIR1) -I$(INC_DIR2) -I$(INC_DIR3)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LDFLAGS) $(IFLAGS) -pthread
	rm -f tags
	ctags -R /home/kim/libeluon/* ./include/
	ctags -R

%.o: %.c
#	$(CC) -std=c99 $(CFLAGS) $< -o $@ 
	$(CC) -std=gnu99 $(CFLAGS) $< -o $@ 

clean:
	rm -f $(OBJS) $(TARGET)
	rm -f tags

.PHONY: all clean

