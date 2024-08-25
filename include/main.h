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
#include <dirent.h>
#include <sys/types.h>
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
	int repl_status;
	int running_flag;
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
pthread_t db_sync_t;

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
int connect_main_db(int, MYSQL*);
MYSQL* connect_db(MYSQL*, int);
char* compare_table(int);
char* time_now();
int get_repcheck_status();
//void get_replication_on(int fd);
//void get_replication_off(int fd);
void get_sql_select_all(int fd, const char *query);
void get_sql_insert_table(int fd, const char *query);
int repl_status();
void* db_sync(void*);
int repl_prepare(MYSQL*, MYSQL*);

//function recv.c
int make_connection();
void broadcast_message(const char*, int);
void add_client(int);
void remove_client(int);
void send_message(int, int, const char*);
void broadcast_message(const char*, int);

//send.c
int send_server_status(int);
int send_repl_status(int);
int send_recent_stat(int);

//util.c
char* time_now();
int count_files(const char*);
int all_request(const char*);
int five_min_request(const char*);
