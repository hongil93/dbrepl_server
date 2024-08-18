#define BUF_SIZE 4096
#define MAX_EPOLL_EVENT 64
#define MAX_CLIENTS 64

#define SQL_SELECT 101
#define SQL_INSERT 102
#define SQL_COMPARE 103
#define REP_CHECK 201
#define REP_ON 202
#define REP_OFF 203
#define EVT_WARNING 400
#define EVT_ERROR 401
#define EVT_STATUS 402

#define SQL_RESTORE 500
#define SQL_RESTORE2 501



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