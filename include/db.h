#include <stdio.h>
#define MAX_HOST 128
#define MAX_USERNAME 64
#define MAX_PASSWORD 64
#define MAX_DBNAME 64
#define MAX_STRING_LENGTH 2048
typedef struct _repl_slave_status{
	char *Master_Log_File;
	char *Read_Master_Log_Pos;
	char *Slave_IO_Running;
	char *Slave_SQL_Running;
	char *Last_IO_Errno;
	char *Last_IO_Error;
	char *Last_SQL_Errno;
	char *Last_SQL_Error;
} REPL_SLAVE_STATUS;

typedef struct _repl_master_status{
	char *File;
	char *Position;
	char *Binlog_Do_DB;
	char *Binlog_Ignore_DB;
} REPL_MASTER_STATUS;

typedef struct _repl_status{
	
}REPL_STATUS;


typedef struct _db_info{
	MYSQL* conn;
	char host[MAX_HOST];
	char username[MAX_USERNAME];
	char password[MAX_PASSWORD];
	char dbname[MAX_DBNAME];
	int port;
	int idx;
	int status;
	REPL_SLAVE_STATUS repl_slave_status;
	REPL_MASTER_STATUS repl_master_status;
} DB_INFO;

typedef struct _db_list{
	DB_INFO* db01;
	DB_INFO* db02;
} DB_LIST;

