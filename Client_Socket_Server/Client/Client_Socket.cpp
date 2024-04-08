// Visual Studio 2022 x86 Release Mode
// Client_Socket.cpp

#include "pch.h"
#include <stdio.h>
#define _USE_INIT_WINDOW_
#include "tipsware.h"

// 자신이 사용할 위녿우의 전역 속성을 초기화 하는 함수
void InitWindow()
{
	// 창 제목 수정
	gp_window_title = "로컷 클라이언트 소켓";
	// 윈도우 속성 변경
	g_wnd_style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN;
}

// 프로그램에 사용할 내부 데이터
typedef struct AppData
{
	SOCKET h_socket;	// 서버와 접속 및 통신에 사용될 소켓 핸들
	void *p_event_list; // 프로그램의 상태를 기록할 리스트 박스의 주소
}AD;

// 서버에 접속을 시도하는 함수
void ConnectToServer(AD *ap_app)
{
	// AF_INET 주소 체계를 사용하는 TCP 방식의 소켓 생성
	ap_app->h_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (ap_app->h_socket != INVALID_SOCKET) {
		// ip 주소 체계, 프로그램 식별 번호(포트 번호)
		sockaddr_in addr_data = {AF_INET, htons(25001)};
		// 서버 주소 지정
		addr_data.sin_addr.s_addr = inet_addr("172.30.1.6");
		// 서버 접속을 비동기 방식으로 처리한다.
		// 서버 접속의 결과 -> gh_main_wnd 윈도우로 a_connect_id(26001) 메시지 전송
		WSAAsyncSelect(ap_app->h_socket, gh_main_wnd, 26001, FD_CLOSE);
		// 서버에 접속을 시도	
		// 비동기 모드 -> connect 함수가 접속의 결과가 나올 때까지 대기하지 않고 바로 빠져나옴
		connect(ap_app->h_socket, (sockaddr *)&addr_data, sizeof(addr_data));
	}
}

// 소켓을 닫고 리셋하는 함수
void DisconnectToServer(AD *ap_app, const char *ap_msg)
{
	// 소켓 유효성 검사
	if (ap_app->h_socket != INVALID_SOCKET) {
		closesocket(ap_app->h_socket); // 소켓 제거
		ap_app->h_socket = INVALID_SOCKET; // 소켓 정보 리셋
		ListBox_InsertString(ap_app->p_event_list, -1, ap_msg);
	}
}

// ap_data에 저장된 바이너리 데이터를 바이트 단위로 표시해서 리스트 박스에 담는 함수
void ShowBinaryData(void *ap_list_box, UINT8 *ap_data, int a_data_size)
{
	char str[1024], *p_str = str;
	for (int i = 0; i < a_data_size; ++i) {
		// 바이트 단위로 정숫값을 문자열로 변경하고 추가된 크기만큼 p_str의 주소를 이동
		p_str += sprintf_s(p_str, 8, "%02hhX, ", *ap_data++);
	}
	ListBox_InsertString(ap_list_box, -1, str);
}

// 4바이트 Head를 사용해 데이터를 전송하는 함수
void SendFrameData(SOCKET ah_socket, UINT8 a_msg_id, const void *ap_data, UINT16 a_data_size)
{
	int send_data_size = 4 + a_data_size;
	UINT8 *p_send_data = (UINT8 *)malloc(send_data_size);
	if (p_send_data) {  // 메모리 할당 여부 체크
		*p_send_data = 0x51; // Key 저장
		*(p_send_data + 1) = a_msg_id; // Message ID 저장
		*(UINT16*)(p_send_data + 2) = a_data_size; // 실제 전송할 데이터 크기 저장
		
		memcpy(p_send_data + 4, ap_data, a_data_size);
		send(ah_socket, (char *)p_send_data, send_data_size, 0);
		free(p_send_data);
	}
}

// 서버가 전송한 데이터를 수신할 때 사용하는 함수
int ReceiveData(SOCKET ah_socket, char *ap_data, int a_data_size)
{
	int total_size = 0, read_size, retry_count = 0;
	while (total_size < a_data_size) {
		read_size = recv(ah_socket, ap_data + total_size, a_data_size - total_size, 0);
		if (read_size <= 0) {
			Sleep(10);
			++retry_count;
			if (retry_count > 300) break;
		}
		else {
			retry_count = 0;
			total_size += read_size;
		}
	}
	return total_size;
}

