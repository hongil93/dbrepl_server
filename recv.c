#include "main.h"
#include "recv.h"
#include <errno.h>
#include <sys/epoll.h>
#include <fcntl.h>

#define ec_log(x) DwDebugLog x;

void type_categorizer(int fd, Packet packet){
	char* send_buf;
	char* time = time_now();
	int* t_ret;
	
	switch(packet.header.type){
		case REQ_CLSV_SQL_SELECT:
			JDRLog((REQUEST, "%s,REQ_CLSV_SQL_SELECT\n", time));
		    send_buf = get_select_all();
			if (send_buf != NULL){
				send_message(fd, RES_CLSV_SQL_SELECT, send_buf);
				JDRLog((RESPONSE, "%s,REQ_CLSV_SQL_SELECT,SUCCESS,SELECT * FROM USER\n", time));
				free(send_buf);
		    	break;
			}else{
				send_message(fd, REQ_SVCL_EVT_WARNING, "ALL DB IS DOWN");
				JDRLog((RESPONSE, "%s,REQ_CLSV_SQL_SELECT,FAIL,ALL DB IS DOWN\n", time));
				break;
			}
        case REQ_CLSV_DB_COMPARE:
			JDRLog((REQUEST, "%s,REQ_CLSV_DB_COMPARE\n", time));
            send_buf = compare_table(atoi(packet.buf));
			if (send_buf != NULL){
				// printf("compare len: %d\n", strlen(send_buf));
				send_message(fd, RES_CLSV_DB_COMPARE, send_buf);
				JDRLog((RESPONSE, "%s,REQ_CLSV_DB_COMPARE,SUCCESS,DB0%sDATA\n", time, packet.buf));
				free(send_buf);
		    	break;
			}else{
				send_message(fd, REQ_SVCL_EVT_WARNING, "ALL DB IS DOWN");
				JDRLog((RESPONSE, "%s,REQ_CLSV_DB_COMPARE,FAIL,ALL DB IS DOWN\n", time));
				break;
			}

		case REQ_CLSV_EVT_STATUS:
			JDRLog((REQUEST, "%s,REQ_CLSV_EVT_STATUS\n", time));
			send_server_status(fd);
			break;

		case REQ_CLSV_REP_CHECK:
			JDRLog((REQUEST, "%s,REQ_CLSV_REP_CHECK\n", time));
			send_repl_status(fd);
			break;
			
		case REQ_CLSV_STAT_RECENT:
			JDRLog((REQUEST, "%s, REQ_CLSV_STAT_RECENT\n", time));
			send_recent_stat(fd);
			break;
		
		case REQ_CLSV_DB_SYNC:
			JDRLog((REQUEST, "%s,REQ_CLSV_DB_SYNC\n", time));

			if(pthread_create(&db_sync_t, NULL, db_sync, packet.buf)!=0){
				printf("cannot create file_check thread\n");
				JDRLog((RESPONSE, "%s,REQ_CLSV_DB_SYNC,FAIL,cannot create db_sync thread \n", time));
				break;
			}else{
				printf("check_file thread created\n");
				if(pthread_join(db_sync_t, (void**)&t_ret)!=0){
					printf("join_error\n");
				}
			}
			if (t_ret == NULL){
				send_message(fd, RES_CLSV_DB_SYNC, "fail");
				JDRLog((RESPONSE, "%s,REQ_CLSV_DB_SYNC,FAIL,cannot sync db\n", time));
			}else{
				send_message(fd, RES_CLSV_DB_SYNC, "success");
				JDRLog((RESPONSE, "%s,REQ_CLSV_DB_SYNC,SUCCESS\n", time));
			}
			
			break;
		case REQ_CLSV_EVT_EXIT:
			ec_log((DEB_DEBUG, ">>>[EVT] Receive Exit Request\n", NULL));
			broadcast_message("Server received exit event", REQ_SVCL_EVT_WARNING);
			gpcb->running_flag = 0;
			break;

		default:
			printf("unknown Type\n");
			break;
	}
	free(time);
}

int recv_packet(int sock, Packet *msg) {
    int total_received = 0;
    int header_size = sizeof(msg->header);
    int received = recv(sock, &msg->header, header_size, 0);
    if (received <= 0) {
        return received;  // Error or connection closed
    }

    total_received += received;
    int remaining_data = msg->header.length;

    // Clear buffer
    memset(msg->buf, 0, BUF_SIZE);

    while (remaining_data > 0) {
        received = recv(sock, msg->buf + total_received - header_size, remaining_data, 0);
        if (received == -1) {
            return -1;  // Error occurred
        } else if (received == 0) {
            break;  // Connection closed
        }

        total_received += received;
        remaining_data -= received;
    }

    msg->buf[total_received - header_size] = '\0';
    return total_received - header_size;  // Return the length of the received data
}

