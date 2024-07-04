#include "Window_Manager.h"

#include <imgui.h>

#include "../_lib/_header/msexception/Exception.h"


// imgui_impl_win32.cpp�� ���ǵ� �޽��� ó�� �Լ��� ���� ���� ����
// VCPKG�� ���� IMGUI�� ����� ��� �����ٷ� ��� �� �� ����
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace ms
{

// RegisterClassEx()���� ������ ��ϵ� �ݹ� �Լ�
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
  wc.cbSize        = sizeof(WNDCLASSEX);    // �� ����ü�� ũ��(����Ʈ)�� �����ϴ� ������.
  wc.style         = CS_CLASSDC;            // ������ Ŭ������ ��Ÿ���� �����մϴ�.
  wc.lpfnWndProc   = WndProc;               // ������ �޼����� ó���� ������ ���ν����� �����͸� �����մϴ�.
  wc.cbClsExtra    = 0;                     // Ŭ������ ���� �߰��� ����Ʈ�� �����մϴ�.
  wc.cbWndExtra    = 0;                     // �������� ���� �߰��� ����Ʈ�� �����մϴ�.
  wc.hInstance     = GetModuleHandle(NULL); // �ش� ������ Ŭ������ ���� ���ø����̼��� �ν��Ͻ� �ڵ��� �����ϴ� ������.
  wc.hIcon         = NULL;                  // �������� �����ϴ� ������.
  wc.hCursor       = NULL;                  // ������ Ŭ�������� �⺻���� ����� Ŀ���� �����ϴ� �����̴�.
  wc.hbrBackground = NULL;                  // �������� ����� ĥ�� �� ����� �귯�ø� �����ϴ� �����̴�.
  wc.lpszMenuName  = NULL;                  // ������ Ŭ������ ����� �޴��� �̸��� �����ϴ� �����̴�.
  wc.lpszClassName = L"MSGraphics";         // ������ Ŭ������ �̸��� �����ϴ� �����̴�.
  wc.hIconSm       = NULL;                  // ���� �������� �����ϴ� �����̴�.

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
    wc.lpszClassName,      // ������ ������ ����� �������� Ŭ���� �̸��� �����Ѵ�.
    L"MSGraphics Example", // �������� ������ �����ϴ� ���ڿ��̴�.
    WS_OVERLAPPEDWINDOW,   // �������� ��Ÿ���� �����ϴ� �����̴�.
    window_pos_x,          // ������ ���� ����� x ��ǥ
    window_pos_y,          // ������ ���� ����� y ��ǥ
    window_num_row_pixels, // ������ ���� ���� �ػ�
    window_num_col_pixels, // ������ ���� ���� �ػ�
    NULL,                  // �θ� �������� �ڵ��� �����ϴ� �����̴�.
    NULL,                  // �޴��� �ڵ��� �����ϴ� �����̴�.
    wc.hInstance,          // �����츦 �����ϴ� ���ø����̼��� �ν��Ͻ� �ڵ��� �����ϴ� �����̴�.
    NULL                   // �����츦 ������ �� ����� �����͸� �����ϴ� �����̴�.
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