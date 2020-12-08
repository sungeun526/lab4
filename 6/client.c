//체팅 메신저 client
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define MAXLINE     1000
#define NAME_LEN    20

char *EXIT_STRING = "exit";
// 소켓 생성 및 서버 연결, 생성된 소켓리턴
int tcp_connect(int af, char *servip, unsigned short port);
void errquit(char *mesg) { perror(mesg); exit(1); }

int main(int argc, char *argv[]) {
	char bufname[NAME_LEN];	// 이름
	char bufmsg[MAXLINE];	// 메시지부분
	char bufall[MAXLINE + NAME_LEN];
	int maxsocket1;	// 최대 소켓번호 + 1
	int s;		// 소켓
	int namelen;	// 이름의 길이
	fd_set read_fds;
	time_t ct;
	struct tm tm;

    // argc[0]: 실행 파일 명, argc[1]: 서버 ip, argc[2]: 포트 번호, argc[3]: 사용자 이름  -> argc 개수가 4개가 아니면 실행 방법 알림
	if (argc != 4) {
		printf("사용법 : %s sever_ip  port name \n", argv[0]);
		exit(0);
	}

	s = tcp_connect(AF_INET, argv[1], atoi(argv[2])); // 소켓 연결 요청 수행
	if (s == -1)
		errquit("tcp_connect fail");

	puts("서버에 접속되었습니다.");
	maxsocket1 = s + 1;
	FD_ZERO(&read_fds); // read_fds의 모든 소켓을 0으로 초기화

	while (1) {
		FD_SET(0, &read_fds); // I/O 변화를 감지할 소켓 선택
		FD_SET(s, &read_fds);
        /*
        select(): 소켓의 I/O 변화를 기다리다가 지정된 I/O 변화가 감시 대상 소켓들 중 하나라도 발생하면 select() 문이 리턴 된다.
        int select (
            int maxfdp1,    // 최대 파일(및 소켓)번호 크기 + 1
            fd_set *readfds,    // 읽기 상태 변화를 감지할 소켓 지정
            fd_set *writefds,   // 쓰기 상태 변화를 감지할 소켓 지정
            fd_set *exceptfds,  // 예외 상태 변화를 감지할 소켓 지정
            struct timeval *tvptr   // select() 시스템 콜이 기다리는 시간, NULL인 경우 지정한 I/O 변화가 발생할 때까지 기다림
        )
        */
		if (select(maxsocket1, &read_fds, NULL, NULL, NULL) < 0)
			errquit("select fail");
		if (FD_ISSET(s, &read_fds)) { // read_fds중 소켓 s에 해당하는 비트가 세트되어 있으면 양수값인 slisten_socket를 리턴
			int nbyte;
			if ((nbyte = recv(s, bufmsg, MAXLINE, 0)) > 0) {    // 소켓으로 부터 데이터 읽음
				bufmsg[nbyte] = 0;
				write(1, "\033[0G", 4);		//커서의 X좌표를 0으로 이동
				printf("%s", bufmsg);		//메시지 출력
				fprintf(stderr, "\033[1;32m");	//글자색을 녹색으로 변경
				fprintf(stderr, "%s>", argv[3]);//내 닉네임 출력

			}
		}
		if (FD_ISSET(0, &read_fds)) {
			if (fgets(bufmsg, MAXLINE, stdin)) {
				fprintf(stderr, "\033[1;33m"); //글자색을 노란색으로 변경
				fprintf(stderr, "\033[1A"); //Y좌표를 현재 위치로부터 -1만큼 이동
				ct = time(NULL);	//현재 시간을 받아옴
				tm = *localtime(&ct);
				sprintf(bufall, "[%02d:%02d:%02d]%s>%s", tm.tm_hour, tm.tm_min, tm.tm_sec, argv[3], bufmsg);//메시지에 현재시간 추가
				/*
                    int send(int s, const void *msg, size_t len, int flags): 연결된 서버나 클라이언트로 데이터를 전송
                    int s: 소켓 디스크립터
                    void *msg: 전송할 데이터
                    size_t len: 데이터의 바이트 단위 길이
                    int flags: MSG_DONTWAIT - 전송할 준비가 전에 대기 상태가 필요하다면 기다리지 않고 -1을 반환하면서 복귀, MSG_NOSIGNAL - 상대방과 연결이 끊겼을 때, SIGPIPE 시그널을 받지 않도록 함
                */
                if (send(s, bufall, strlen(bufall), 0) < 0)
					puts("Error : Write error on socket.");
				if (strstr(bufmsg, EXIT_STRING) != NULL) {
					puts("Good bye.");
					close(s);
					exit(0);
				}
			}
		}
	} // end of while
}

int tcp_connect(int af, char *servip, unsigned short port) {
	struct sockaddr_in servaddr;
	int  s;
	// 소켓 생성
	if ((s = socket(af, SOCK_STREAM, 0)) < 0)
		return -1;
	// 채팅 서버의 소켓주소 구조체 servaddr 초기화
	bzero((char *)&servaddr, sizeof(servaddr)); // 원하는 메모리 영역을 '0'으로 초기화 (초기화를 수행할 메모리 영역의 시작주소, 시작 주소로부터 초기화를 수행할 크기)
	servaddr.sin_family = af;
	inet_pton(AF_INET, servip, &servaddr.sin_addr); // ipv4와 ipv6 주소를 binary 형태에서 사람이 알아보기 쉬운 텍스트 형태로 전환해줌
	servaddr.sin_port = htons(port);

	// 연결요청
    /*
    int connect(int sockfd, const struct sockaddr *serv_addr, socklen_t addrlen): 생성한 소켓을 통해 서버로 접속을 요청
    int sockfd: 소켓 디스크립터
    struct sockaddr *serv_addr: 서버 주소 정보에 대한 포인터
    socklen_t addrlen: struct sockaddr *serv_addr 포인터가 가르키는 구조체의 크기
    */
	if (connect(s, (struct sockaddr *)&servaddr, sizeof(servaddr))
		< 0)
		return -1;
	return s;
}
