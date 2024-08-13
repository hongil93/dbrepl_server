#include <stdio.h>
#include <stdlib.h>
#include <mysql.h>
#include "main.h"
#include "recv.h"

#define ec_log(x) DwDebugLog x;

int connecting_test(MYSQL* conn1, MYSQL* conn2)
{
	printf("start connecting_db\n");
    conn1 = mysql_init(NULL);

	if(mysql_real_connect(conn1, "10.0.2.4", "slave_db", "0000", "eluon", 3306, NULL, 0)==NULL)
	{
		fprintf(stderr, "%s\n", mysql_error(conn1));
		printf("Can not connect to DB01");
		mysql_close(conn1);
	}

	printf("DB01 is ON\n");
	
	mysql_close(conn1);

	conn2 = mysql_init(NULL);

	if(mysql_real_connect(conn2, "10.0.2.5", "slave_db", "0000", "eluon", 3306, NULL, 0)==NULL)
	{
		fprintf(stderr, "%s\n", mysql_error(conn2));
		printf("Can not connect to DB02");
		mysql_close(conn2);
	}

    printf("DB02 is ON\n");

	mysql_close(conn2);

	return 0;	
}

void* check_db(void* args)
{

	DB_INFO *db01 = &(gpcb->db01);
	DB_INFO *db02 = &(gpcb->db02);
	//DB_INFO *db01 = &gpcb->db01;
	//DB_INFO *db02 = &gpcb->db02;
	MYSQL* conn1 = db01->conn;
	MYSQL* conn2 = db02->conn;
	while(1)
	{
		conn1 = mysql_init(NULL);
		conn2 = mysql_init(NULL);

		if(mysql_real_connect(conn1, 
					db01->host,
					db01->username, 
					db01->password,
					db01->dbname,
					db01->port,
					NULL,
					0)==NULL)
		{
			fprintf(stderr, "%s\n", mysql_error(conn1));
			gpcb->db01.status = 0;
			printf("Can not connect to DB01\n");
			mysql_close(conn1);
		}else{
			gpcb->db01.status = 1;
			printf("DB01 is ON\n");
			mysql_close(conn1);
		}

		if(mysql_real_connect(conn2, 
					db02->host,
					db02->username, 
					db02->password,
					db02->dbname,
					db02->port,
					NULL,
					0)==NULL)
		{
			fprintf(stderr, "%s\n", mysql_error(conn2));
			gpcb->db02.status = 0;
			printf("Can not connect to DB02\n");
			mysql_close(conn2);
		}else{
			gpcb->db02.status = 1;
			printf("DB02 is ON\n");
			mysql_close(conn2);
		}

        if(gpcb->db01.status == 0 && gpcb->db02.status == 0){
            broadcast_message("ALL DB IS DOWN", EVT_WARNING);
            ec_log((DEB_ERROR, ">>>[DB] ALL DB IS DOWN\n", NULL));
            sleep(5);
            continue;
        }

        if((gpcb->db01.status) == 0){
            broadcast_message("DB01 IS DOWN", EVT_WARNING);
            ec_log((DEB_WARN, ">>>[DB] DB01 IS DOWN\n", NULL));
            sleep(5);
            continue;
		}
        if((gpcb->db02.status) == 0){
            broadcast_message("DB02 IS DOWN", EVT_WARNING);
            ec_log((DEB_WARN, ">>>[DB] DB02 IS DOWN\n", NULL));
            sleep(5);
            continue;
		}

        if (gpcb->db01.status == 1 && gpcb->db02.status ==1){
            ec_log((DEB_DEBUG, ">>>[DB] db_check_success\n", NULL));
            sleep(5);
            continue;
		}
	}

}

