#include "Window_Manager.h"

#include "../_lib/_header/msexception/Exception.h"
#include "windowsx.h"
#include <imgui.h>

// imgui_impl_win32.cpp�� ���ǵ� �޽��� ó�� �Լ��� ���� ���� ����
// VCPKG�� ���� IMGUI�� ����� ��� �����ٷ� ��� �� �� ����
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace ms
{

LRESULT __stdcall Window_Manager::window_procedure(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
    return true;

  switch (msg)
  {
  case WM_SIZE:
  {
    _size.num_pixel_width  = static_cast<size_t>(GET_X_LPARAM(lParam));
    _size.num_pixel_height = static_cast<size_t>(GET_Y_LPARAM(lParam));
    break;
  }
  case WM_SYSCOMMAND:
    if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
      return 0;
    break;
  case WM_MOUSEMOVE:
  {
    _mouse_pos.x = static_cast<size_t>(LOWORD(lParam));
    _mouse_pos.y = static_cast<size_t>(HIWORD(lParam));
    break;
  }
  case WM_LBUTTONUP:
    // cout << "WM_LBUTTONUP Left mouse button" << endl;
    break;
  case WM_RBUTTONUP:
    // cout << "WM_RBUTTONUP Right mouse button" << endl;
    break;
  case WM_KEYDOWN:
    _is_key_pressed[wParam] = true;
    break;
  case WM_KEYUP:
    _is_key_pressed[wParam] = false;
    break;
  case WM_DESTROY:
    ::PostQuitMessage(0);
    return 0;
  }

  return ::DefWindowProc(hWnd, msg, wParam, lParam);
}

bool Window_Manager::is_Wkey_pressed(void)
{
  // https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
  return _is_key_pressed[0x57];
}

bool Window_Manager::is_Akey_pressed(void)
{
  return _is_key_pressed[0x41];
}

bool Window_Manager::is_Skey_pressed(void)
{
  return _is_key_pressed[0x53];
}

bool Window_Manager::is_Dkey_pressed(void)
{
  return _is_key_pressed[0x44];
}

float Window_Manager::mouse_x_pos_NDC(void)
{
  // ���콺 Ŀ���� x ��ġ(pixel) -> NDC�� ��ȯ
  // [0, width-1] -> [-1, 1]
  return 2.0f / (_size.num_pixel_width - 1) * _mouse_pos.x - 1.0f;
}

float Window_Manager::mouse_y_pos_NDC(void)
{
  // ���콺 Ŀ���� y ��ġ(pixel) -> NDC�� ��ȯ
  // [0, height-1] -> [-1, 1]
  // ��, 0 -> 1, height-1 -> -1
  return -2.0f / (_size.num_pixel_height - 1) * _mouse_pos.y + 1.0f;
}

Window_Manager::Window_Manager(const int num_row_pixel, const int num_col_pixel)
{
  WNDCLASSEX wc;
  wc.cbSize        = sizeof(WNDCLASSEX);               // �� ����ü�� ũ��(����Ʈ)�� �����ϴ� ������.
  wc.style         = CS_CLASSDC;                       // ������ Ŭ������ ��Ÿ���� �����մϴ�.
  wc.lpfnWndProc   = Window_Manager::window_procedure; // ������ �޼����� ó���� ������ ���ν����� �����͸� �����մϴ�.
  wc.cbClsExtra    = 0;                                // Ŭ������ ���� �߰��� ����Ʈ�� �����մϴ�.
  wc.cbWndExtra    = 0;                                // �������� ���� �߰��� ����Ʈ�� �����մϴ�.
  wc.hInstance     = GetModuleHandle(NULL);            // �ش� ������ Ŭ������ ���� ���ø����̼��� �ν��Ͻ� �ڵ��� �����ϴ� ������.
  wc.hIcon         = NULL;                             // �������� �����ϴ� ������.
  wc.hCursor       = NULL;                             // ������ Ŭ�������� �⺻���� ����� Ŀ���� �����ϴ� �����̴�.
  wc.hbrBackground = NULL;                             // �������� ����� ĥ�� �� ����� �귯�ø� �����ϴ� �����̴�.
  wc.lpszMenuName  = NULL;                             // ������ Ŭ������ ����� �޴��� �̸��� �����ϴ� �����̴�.
  wc.lpszClassName = L"MSGraphics";                    // ������ Ŭ������ �̸��� �����ϴ� �����̴�.
  wc.hIconSm       = NULL;                             // ���� �������� �����ϴ� �����̴�.

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

  std::fill(_is_key_pressed, _is_key_pressed + _num_key, false);
}

HWND Window_Manager::main_window(void) const
{
  return _main_window;
}

} // namespace ms