// 컨트롤을 조작했을 때 호출할 함수
void OnCommand(INT32 a_ctrl_id, INT32 a_notify_code, void *ap_ctrl)
{
	if (a_ctrl_id == 1010) {  // '서버에 접속' 버튼을 누른 경우
		AD *p_app = (AD *)GetAppData();
		if (p_app->h_socket == INVALID_SOCKET) {
			ListBox_InsertString(p_app->p_event_list, -1, "서버에 접속을 시도합니다.");
			ConnectToServer(p_app);
		}
	}
	else if (a_ctrl_id == 1011) { // '접속 해제' 버튼을 누른 경우
		AD *p_app = (AD *)GetAppData();
		if (p_app->h_socket != INVALID_SOCKET) {
			closesocket(p_app->h_socket);
			p_app->h_socket = INVALID_SOCKET;
			ListBox_InsertString(p_app->p_event_list, -1, "서버와 연결을 해제했습니다.");
		}
	}
	else if (a_ctrl_id == 1012 || (a_ctrl_id == 1020 && a_notify_code == 1000)) { // '전송' 버튼을 누른 경우
		AD* p_app = (AD*)GetAppData();
		if (p_app->h_socket != INVALID_SOCKET) {
			char str[128];
			GetCtrlName(FindControl(1020), str, 128);
			SendFrameData(p_app->h_socket, 1, str, (UINT16)(strlen(str)+1));
			ListBox_InsertString(p_app->p_event_list, -1, "서버에 데이터를 전송했습니다.");
		}
	}
}

// 윈도우가 종료할 때 호출되는 함수
void OnDestroy()
{
	AD *p_app = (AD *)GetAppData(); // 내부 데이터의 주소를 얻음
	// 소켓이 생성되어 있다면 소켓을 닫음
	if (p_app->h_socket != INVALID_SOCKET) {
		closesocket(p_app->h_socket); 
		p_app->h_socket = INVALID_SOCKET; // 소켓 핸들 값 리셋
		WSACleanup(); // 소켓 라이브러리 사용 중지
	}
}

// 윈도우에 발생하는 일반 메시지를 처리하는 함수
int OnUserMsg(HWND ah_wnd, UINT a_message_id, WPARAM wParam, LPARAM lParam)
{
	if (a_message_id == 26001) {
		AD *p_app = (AD *)GetAppData();
		if (WSAGETSELECTERROR(lParam)) {
			ListBox_InsertString(p_app->p_event_list, -1, "서버에 접속할 수 없습니다.");
			closesocket(p_app->h_socket);
			p_app->h_socket = INVALID_SOCKET;
		}
		else {
			WSAAsyncSelect(p_app->h_socket, ah_wnd, 26002, FD_READ | FD_CLOSE);
			ListBox_InsertString(p_app->p_event_list, -1, "서버에 접속했습니다.");
		}
	}
	else if (a_message_id == 26002) {
		AD *p_app = (AD *)GetAppData();
		if (FD_READ == WSAGETSELECTEVENT(lParam)) {
			WSAAsyncSelect(p_app->h_socket, ah_wnd, 26002, FD_CLOSE);
		
			UINT8 key, msg_id;
			UINT16 body_size;

			if (1 == recv(p_app->h_socket, (char*)&key, 1, 0) && key == 0x51) {
				recv(p_app->h_socket, (char*)&msg_id, 1, 0); // 메시지 id를 얻음
				recv(p_app->h_socket, (char*)&body_size, 2, 0); // 실제로 수신할 데이터 크기를 얻음
				if (body_size > 0) {
					UINT8 *p_body_data = (UINT8 *)malloc(body_size);
					int read_size = ReceiveData(p_app->h_socket, (char *)p_body_data, body_size);
					if (read_size == body_size) {
						ListBox_PrintFormat(p_app->p_event_list, "서버에서 데이터 수신: %d bytes", read_size);
						ShowBinaryData(p_app->p_event_list, p_body_data, read_size);
						if (msg_id == 1) ListBox_InsertString(p_app->p_event_list, -1, (char *)p_body_data);
					}
					else DisconnectToServer(p_app, "수신 오류 -> 서버와 연결을 해제");
					free(p_body_data);
				}
				
			}
			else DisconnectToServer(p_app, "원하지 않는 Head 정보 -> 서버와 연결을 해제");
			WSAAsyncSelect(p_app->h_socket, ah_wnd, 26002, FD_READ | FD_CLOSE);
		}
		else {
			closesocket(p_app->h_socket);
			p_app->h_socket = INVALID_SOCKET;
			ListBox_InsertString(p_app->p_event_list, -1, "서버와 연결이 해제되었습니다.");
		}
	}
	return 0;
}
CMD_USER_MESSAGE(OnCommand, OnDestroy, OnUserMsg)

int main()
{
	ChangeWorkSize(600, 281); // 작업 영역(폭, 높이)을 지정
	Clear(0, RGB(82, 97, 124)); // 윈도우 배경을 그림
	
	WSADATA temp;
	WSAStartup(0x0202, &temp);

	AD *p_app = (AD *)CreateAppData(sizeof(AD));
	p_app->h_socket = INVALID_SOCKET;
	p_app->p_event_list = CreateListBox(5, 5, 590, 240, 1000, NULL, 0);
	
	CreateButton("서버에 접속", 5, 250, 80, 26, 1010);
	CreateButton("접속 해제", 88, 250, 80, 26, 1011);
	CreateButton("전송", 516, 250, 80, 26, 1012);

	void *p = CreateEdit(172, 252, 340, 22, 1020, 0);
	EnableEnterKey(p);

	ShowDisplay();
	return 0;
}