void get_sql_select_all(int fd, const char *query)
{
    Packet packet;
	MYSQL* conn;
    MYSQL_RES* res;
    MYSQL_ROW row;
	int active_db = set_main_db(gpcb->db01.status, gpcb->db02.status, conn);
	if (active_db == 0){
		printf("all_db_is_down\n");
        send_message(fd, EVT_WARNING, "ALL DB IS DOWN");
        return;
	}

    conn = mysql_init(NULL);
    int i;

    connect_main_db(active_db, conn);

    if (mysql_query(conn, query))
    {
        printf("query fail\n");
        ec_log((DEB_ERROR, ">>>[db] mysql_query_error\n", NULL));
    }

    res = mysql_store_result(conn);

    int num_fields = mysql_num_fields(res);
    int buffer_size = 1024;
    char* result_buffer = (char*)malloc(buffer_size);
    if (result_buffer == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        mysql_free_result(res);
        mysql_close(conn);
        exit(1);
    }

    result_buffer[0] = '\0';

    while ((row = mysql_fetch_row(res)))
    {
        for (i = 0; i < num_fields; i++)
        {

            int field_length = row[i] ? strlen(row[i]) : 4;
            int required_length = strlen(result_buffer) + field_length + 2;

            if (required_length > buffer_size)
            {
                buffer_size *= 2;
                result_buffer = (char*)realloc(result_buffer, buffer_size);
                if (result_buffer == NULL)
                {
                    fprintf(stderr, "Memory reallocation failed\n");
                    mysql_free_result(res);
                    mysql_close(conn);
                    exit(1);
                }
            }

            strcat(result_buffer, row[i] ? row[i] : "NULL");
            strcat(result_buffer, " ");
        }
        strcat(result_buffer, "\n");
    }

    printf("Result:\n%s", result_buffer);
    packet.header.type = SQL_SELECT;
	strncpy(packet.buf, result_buffer, BUF_SIZE -1);
    packet.header.length = strlen(packet.buf);

    send(fd, &packet, sizeof(packet.header) + packet.header.length, 0);
    printf("send select * data\n");
    ec_log((DEB_DEBUG, ">>>[DB] SQL Request :: select * from USER_TB\n", NULL));

    free(result_buffer);
    mysql_free_result(res);
    mysql_close(conn);
}

void selectall(MYSQL* conn)
{
    printf("start selectall\n");
    MYSQL_RES* res;
    MYSQL_ROW row;
    conn = mysql_init(NULL);
    int i;

    if (mysql_real_connect(conn, "10.0.2.4", "slave_db", "0000", "eluon", 3306, NULL, 0) == NULL)
    {
        fprintf(stderr, "%s\n", mysql_error(conn));
        mysql_close(conn);
        exit(0);
    }
    printf("selectall-connected\n");

    if (mysql_query(conn, "select * from USER_TB"))
    {
        printf("query fail\n");
    }

    res = mysql_store_result(conn);

    int num_fields = mysql_num_fields(res);
    int buffer_size = 1024; // �ʱ� ���� ũ��
    char* result_buffer = (char*)malloc(buffer_size);
    if (result_buffer == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        mysql_free_result(res);
        mysql_close(conn);
        exit(1);
    }

    result_buffer[0] = '\0'; // �� ���ڿ��� �ʱ�ȭ

    while ((row = mysql_fetch_row(res)))
    {
        for (i = 0; i < num_fields; i++)
        {
            // �ʿ��ϴٸ� �� �ʵ忡 ���� ���� ũ�⸦ �������� Ȯ��
            int field_length = row[i] ? strlen(row[i]) : 4; // "NULL"�� ���̴� 4
            int required_length = strlen(result_buffer) + field_length + 2; // ����� '\0' ����

            if (required_length > buffer_size)
            {
                buffer_size *= 2;
                result_buffer = (char*)realloc(result_buffer, buffer_size);
                if (result_buffer == NULL)
                {
                    fprintf(stderr, "Memory reallocation failed\n");
                    mysql_free_result(res);
                    mysql_close(conn);
                    exit(1);
                }
            }

            strcat(result_buffer, row[i] ? row[i] : "NULL");
            strcat(result_buffer, " ");
        }
    }

    printf("Result:\n%s", result_buffer);

    free(result_buffer);
    mysql_free_result(res);
    mysql_close(conn);
}

