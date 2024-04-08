// Visual Studio 2022 x86 Release Mode
// Server_Socket.cpp

#include "pch.h"			// 컴파일 속도 향상
#include <stdio.h>			// sprintf_s 함수 사용
#define _USE_INIT_WINDOW_	// 윈도우 전역 속성 초기화 함수 직접 지정
#include "tipsware.h"		// EasyWin32 사용

// 이 프로그램은 특별한 메시지 사용하지 않음
NOT_USE_MESSAGE

// 윈도우 전역 속성 초기화 함수
void InitWindow()
{
	// 창 제목 설정
	gp_window_title = "로컬 서버 소켓";
	// 윈도우 속성 변경
	g_wnd_style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN;
}

// 서버에 접속하는 사용자를 관리하기 위한 구조체
struct UserData
{
	unsigned int h_socket; // 소켓 핸들
	char ip_address[16]; // 접속한 클라이언트의 주소
};

// 새로운 클라이언트 접속 -> 이 함수 호출
// ap_user_data -> 접속한 클라이언트 정보
// ap_server -> 서버 객체의 주소
void OnNewUser(UserData *ap_user_data, void *ap_server, int a_server_index)
{
	char temp_str[64];
	sprintf_s(temp_str, 64, "새로운 사용자가 %s에서 접속을 했습니다!", ap_user_data->ip_address);
	// 접속된 클라이언트 주소가 담긴 문자열을 리스트 박스(ctrl_id -> 1000)에 추가
	ListBox_InsertString(FindControl(1000), -1, temp_str);
}

// 클라이언트가 데이터 전송 -> 이 함수 호출
int OnClientMessage(CurrentServerNetworkData *ap_data, void *ap_server, int a_server_index)
{
	// 전달한 사용자 정보 -> 자신이 선언한 구조체로 형 변환
	// 서버가 만들어질 때 sizeof(UserData) 크기로 만들어 달라고 지정
	// 내부적으로 사용자 정보는 UserData 형식으로 관리됨
	UserData *p_user_data = (UserData *)ap_data->mp_net_user;
	char temp_str[128];
	// 채팅 데이터가 전달됨
	if (1 == ap_data->m_net_msg_id) {
		// 누가 전송했는 지 확인 -> 채팅을 전송한 클라이언트의 인터넷 주소
		// 채팅 데이터 앞에 붙여서 채팅 내용 재구성
		sprintf_s(temp_str, 128, "%s : %s", p_user_data->ip_address, ap_data->mp_net_body_data);
		// 재구성된 채팅 내용을 리스트 박스(a_ctrl_id -> 1000)에 추가
		ListBox_InsertString(FindControl(1000), -1, temp_str);
		// 접속한 모든 클라이언트에게 채팅 내용 다시 전송
		BroadcastFrameData(ap_server, 1, temp_str, strlen(temp_str) + 1);
	}
	return 1;
}

// 클라이언트가 접속 해제 -> 이 함수 호출
void OnCloseUser(UserData *ap_user_data, void *ap_server, int a_error_flag, int a_server_index)
{
	char temp_str[64];
	if (1 == a_error_flag) {
		sprintf_s(temp_str, 64, "%s에서 접속한 사용자를 강제로 접속 해제했습니다.", ap_user_data->ip_address);
	}
	else {
		sprintf_s(temp_str, 64, "%s에서 사용자가 접속을 해제하였습니다.", ap_user_data->ip_address);
	}
	// 클라이언트의 해제 상태를 리스트 박스(a_ctrl_id -> 1000)에 추가
	ListBox_InsertString(FindControl(1000), -1, temp_str);
}

int main()
{
	ChangeWorkSize(620, 240); // 작업 영역을 설정
	Clear(0, RGB(72, 87, 114)); // 윈도우 배경을 RGB(72,87,114)로 설정
	StartSocketSystem(); // 이 프로그램이 소켓 시스템을 사용하겠다고 설정
	
	// UserData 구조체를 사용하는 서버를 생성
	// 이 서버는 자신의 상태에 따라 위에서 정의한
	// OnNewUser, OnClientMessage, OnCloseUser 함수 호출하여 작업 진행
	void* p_server = CreateServerSocket(sizeof(UserData), OnNewUser, OnClientMessage, OnCloseUser);
	// "172.30.1.6"에서 25001번 포트로 서버 서비스를 시작함
	StartListenService(p_server, "172.30.1.6", 25001);
	
	CreateListBox(10, 30, 600, 200, 1000); // 1000번 아이디를 가진 리스트 박스 생성
	SelectFontObject("굴림", 12); // '굴림', 12로 지정
	TextOut(15, 10, RGB(200, 255, 200), "사용자 채팅글 목록"); // 리스트 박스의 제목
	ShowDisplay();
	return 0;
}