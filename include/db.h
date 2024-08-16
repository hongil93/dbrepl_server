#include <stdio.h>
#define MAX_HOST 128
#define MAX_USERNAME 64
#define MAX_PASSWORD 64
#define MAX_DBNAME 64


typedef struct _db_info{
	MYSQL* conn;
	char host[MAX_HOST];
	char username[MAX_USERNAME];
	char password[MAX_PASSWORD];
	char dbname[MAX_DBNAME];
	int port;
	int idx;
	int status;

} DB_INFO;

typedef struct _db_list{
	DB_INFO* db01;
	DB_INFO* db02;
} DB_LIST;

