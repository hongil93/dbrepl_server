#define BUF_SIZE 2024
#define MAX_EPOLL_EVENT 64
#define MAX_CLIENTS 64

#define SQL_SELECT 101
#define SQL_COMPARE 102
#define REQ_CLSV_SQL_SHOWTB 10102101
#define RES_CLSV_SQL_SHOWTB 20102101
#define REQ_CLSV_SQL_SELECT 10102103
#define RES_CLSV_SQL_SELECT 20102103
#define REQ_CLSV_SQL_DELETE 10102104
#define RES_CLSV_SQL_DELETE 20102104
#define REQ_CLSV_SQL_INSERT 10102105
#define RES_CLSV_SQL_INSERT 20102105
#define REQ_CLSV_REP_CHECK 10102201
#define RES_CLSV_REP_CHECK 20102201
#define REP_ON 202
#define REP_OFF 203
#define REQ_SVCL_EVT_WARNING 10201400
#define RES_SVCL_EVT_WARNING 20201400
#define REQ_CLSV_EVT_ERROR 10102401
#define RES_CLSV_EVT_ERROR 20102401
#define REQ_CLSV_EVT_STATUS 10102402
#define RES_CLSV_EVT_STATUS 20102402

#define SQL_RESTORE 500
#define SQL_RESTORE2 501

#define STAT_RECENT 600

typedef struct _header{
    int length;
    int type;
} Header;

typedef struct _packet{
    Header header;
    char buf[BUF_SIZE];
} Packet;

typedef struct _client_list{
    int client_sockets[MAX_CLIENTS];
    int client_count;
} CLIENT_LIST;

CLIENT_LIST client_list;