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
#include <sys/stat.h>
#include "elnutil.h"
#include "inifile.h"
#include "debug.h"
#include "db.h"
#include "jdrlog.h"


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

enum jdr_type{
	DB = 0,
	SQL,
	REQUEST,
	RESPONSE

};

//init Global
PCB gpcb[1];
pthread_t check_db_status_t;
pthread_t check_file_t;

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
void* check_db(void*);
char* get_select_all();
int set_main_db(int, int, MYSQL*);
void connect_main_db(int, MYSQL*);
int get_db_data(int);
MYSQL* connect_db(MYSQL*, int);
void compare_table(int, int);
void* check_file(void*);
char* time_now();

//function recv.c
int make_connection();
void broadcast_message(const char*, int);
void add_client(int);
void remove_client(int);
void send_message(int, int, const char*);
void broadcast_message(const char*, int);
//void type_categorizer(int);


//util.c
char* time_now();