#pragma once
#include <windows.h>


namespace ms
{

class Window_Manager
{
public:
  Window_Manager(const int num_row_pixel, const int num_col_pixel);

public:
  HWND main_window(void) const;

private:
  HWND _main_window;
};

} // namespace ms
