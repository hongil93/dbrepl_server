#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <mysql.h>
#include <pthread.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include "elnutil.h"
#include "inifile.h"
#include "debug.h"
#include "db.h"



//define
#define MAX_FILE_NAME 256

//struct

typedef struct _config{
	char config_file_name[MAX_FILE_NAME];
	char log_file_name[MAX_FILE_NAME];
	int max_file_size;
	int max_log_count;
	int debug_level;
	int max_queue_count;
}CFG;


typedef struct _process_control_block{
	CFG cfg[1];
	DB_INFO db01;
	DB_INFO db02;
}PCB;



//init Global
PCB gpcb[1];
/*
DB_INFO db01;
DB_INFO db02;
*/


//function main.c
void print_db_ver();

//function init.c
void set_config_name(char*);
int read_db_cfg();
int read_log_cfg();

//function db.c
int connecting_db(MYSQL*, MYSQL*);
void selectall(MYSQL*);
void* check_db(void*);
void get_select_all(int);
int set_main_db(int, int, MYSQL*);
void connect_main_db(int, MYSQL*);

//function recv.c
int make_connection();
void broadcast_message(const char*, int);
void add_client(int);
void remove_client(int);
void send_message(int, int, const char*);
void broadcast_message(const char*, int);
//void type_categorizer(int);
