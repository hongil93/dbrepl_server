#define BUF_SIZE 2048
#define MAX_EPOLL_EVENT 64
#define MAX_CLIENTS 64

#define REQ_CLSV_SQL_SELECT 10102101
#define RES_CLSV_SQL_SELECT 20102101
#define REQ_CLSV_DB_COMPARE 10102102
#define RES_CLSV_DB_COMPARE 20102102
#define REQ_CLSV_REP_CHECK 10102201
#define RES_CLSV_REP_CHECK 20102201
#define REQ_SVCL_EVT_WARNING 10201400
#define RES_SVCL_EVT_WARNING 20201400
#define REQ_CLSV_EVT_ERROR 10102401
#define RES_CLSV_EVT_ERROR 20102401
#define REQ_CLSV_EVT_STATUS 10102402
#define RES_CLSV_EVT_STATUS 20102402
#define REQ_CLSV_EVT_EXIT 10102444
#define RES_CLSV_EVT_EXIT 20102444
#define REQ_CLSV_STAT_RECENT 10102600
#define RES_CLSV_STAT_RECENT 20102600
#define REQ_CLSV_DB_SYNC 10102700
#define RES_CLSV_DB_SYNC 20102700

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

