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
char* compare_table(int);
void* check_file(void*);
char* time_now();
int get_repcheck_status();
void get_replication_on(int fd);
void get_replication_off(int fd);
void get_sql_select_all(int fd, const char *query);
void get_sql_insert_table(int fd, const char *query);
int repl_status();
char *db_info_to_string(DB_INFO * db_info);
char *get_show_tb();
char *get_show_tb_list(char *buf);
char *get_show_tb_del(char *buf);

//function recv.c
int make_connection();
void broadcast_message(const char*, int);
void add_client(int);
void remove_client(int);
void send_message(int, int, const char*);
void broadcast_message(const char*, int);

//send.c
int send_repl_status(int);
int send_recent_stat(int);
int send_server_status(int);
int send_server_showtb(int);
int send_server_showtb_list(int fd, char *buf);
int send_server_del(int fd, char *buf);

//util.c
char* time_now();
int count_files(const char*);
int all_request(const char*);
int five_min_request(const char*);