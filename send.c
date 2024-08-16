#include "main.h"
#include "recv.h"

#define ec_log(x) DwDebugLog x;

void* send_server_status(void* args){
    char server_status[BUF_SIZE];
    char* cdb_status;
    while(1){
        int client_cnt = client_list.client_count;
        int db_status;
        int len;
        db_status = check_db();

        switch (db_status) {
            case 0:
                cdb_status = "DB01: DOWN, DB02: DOWN";
                ec_log((DEB_ERROR, ">>>[DB] ALL DB DOWN\n", NULL));
                JDRLog((DB, "%s,%s\n" , time_now(),"ALLDOWN"));
                break;
            case 1:
                cdb_status = "DB01: DOWN\nDB02: ON";
                ec_log((DEB_WARN, ">>>[DB] DB01 IS DOWN\n", NULL));
                JDRLog((DB, "%s,%s\n" , time_now(),"DB01DOWN"));
                break;
            case 2:
                cdb_status = "DB01: ON\nDB02: DOWN";
                ec_log((DEB_WARN, ">>>[DB] DB02 IS DOWN\n", NULL));
                JDRLog((DB, "%s,%s\n" , time_now(),"DB02DOWN"));
                break;
            case 3:
                cdb_status = "DB01: ON\nDB02: ON";
                ec_log((DEB_DEBUG, ">>>[DB] ALL DB OK\n", NULL));
                JDRLog((DB, "%s,%s\n" , time_now(), "ALLOK"));
                break;
            default:
                cdb_status = "UNKNOWN DB STATUS";
                
                break;
        }
        
        snprintf(server_status, BUF_SIZE, "=======SERVER_STATUS=======\n%s\nClient Count: %d\n===========================\n", cdb_status, client_cnt);

        if(client_cnt > 0 ){
            broadcast_message(server_status, EVT_STATUS);
        }
        sleep(5);
    }
}