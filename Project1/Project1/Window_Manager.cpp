#include "Window_Manager.h"

#include <imgui.h>

#include "../_lib/_header/msexception/Exception.h"


// imgui_impl_win32.cpp에 정의된 메시지 처리 함수에 대한 전방 선언
// VCPKG를 통해 IMGUI를 사용할 경우 빨간줄로 경고가 뜰 수 있음
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace ms
{

// RegisterClassEx()에서 실제로 등록될 콜백 함수
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
    return true;

  switch (msg)
  {
  case WM_SIZE:
    // Reset and resize swapchain
    break;
  case WM_SYSCOMMAND:
    if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
      return 0;
    break;
  case WM_MOUSEMOVE:
    // cout << "Mouse " << LOWORD(lParam) << " " << HIWORD(lParam) << endl;
    break;
  case WM_LBUTTONUP:
    // cout << "WM_LBUTTONUP Left mouse button" << endl;
    break;
  case WM_RBUTTONUP:
    // cout << "WM_RBUTTONUP Right mouse button" << endl;
    break;
  case WM_KEYDOWN:
    // cout << "WM_KEYDOWN " << (int)wParam << endl;
    break;
  case WM_DESTROY:
    ::PostQuitMessage(0);
    return 0;
  }

  return ::DefWindowProc(hWnd, msg, wParam, lParam);
}

Window_Manager::Window_Manager(const int num_row_pixel, const int num_col_pixel)
{
  WNDCLASSEX wc;
  wc.cbSize        = sizeof(WNDCLASSEX);    // 이 구조체의 크기(바이트)를 지정하는 변수다.
  wc.style         = CS_CLASSDC;            // 윈도우 클래스의 스타일을 지정합니다.
  wc.lpfnWndProc   = WndProc;               // 윈도우 메세지를 처리할 윈도우 프로시저의 포인터를 지정합니다.
  wc.cbClsExtra    = 0;                     // 클래스의 끝에 추가할 바이트를 지정합니다.
  wc.cbWndExtra    = 0;                     // 윈도우의 끝에 추가할 바이트를 지정합니다.
  wc.hInstance     = GetModuleHandle(NULL); // 해당 윈도우 클래스가 속한 애플리케이션의 인스턴스 핸들을 지정하는 변수다.
  wc.hIcon         = NULL;                  // 아이콘을 지정하는 변수다.
  wc.hCursor       = NULL;                  // 윈도우 클래스에서 기본으로 사용할 커서를 지정하는 변수이다.
  wc.hbrBackground = NULL;                  // 윈도우의 배경을 칠할 때 사용할 브러시를 지정하는 변수이다.
  wc.lpszMenuName  = NULL;                  // 윈도우 클래스에 사용할 메뉴의 이름을 지정하는 변수이다.
  wc.lpszClassName = L"MSGraphics";         // 윈도우 클래스의 이름을 지정하는 변수이다.
  wc.hIconSm       = NULL;                  // 작은 아이콘을 지정하는 변수이다.

  {
    const auto result = RegisterClassEx(&wc);
    REQUIRE(result != 0, "window class registration should succeed.");
  }

  RECT rect;
  rect.left   = 0;
  rect.top    = 0;
  rect.right  = num_row_pixel;
  rect.bottom = num_col_pixel;
  AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);

  constexpr int window_pos_x          = 100; // window start top left pos x
  constexpr int window_pos_y          = 100; // window start top left pos y
  const int     window_num_row_pixels = rect.right - rect.left;
  const int     window_num_col_pixels = rect.bottom - rect.top;

  const auto handle = CreateWindow(
    wc.lpszClassName,      // 윈도우 생성에 사용할 윈도우의 클래스 이름을 지정한다.
    L"MSGraphics Example", // 윈도우의 제목을 지정하는 문자열이다.
    WS_OVERLAPPEDWINDOW,   // 윈도우의 스타일을 지정하는 변수이다.
    window_pos_x,          // 윈도우 좌측 상단의 x 좌표
    window_pos_y,          // 윈도우 좌측 상단의 y 좌표
    window_num_row_pixels, // 윈도우 가로 방향 해상도
    window_num_col_pixels, // 윈도우 세로 방향 해상도
    NULL,                  // 부모 윈도우의 핸들을 지정하는 변수이다.
    NULL,                  // 메뉴의 핸들을 지정하는 변수이다.
    wc.hInstance,          // 윈도우를 생성하는 애플리케이션의 인스턴스 핸들을 지정하는 변수이다.
    NULL                   // 윈도우를 생성할 때 사용할 데이터를 지정하는 변수이다.
  );

  REQUIRE(handle != NULL, "window creation should succeed");

  _main_window = handle;

  ShowWindow(_main_window, SW_SHOWDEFAULT);
  UpdateWindow(_main_window);
}

HWND Window_Manager::main_window(void) const
{
  return _main_window;
}

} // namespace ms