#include "main.h"
#include "recv.h"

#define ec_log(x) DwDebugLog x;

// void* send_server_status(void* args){
//     char server_status[BUF_SIZE];
//     char* cdb_status;
//     while(1){
//         int client_cnt = client_list.client_count;
//         int db_status;
//         int all_request_cnt;
//         int len;
//         all_request_cnt = all_request("/home/kim/project/dbrepl_server/jdr/req_done");
//         db_status = check_db();

//         switch (db_status) {
//             case 0:
//                 cdb_status = "DB01: DOWN\nDB02: DOWN";
//                 ec_log((DEB_ERROR, ">>>[DB] ALL DB DOWN\n", NULL));
//                 JDRLog((DB, "%s,%s\n" , time_now(),"ALLDOWN"));
//                 break;
//             case 1:
//                 cdb_status = "DB01: DOWN\nDB02: ON";
//                 ec_log((DEB_WARN, ">>>[DB] DB01 IS DOWN\n", NULL));
//                 JDRLog((DB, "%s,%s\n" , time_now(),"DB01DOWN"));
//                 break;
//             case 2:
//                 cdb_status = "DB01: ON\nDB02: DOWN";
//                 ec_log((DEB_WARN, ">>>[DB] DB02 IS DOWN\n", NULL));
//                 JDRLog((DB, "%s,%s\n" , time_now(),"DB02DOWN"));
//                 break;
//             case 3:
//                 cdb_status = "DB01: ON\nDB02: ON";
//                 ec_log((DEB_DEBUG, ">>>[DB] ALL DB OK\n", NULL));
//                 JDRLog((DB, "%s,%s\n" , time_now(), "ALLOK"));
//                 break;
//             default:
//                 cdb_status = "UNKNOWN DB STATUS";
                
//                 break;
//         }
        
//         snprintf(server_status, BUF_SIZE, \
//         "=======SERVER_STATUS=======\n"\
//         "%s\n"\
//         "Client Count: %d\n"\
//         "All Request: %d\n"\
//         "===========================\n"\
//         , cdb_status, client_cnt, all_request_cnt);

//         if(client_cnt > 0 ){
//             broadcast_message(server_status, EVT_STATUS);
//             ec_log((DEB_DEBUG, ">>>[TCP] Send Status\n", NULL));
//         }
//         sleep(5);
//     }
// }

int send_server_status(int fd){
    char server_status[BUF_SIZE];
    char* db01_status = "DOWN";
	char* db02_status = "DOWN";
    int client_cnt = client_list.client_count;

	if (gpcb->db01.status == 1){
		db01_status = "ON";
	}

	if (gpcb->db02.status == 1){
		db02_status = "ON";
	}

    snprintf(server_status, BUF_SIZE, \
    "=======SERVER_STATUS=======\n"\
	"DB01: %s\n"\
	"DB02: %s\n"\
    "Client Count: %d\n"\
    "===========================\n"\
    , db01_status, db02_status, client_cnt);

    send_message(fd, EVT_STATUS, server_status);
}



