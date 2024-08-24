#define BUF_SIZE 2024
#define MAX_EPOLL_EVENT 64
#define MAX_CLIENTS 64

#define SQL_SELECT 101
#define SQL_COMPARE 102
#define SQL_SHOW_TB 103
#define SQL_SHOW_TB_LIST 104
#define SQL_SHOW_TB_DEL 105
#define SQL_SHOW_TB_INT 106
#define REP_CHECK 201
#define REP_ON 202
#define REP_OFF 203
#define EVT_WARNING 400
#define EVT_ERROR 401
#define EVT_STATUS 402

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