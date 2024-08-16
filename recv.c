#include "main.h"
#include "recv.h"
#include <errno.h>
#include <sys/epoll.h>
#include <fcntl.h>

#define ec_log(x) DwDebugLog x;

void type_categorizer(int fd, Packet packet){
	char* send_buf;
	time_t now = time(NULL);
	char datetime[30];
	strftime(datetime, sizeof(datetime), "[%Y-%m-%d]%H:%M:%S", localtime(&now));

	switch(packet.header.type){
		case SQL_SELECT:
			JDRLog((REQUEST, "%s,SQL_SELECT\n", datetime));
		    send_buf = get_select_all();
			if (send_buf != NULL){
				send_message(fd, SQL_SELECT, send_buf);
				JDRLog((RESPONSE, "%s,SQL_SELECT,SUCCESS,SELECT * FROM USER\n", time_now()));
				free(send_buf);
		    	break;
			}else{
				send_message(fd, EVT_WARNING, "ALL DB IS DOWN");
				JDRLog((RESPONSE, "%s,SQL_SELECT,FAIL,ALL DB IS DOWN\n", time_now()));
				break;
			}
        case SQL_COMPARE:
			JDRLog((REQUEST, "%s,DB_COMPARE\n", datetime));
            get_db_data(2);
			if(pthread_create(&check_file_t, NULL, check_file, "/home/kim/backup/db02_data.csv")!=0){
				printf("cannot create file_check thread\n");
				JDRLog((RESPONSE, "%s,DB_COMPARE,FAIL,cannot create file_check thread \n", datetime));
				break;
			}else{
				printf("check_file thread created\n");
				if(pthread_join(check_file_t, NULL)!=0){
					printf("join_error\n");
				}
			}

            send_buf = compare_table(atoi(packet.buf));
			if (send_buf != NULL){
				send_message(fd, SQL_COMPARE, send_buf);
				JDRLog((RESPONSE, "%s,DB_COMPARE,SUCCESS,DB0%sDATA\n", datetime, packet.buf));
				free(send_buf);
		    	break;
			}else{
				send_message(fd, EVT_WARNING, "ALL DB IS DOWN");
				JDRLog((RESPONSE, "%s,DB_COMPARE,FAIL,ALL DB IS DOWN\n", time_now()));
				break;
			}
            break;
			
		default:
			printf("unknown Type\n");
			break;
	}
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
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
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
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
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

    printf("Received Message. Type: %d, Length: %d, Msg: %s\n", 
           msg.header.type, msg.header.length, msg.buf);
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

    // flags |= O_NONBLOCK;
    // if (fcntl(sfd, F_SETFL, flags) == -1) {
    //     perror("fcntl");
    //     return -1;
    // }

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
		epcnt = epoll_wait(epfd, ep_events, MAX_EPOLL_EVENT, -1);
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

	return 0;
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
