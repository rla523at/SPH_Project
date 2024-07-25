#pragma once
#include <string>
#include <vector>
#include <windows.h>

namespace ms
{
class Device_Manager;
}

namespace ms
{
class GUI_Manager
{
public:
  GUI_Manager(const HWND main_window, const Device_Manager& device_manager);

public:
  void register_GUI_component(const std::string& name, bool* value_ptr);

public:
  void  render(void) const;
  float delta_time(void) const;

private:
  void register_check_box(void) const;

private:
  std::vector<std::string> _check_box_labels;
  std::vector<bool*>       _check_box_values;
};
} // namespace ms
