#pragma once
#include <windows.h>

namespace ms
{

struct Window_Size
{
  size_t num_pixel_width  = 0;
  size_t num_pixel_height = 0;
};

struct Mouse_Pos
{
  // pixel index�� �־�����.
  size_t x = 0;
  size_t y = 0;
};

class Window_Manager
{
public:
  static LRESULT WINAPI window_procedure(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
  static bool           is_Wkey_pressed(void);
  static bool           is_Akey_pressed(void);
  static bool           is_Skey_pressed(void);
  static bool           is_Dkey_pressed(void);
  static bool           is_z_key_pressed(void);
  static bool           is_x_key_pressed(void);
  static bool           is_Up_key_pressed(void);
  static bool           is_Down_key_pressed(void);
  static bool           is_Right_key_pressed(void);
  static bool           is_Left_key_pressed(void);
  static bool           is_space_key_pressed(void);
  static float          mouse_x_pos_NDC(void); // normalize device coordinate�� ��ȯ�� ���콺 Ŀ���� x ��ġ
  static float          mouse_y_pos_NDC(void); // normalize device coordinate�� ��ȯ�� ���콺 Ŀ���� y ��ġ

public:
  Window_Manager(const int num_row_pixel, const int num_col_pixel);
  ~Window_Manager(void);

public:
  HWND main_window(void) const;

private:
  static constexpr size_t         _num_key = 256;
  static inline bool              _is_key_pressed[_num_key];
  static inline Window_Size       _size              = {0, 0};
  static inline Mouse_Pos         _mouse_pos         = {0, 0};
  static constexpr const wchar_t* _window_class_name = L"MSGraphics";

  HWND      _main_window;
  HINSTANCE _window_class;
};

// Window_Manager class�� static method���� ���� private���� �����,
// class window manager service�� friend�� �ξ, class window manager service�� ȣ���� �� �ְ� �Ѵ�.
// class window manager service�� Window_Manger�� ���� �������� ���µ� ���Ǵ� ��ü�̴�.
// class Window_Manager_Service

} // namespace ms