int set_main_db(int db01_st, int db02_st, MYSQL* conn)
{
	if((db01_st)==1){
		conn = gpcb->db01.conn;
        printf("main DB is DB01\n");
        return 1;
	}
	if((db02_st)==1){
		conn = gpcb->db02.conn;
        printf("main DB is DB02\n");
        return 2;
	}
	return 0;
}

void connect_main_db(int activated_db, MYSQL* conn)
{
    DB_INFO *db01 = &(gpcb->db01);
	DB_INFO *db02 = &(gpcb->db02);
    if(activated_db == 1){
        if(mysql_real_connect(conn, 
			db01->host,
			db01->username, 
			db01->password,
			db01->dbname,
			db01->port,
			NULL,
			0)==NULL)
        {
            fprintf(stderr, "%s\n", mysql_error(conn));
            mysql_close(conn);
            exit(0);
        }

    }

    if(activated_db == 2){
        if(mysql_real_connect(conn, 
			db02->host,
			db02->username, 
			db02->password,
			db02->dbname,
			db02->port,
			NULL,
			0)==NULL)
        {
            fprintf(stderr, "%s\n", mysql_error(conn));
            mysql_close(conn);
            exit(0);
        }

    }
}

void print_db_ver()
{
	printf("MySQL Client Version: %s\n", mysql_get_client_info());

}

/* hyogi code */
/* 2. replication check */
void get_repcheck_status(int fd)
{
    Packet packet;
    MYSQL *conn_ptr = mysql_init(NULL);

    // conn init
    if (conn_ptr == NULL) {
        fprintf(stderr, "mysql_init() failed\n");
        return; 
    }
    
    // conn exception
    int active_db = set_main_db(gpcb->db01.status, gpcb->db02.status, conn_ptr);
    if (active_db == 0){
		printf("all_db_is_down\n");
        send_message(fd, EVT_WARNING, "ALL DB IS DOWN");
        mysql_close(conn_ptr);
        return;
	}

    // conn connect
    connect_main_db(active_db, conn_ptr);

    // buffer exception
    char* result_buffer = (char*)malloc(BUF_SIZE);
    if (result_buffer == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        mysql_close(conn_ptr);
        exit(1);
    }

    // buffer last string
    result_buffer[0] = '\0';

    // buffer init
    memset(result_buffer, 0, BUF_SIZE);

    // query exception
    if (mysql_query(conn_ptr, "SHOW SLAVE STATUS"))
    {
        printf("query error: %s\n", mysql_error(conn_ptr));
        return;
    }

    MYSQL_RES *result = mysql_store_result(conn_ptr);
    // result exception
    if (result == NULL)
    {
        fprintf(stderr, "result error: %s\n", mysql_error(conn_ptr));
        return;
    }

    int num_fields = mysql_num_fields(result);
    MYSQL_ROW row;

    while ((row = mysql_fetch_row(result)) != NULL) {
        for (int i = 0; i < num_fields; i++) {
            const char *field_name = mysql_fetch_field_direct(result, i)->name;

            if(strcmp(field_name, "Slave_IO_Running") == 0)
            {   
                printf("=======================================\n");
                printf("\tSlave_IO_Running: %s\n", row[i]);
                strcat(result_buffer, row[i] ? row[i] : "NULL");
                strcat(result_buffer, ","); // 띄어쓰기로 구분
            }

            if (strcmp(field_name, "Slave_SQL_Running") == 0)
			{
				printf("\tSlave_SQL_Running: %s\n", row[i]);
                strcat(result_buffer, row[i] ? row[i] : "NULL");
                strcat(result_buffer, ","); // 띄어쓰기로 구분
			}

            if (strcmp(field_name, "Last_IO_Errno") == 0)
			{
				printf("\tLast_IO_Errno: %s\n", row[i]);
                strcat(result_buffer, row[i] ? row[i] : "NULL");
                strcat(result_buffer, ","); // 띄어쓰기로 구분
			}

            if (strcmp(field_name, "Last_IO_Error") == 0)
			{
                if(strcmp(row[i], "") == 0){
                    strcat(result_buffer, "empty");
                    printf("\tLast_IO_Error: %s\n", "empty");
                }else{
                    strcat(result_buffer, row[i]);
                    printf("\tLast_IO_Error: %s\n", row[i]);
                }
                strcat(result_buffer, ","); // 띄어쓰기로 구분
            }   

            if (strcmp(field_name, "Last_SQL_Errno") == 0)
			{
				printf("\tLast_SQL_Errno: %s\n", row[i]);
                strcat(result_buffer, row[i] ? row[i] : "NULL");
                strcat(result_buffer, ","); // 띄어쓰기로 구분
			}

            if (strcmp(field_name, "Last_SQL_Error") == 0)
			{
                if(strcmp(row[i], "") == 0){
                    strcat(result_buffer, "empty");
                    printf("\tLast_SQL_Error: %s\n", "empty");
                }else{
                    strcat(result_buffer, row[i]);
                    printf("\tLast_SQL_Error: %s\n", row[i]);
                }
                printf("=======================================\n\n");
            }   
        }
    }
    mysql_free_result(result);

    // 패킷 준비
    printf("Result:\n%s\n", result_buffer);
    packet.header.type = REP_CHECK; // 패킷 타입 설정
    strncpy(packet.buf, result_buffer, BUF_SIZE - 1); // 결과 버퍼 복사
    packet.header.length = strlen(packet.buf); // 패킷 길이 설정

    // 데이터 전송
    send(fd, &packet, sizeof(packet.header) + packet.header.length, 0);
    ec_log((DEB_DEBUG, ">>>[REPLIE] Replication_status_success\n", NULL));
    free(result_buffer);
    mysql_close(conn_ptr);
}

