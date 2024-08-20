#include "main.h"
#include "recv.h"

#define ec_log(x) DwDebugLog x;



int send_server_status(int fd){
    char server_status[BUF_SIZE];
    char* db01_status = "DOWN";
	char* db02_status = "DOWN";
    int client_cnt = client_list.client_count;
    char* replstatus = "ERROR";

	if (gpcb->db01.status == 1){
		db01_status = "ON";
	}

	if (gpcb->db02.status == 1){
		db02_status = "ON";
	}

    if (gpcb->repl_status){
        replstatus = "OK";
    } else {
        replstatus = "ERROR";
    }

    snprintf(server_status, BUF_SIZE, \
    "=======SERVER_STATUS=======\n"\
	"DB01: %s\n"\
	"DB02: %s\n"\
    "Clients: %d\n"\
    "Replication: %s\n"\
    "===========================\n\n"\
    , db01_status, db02_status, client_cnt, replstatus);

    send_message(fd, EVT_STATUS, server_status);
    JDRLog((RESPONSE, "%s,EVT_STATUS,SUCCESS,DB01:%s,DB02:%s,Clients:%d,Replication:%s\n", time_now(), db01_status, db02_status, client_cnt, replstatus));

    return 0;
}

int send_repl_status(int fd){
    char error_log[BUF_SIZE];
    char res[BUF_SIZE * 2];

    if(gpcb->db01.status == 0 && gpcb->db02.status == 0){
        sprintf(res, "ALL DB DOWN\n");
        send_message(fd, REP_CHECK, res);
        JDRLog((RESPONSE, "%s,REP_CHECK,FAIL,%s\n", time_now(), "ALL_DB_DOWN"));
        return 0;
    }

    snprintf(res, BUF_SIZE, \
    "=======DB01 MASTER STATUS=======\n"\
    "File: %s\n"\
    "Position: %s\n"\
    "=======DB01 SLAVE STATUS=======\n"\
    "File: %s\n"\
    "Position: %s\n"\
    "Slave_IO_Running: %s\n"\
    "Slave_SQL_Running: %s\n\n"\
    "=======DB02 MASTER STATUS=======\n"\
    "File: %s\n"\
    "Position: %s\n"\
    "=======DB02 SLAVE STATUS=======\n"\
    "File: %s\n"\
    "Position: %s\n"\
    "Slave_IO_Running: %s\n"\
    "Slave_SQL_Running: %s\n"\
    , gpcb->db01.repl_master_status.File, gpcb->db01.repl_master_status.Position
    , gpcb->db01.repl_slave_status.Master_Log_File, gpcb->db01.repl_slave_status.Read_Master_Log_Pos, gpcb->db01.repl_slave_status.Slave_IO_Running, gpcb->db01.repl_slave_status.Slave_SQL_Running
    , gpcb->db02.repl_master_status.File, gpcb->db02.repl_master_status.Position
    , gpcb->db02.repl_slave_status.Master_Log_File, gpcb->db02.repl_slave_status.Read_Master_Log_Pos, gpcb->db02.repl_slave_status.Slave_IO_Running, gpcb->db02.repl_slave_status.Slave_SQL_Running
    );

    snprintf(error_log, BUF_SIZE, \
    "\n=======DB01 ERROR=======\n"\
    "Last_IO_Errno: %s\n"\
    "Last_IO_Error: %s\n\n"\
    "Last_SQL_Errno: %s\n"\
    "Last_SQL_Error: %s\n\n"\
    "=======DB02 ERROR=======\n"\
    "Last_IO_Errno: %s\n"\
    "Last_IO_Error: %s\n\n"\
    "Last_SQL_Errno: %s\n"\
    "Last_SQL_Error: %s\n\n"\
    , gpcb->db01.repl_slave_status.Last_IO_Errno, gpcb->db01.repl_slave_status.Last_IO_Error, gpcb->db01.repl_slave_status.Last_SQL_Errno, gpcb->db01.repl_slave_status.Last_SQL_Error
    , gpcb->db02.repl_slave_status.Last_IO_Errno, gpcb->db02.repl_slave_status.Last_IO_Error, gpcb->db02.repl_slave_status.Last_SQL_Errno, gpcb->db02.repl_slave_status.Last_SQL_Error
    );
    strncat(res, error_log, sizeof(error_log));

    send_message(fd, REP_CHECK, res);
    JDRLog((RESPONSE, "%s,REP_CHECK,SUCCESS,%s\n", time_now(), ""));
    return 0;
}

int send_recent_stat(int fd){
    char res[BUF_SIZE];
    int all_request_cnt=0;
    int all_response_cnt=0;

    int five_min_req=0;
    int five_min_res=0;

    all_request_cnt = all_request("/home/kim/project/dbrepl_server/jdr/req_done");
    all_response_cnt = all_request("/home/kim/project/dbrepl_server/jdr/res_done");

    five_min_req = five_min_request("/home/kim/project/dbrepl_server/jdr/req_done");
    five_min_res = five_min_request("/home/kim/project/dbrepl_server/jdr/res_done");    
    
    snprintf(res, BUF_SIZE, \
    "\n=======RECENT STATISTICS=======\n"\
    "All request: %d\n"\
    "All response: %d\n"\
    "5min request: %d\n"\
    "5min response: %d\n"\
    , all_request_cnt, all_response_cnt, five_min_req, five_min_res
    );
    send_message(fd, STAT_RECENT, res);
    JDRLog((RESPONSE, "%s,STAT_RECENT,SUCCESS,5minrequest:%d,5minresponse:%d,\n", time_now(), five_min_req, five_min_res));
    return 0;

}

/* show all table */
int send_server_showtb(int fd){
    char showtb[BUF_SIZE];
    char* replstatus = get_show_tb();

    snprintf(showtb, BUF_SIZE, \
    "======= SHOW TABLES =======\n"\
	"%s"\
    "===========================\n"\
    , replstatus);

    send_message(fd, SQL_SHOW_TB, showtb);
    free(replstatus);
}

/* show table list */
int send_server_showtb_list(int fd, char *buf){
    char showtblist[BUF_SIZE];
    char* replstatus = get_show_tb_list(buf);

    snprintf(showtblist, BUF_SIZE, \
    "======== TABLE LIST =======\n"\
	"%s"\
    "===========================\n"\
    , replstatus);

    send_message(fd, SQL_SHOW_TB_LIST, showtblist);
    free(replstatus);
}