void recv_message(int clfd) {
    Packet msg;
    int total_received = 0;
    int recv_len;
    int header_received = 0;

    while (total_received < sizeof(Header)) {
        recv_len = recv(clfd, ((char*)&msg.header) + total_received, sizeof(Header) - total_received, 0);
        if (recv_len < 0) {
            if (errno == EAGAIN) {
                continue; 
            } else {
                perror("recv error");
                close(clfd);
                return;
            }
        } else if (recv_len == 0) {
            printf("Client disconnected.\n");
			ec_log((DEB_DEBUG, ">>>[TCP] Client disconnected.\n", NULL));
			remove_client(clfd);
            close(clfd);
            return;
        }
        total_received += recv_len;
    }

    // get buf size
    int total_size = msg.header.length;
    total_received = 0;

    // get buf
    while (total_received < total_size) {
        recv_len = recv(clfd, msg.buf + total_received, total_size - total_received, 0);
        if (recv_len < 0) {
            if (errno == EAGAIN) {
                continue;
            } else {
                perror("recv error");
                close(clfd);
                return;
            }
        } else if (recv_len == 0) {
            printf("Client disconnected.\n");
            close(clfd);
            return;
        }
        total_received += recv_len;
    }

	msg.buf[total_size] = '\0';

    //printf("Received Message. Type: %d, Length: %d, Msg: %s\n", msg.header.type, msg.header.length, msg.buf);
	ec_log((DEB_DEBUG, ">>>[TCP] Received Message Type: %d, Length: %d, Msg: %s", msg.header.type, msg.header.length, msg.buf));
	
	type_categorizer(clfd, msg);
}

int set_non_blocking(int sfd) {
    int flags = fcntl(sfd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl");
        return -1;
    }

	fcntl(sfd, F_SETFL, flags | O_NONBLOCK);

    return 0;
}

int make_connection()
{
	int sfd;
	int clfd;
	int recv_msg;
	int epfd;
	int epcnt;
	struct sockaddr_in address;
	struct epoll_event ev;
	struct epoll_event ep_events[MAX_EPOLL_EVENT];
	int addrlen = sizeof(address);
	char buffer[BUF_SIZE] = {0};
	gpcb->running_flag = 1;
	
	//make socket
	sfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sfd == 0){
		perror("socket fail");
		return 0;
	}
	
	//bind, listen socket
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(33333);

	if (bind(sfd, (struct sockaddr*)&address, sizeof(address)) < 0) {
		perror("bind error");
		return 0;
	}

	if (listen(sfd, 100) < 0){
		perror("listen error");
		return 0;
	}
	
	printf("socket make, bind, listen succenss\n");
	
	epfd = epoll_create1(0);
	if (epfd == -1){
		printf("epoll create error");
		close(sfd);
		return 0;
	}

	ev.events = EPOLLIN;
	ev.data.fd = sfd;
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, sfd, &ev) == -1){
		perror("epoll_ctl: sfd");
		close(sfd);
		close(epfd);
		return 0;
	}
	
	while(1){
		if(gpcb->running_flag == 0){
			exit(0);
		}
		while((epcnt = epoll_wait(epfd, ep_events, MAX_EPOLL_EVENT, -1)) == -1 && errno == EINTR){
			printf("interruped by signal, retry epoll_wait");
			continue;
		}
		if (epcnt == -1){
			perror("epoll wait error");
			close(sfd);
			close(epfd);
			return 0;
		}

		for (int i = 0; i<epcnt; i++){
			if(ep_events[i].data.fd == sfd){
				if((clfd = accept(sfd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0){
					perror("accept error");
					//return 0;
					continue;
				}
				add_client(clfd);

				printf("accepted\n");
				ec_log((DEB_DEBUG, ">>>[TCP] Client connected.\n", NULL));
				if (set_non_blocking(clfd) == -1){
					close(clfd);
					continue;
				}

				ev.events = EPOLLIN;
				ev.data.fd = clfd;
				if (epoll_ctl(epfd, EPOLL_CTL_ADD, clfd, &ev) == -1){
					perror("epoll_ctl: clfd");
					close(clfd);
					continue;
				}

			}else{
				clfd = ep_events[i].data.fd;
				recv_message(clfd);
			}
		}
	}
}

void add_client(int clfd) {
    if (client_list.client_count < MAX_CLIENTS) {
        client_list.client_sockets[client_list.client_count++] = clfd;
        printf("Client added. Total clients: %d\n", client_list.client_count);
    } else {
        printf("Max clients reached. Cannot add more clients.\n");
    }
}

void remove_client(int clfd) {
    for (int i = 0; i < client_list.client_count; i++) {
        if (client_list.client_sockets[i] == clfd) {
            client_list.client_sockets[i] = client_list.client_sockets[--client_list.client_count];
            printf("Client removed. Total clients: %d\n", client_list.client_count);
            break;
        }
    }
    close(clfd);
}

void send_message(int clfd, int type, const char* message) {
	Packet packet;
	packet.header.type = type;
	strncpy(packet.buf, message, BUF_SIZE -1);
    packet.header.length = strlen(packet.buf);

    send(clfd, &packet, sizeof(packet.header) + packet.header.length, 0);

}

void broadcast_message(const char* message, int type) {
    for (int i = 0; i < client_list.client_count; i++) {
        send_message(client_list.client_sockets[i], type, message);
    }
}