/* 3. ON OFF replication */
void get_replication_on(int fd)
{
    Packet packet;
    MYSQL *conn_ptr = mysql_init(NULL);

    // conn init
    if (conn_ptr == NULL) {
        fprintf(stderr, "mysql_init() failed\n");
        return; 
    }
    
    // conn exception
    int active_db = set_main_db(gpcb->db01.status, gpcb->db02.status, conn_ptr);
    if (active_db == 0){
		printf("all_db_is_down\n");
        send_message(fd, EVT_WARNING, "ALL DB IS DOWN");
        mysql_close(conn_ptr);
        return;
	}

    // conn connect
    connect_main_db(active_db, conn_ptr);

    // buffer exception
    char* result_buffer = (char*)malloc(BUF_SIZE);
    if (result_buffer == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        mysql_close(conn_ptr);
        exit(1);
    }

    // buffer last string
    result_buffer[0] = '\0';

    // buffer init
    memset(result_buffer, 0, BUF_SIZE);

    // accept query
    if(mysql_query(conn_ptr, "start slave"))
    {
        printf("query error: %s\n", mysql_error(conn_ptr));
        ec_log((DEB_ERROR, ">>>[REPLIE] Replication_start_error\n", NULL));
        return;
    }

    // restore result
    strcat(result_buffer, "START Replication" ? "START Replication" : "NULL");

    // 패킷 준비
    printf("Result:\n%s\n", result_buffer);
    packet.header.type = REP_ON; // 패킷 타입 설정
    strncpy(packet.buf, result_buffer, BUF_SIZE - 1); // 결과 버퍼 복사
    packet.header.length = strlen(packet.buf); // 패킷 길이 설정

    // 데이터 전송
    send(fd, &packet, sizeof(packet.header) + packet.header.length, 0);
    ec_log((DEB_DEBUG, ">>>[REPLIE] Replication_start_success\n", NULL));
    free(result_buffer);
    mysql_close(conn_ptr);
}
void get_replication_off(int fd)
{
    Packet packet;
    MYSQL *conn_ptr = mysql_init(NULL);

    // conn init
    if (conn_ptr == NULL) {
        fprintf(stderr, "mysql_init() failed\n");
        return; 
    }
    
    // conn exception
    int active_db = set_main_db(gpcb->db01.status, gpcb->db02.status, conn_ptr);
    if (active_db == 0){
		printf("all_db_is_down\n");
        send_message(fd, EVT_WARNING, "ALL DB IS DOWN");
        mysql_close(conn_ptr);
        return;
	}

    // conn connect
    connect_main_db(active_db, conn_ptr);

    // buffer exception
    char* result_buffer = (char*)malloc(BUF_SIZE);
    if (result_buffer == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        mysql_close(conn_ptr);
        exit(1);
    }

    // buffer last string
    result_buffer[0] = '\0';

    // buffer init
    memset(result_buffer, 0, BUF_SIZE);

    // accept query
    if(mysql_query(conn_ptr, "stop slave"))
    {
        printf("query error: %s\n", mysql_error(conn_ptr));
        ec_log((DEB_ERROR, ">>>[REPLIE] Replication_stop_error\n", NULL));
        return;
    }

    // restore result
    strcat(result_buffer, "STOP Replication" ? "STOP Replication" : "NULL");

    // 패킷 준비
    printf("Result:\n%s\n", result_buffer);
    packet.header.type = REP_OFF; // 패킷 타입 설정
    strncpy(packet.buf, result_buffer, BUF_SIZE - 1); // 결과 버퍼 복사
    packet.header.length = strlen(packet.buf); // 패킷 길이 설정

    // 데이터 전송
    send(fd, &packet, sizeof(packet.header) + packet.header.length, 0);
    ec_log((DEB_DEBUG, ">>>[REPLIE] Replication_stop_success\n", NULL));
    free(result_buffer);
    mysql_close(conn_ptr);
}
/* CRUD mysql query */
void get_sql_insert_table(int fd, const char *query)
{
    Packet packet;
    MYSQL *conn_ptr = mysql_init(NULL);

    // conn init
    if (conn_ptr == NULL) {
        fprintf(stderr, "mysql_init() failed\n");
        return; 
    }
    
    // conn exception
    int active_db = set_main_db(gpcb->db01.status, gpcb->db02.status, conn_ptr);
    if (active_db == 0){
		printf("all_db_is_down\n");
        send_message(fd, EVT_WARNING, "ALL DB IS DOWN");
        mysql_close(conn_ptr);
        return;
	}

    // conn connect
    connect_main_db(active_db, conn_ptr);

    // buffer exception
    char* result_buffer = (char*)malloc(BUF_SIZE);
    if (result_buffer == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        mysql_close(conn_ptr);
        exit(1);
    }

    // buffer last string
    result_buffer[0] = '\0';

    // buffer init
    memset(result_buffer, 0, BUF_SIZE);

    // accept query
    if(mysql_query(conn_ptr, query))
    {
        printf("query error: %s\n", mysql_error(conn_ptr));
        ec_log((DEB_ERROR, ">>>[db] mysql_query_error\n", NULL));
        return;
    }
    // restore result
    strcat(result_buffer, "INSERT SUCCESS" ? "INSERT SUCCESS" : "NULL");
    // 패킷 준비
    printf("Result:\n%s\n", result_buffer);
    packet.header.type = SQL_INSERT; // 패킷 타입 설정
    strncpy(packet.buf, result_buffer, BUF_SIZE - 1); // 결과 버퍼 복사
    packet.header.length = strlen(packet.buf); // 패킷 길이 설정

    // 데이터 전송
    send(fd, &packet, sizeof(packet.header) + packet.header.length, 0);
    ec_log((DEB_DEBUG, ">>>[db] mysql_send_success\n", NULL));
    free(result_buffer);
    mysql_close(conn_ptr);
}

