#define BUF_SIZE 1024
#define MAX_EPOLL_EVENT 64
#define MAX_CLIENTS 64

#define SQL_SELECT 101
#define REP_CHECK 201
#define REP_ON 202
#define REP_OFF 203
#define EVT_WARNING 400
#define EVT_ERROR 401

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

