#include "pch.h"			// 컴파일 속도 향상
#define _USE_INIT_WINDOW_	// 윈도우 전역 초기화 함수 직접 지정 
#include "tipsware.h"		// EasyWin32 사용

// 윈도우 전역 초기화 함수 
void InitWindow()
{
    // 창 제목 설정
    gp_window_title = "클라이언트 소켓 테스트";
    // 윈도우 속성 변경
    g_wnd_style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN;
}

// 프로그램에서 사용할 내부 데이터      
typedef struct AppData
{
    SOCKET h_socket;      // 서버와 접속 및 통신에 사용할 소켓 핸들
    void* p_event_list;   // 프로그램의 상태를 기록할 리스트 박스의 주소
}AD;

// 서버에 접속을 시도하는 함수
void ConnectToServer(AD* ap_app)
{
    // AF_INET 주소 체계를 사용하는 TCP 방식의 소켓 생성
    ap_app->h_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (ap_app->h_socket != INVALID_SOCKET) {
        // ip 주소 체계, 프로그램 식별 번호(포트 번호)
        sockaddr_in addr_data = { AF_INET, htons(1992) };
        // 서버 주소 지정
        addr_data.sin_addr.s_addr = inet_addr("172.30.1.6");
        // 서버 접속 -> 비동기 방식으로 처리
        // 서버 접속 -> gh_main_wnd 윈도우로 a_connect_id 메시지 전송 
        WSAAsyncSelect(ap_app->h_socket, gh_main_wnd, 26001, FD_CONNECT);
        // 서버에 접속 시도
        // 비동기 모드 -> connect 함수가 접속의 결과가 나올 때까지 대기하지 않고 바로 빠져나온다.
        connect(ap_app->h_socket, (sockaddr*)&addr_data, sizeof(addr_data));
    }
}

// 컨트롤을 조작했을 때 호출할 함수
void OnCommand(INT32 a_ctrl_id, INT32 a_notify_code, void* ap_ctrl)
{
    // '서버에 접속' 버튼을 누른 경우
    if (a_ctrl_id == 1010) {
        // 내부 데이터의 주소를 얻어옴
        AD* p_app = (AD*)GetAppData();
        if (p_app->h_socket == INVALID_SOCKET) { // 접속 상태가 아닌 경우, 서버에 접속을 시도
            ListBox_InsertString(p_app->p_event_list, -1, "서버에 접속을 시도합니다.");
            ConnectToServer(p_app);
        }
    }
}

// 윈도우가 종료될 때 호출될 함수
void OnDestroy()
{
    AD* p_app = (AD*)GetAppData(); // 프로그램의 내부 데이터의 주소를 얻어옴
    if (p_app->h_socket != INVALID_SOCKET) { // 소켓이 생성되어 있다면 
        closesocket(p_app->h_socket); // 소켓을 닫음
        p_app->h_socket = INVALID_SOCKET; // 소켓 핸들 값 리셋
        WSACleanup(); // 소켓 라이브러리 사용 종료
    }
}

// 윈도우에 발생하는 일반 메시지를 처리하는 함수
int OnUserMsg(HWND ah_wnd, UINT a_message_id, WPARAM wParam, LPARAM lParam)
{
    if (a_message_id == 26001) { // 서버 접속 시도에 대한 결과
        AD *p_app = (AD *)GetAppData(); // 내부 데이터 주소를 얻음
        if (WSAGETSELECTERROR(lParam)) { // 서버 접속에 실패한 경우
            ListBox_InsertString(p_app->p_event_list, -1, "서버에 접속할 수 없습니다.");
            closesocket(p_app->h_socket); // 소켓을 닫음
            p_app->h_socket = INVALID_SOCKET; // 소켓 사용 정보 리셋
        }
        else {  // 서버 접속에 성공한 경우
            WSAAsyncSelect(p_app->h_socket, ah_wnd, 26002, FD_CLOSE); // 연결 해제 이벤트 발생 시 26002 메시지 발생하도록 설정
            ListBox_InsertString(p_app->p_event_list, -1, "서버에 접속했습니다."); // 리스트 박스에 서버 접속했다는 문자열 출력
        }
    }
    else if (a_message_id == 26002) {  // 서버와 연결이 종료된느 경우 발생하는 메시지
        AD* p_app = (AD*)GetAppData(); // 프로그램의 내부 데이터 주소를 얻음
        if (WSAGETSELECTEVENT(lParam) == FD_CLOSE) { // 서버와 연결이 해제됨
            closesocket(p_app->h_socket); // 소켓을 닫음
            p_app->h_socket = INVALID_SOCKET; // 소켓 사용 정보 리셋
            ListBox_InsertString(p_app, -1, "서버와 연결이 해제되었습니다."); // 리스트 박스에 서버 해제 문자열 출력
        }
    }
    return 0;
}
CMD_USER_MESSAGE(OnCommand, OnDestroy, OnUserMsg)

// 프로그램 실행하는 메인 함수
int main()
{
    ChangeWorkSize(600, 281); // 작업 영역 설정(폭,높이)
    Clear(0, RGB(82, 97, 124)); // 윈도우 배경색 채움

    WSADATA temp;  // 소켓 구조체 변수 선언
    WSAStartup(0x0202, &temp); // 소켓 라이브러리를 사용 가능 상태로 전환

    AD* p_app = (AD*)CreateAppData(sizeof(AD)); // 데이터를 저장할 내부 메모리
    p_app->h_socket = INVALID_SOCKET; // 소켓 핸들 값이 유효하지 않음으로 지정
    p_app->p_event_list = CreateListBox(5, 5, 590, 240, 1000, NULL, 0); // 상대 목록을 출력할 리스트 박스 생성

    CreateButton("서버에 접속", 5, 250, 80, 26, 1010); // '서버에 접속' 버튼 생성
    CreateButton("접속 해제", 88, 250, 80, 26, 1011); // '접속 해제' 버튼 생성
    CreateButton("전송", 180, 250, 80, 26, 1012); // '전송' 버튼 생성

    ShowDisplay(); // 구성된 정보를 윈도우에 출력
    return 0;
}