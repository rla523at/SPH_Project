#include "GUI_Manager.h"

#include "Device_Manager.h"
#include "Window_Manager.h"

#include "../../_lib/_header/msexception/Exception.h"
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

namespace ms
{

GUI_Manager::GUI_Manager(const HWND main_window, const Device_Manager& device_manager)
{
  auto cptr_device  = device_manager.device_cptr();
  auto cptr_context = device_manager.context_cptr();

  const auto [num_pixel_width, num_pixel_height] = device_manager.render_size();

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io    = ImGui::GetIO();
  io.DisplaySize = ImVec2(float(num_pixel_width), float(num_pixel_height));
  ImGui::StyleColorsLight();

  // Setup Platform/Renderer backends
  const bool is_DX11_init_success = ImGui_ImplDX11_Init(cptr_device.Get(), cptr_context.Get());
  REQUIRE(is_DX11_init_success, "ImGui_ImplDX11_Init should succeed");

  const bool is_Win32_init_success = ImGui_ImplWin32_Init(main_window);
  REQUIRE(is_Win32_init_success, "ImGui_ImplWin32_Init should succeed");
}

void GUI_Manager::register_GUI_component(const std::string& lable, bool* value_ptr)
{
  _check_box_labels.push_back(lable);
  _check_box_values.push_back(value_ptr);
}

void GUI_Manager::render(void) const
{
  // Frame �ʱ�ȭ �� ���ο� Frame�� �׸� �غ�
  ImGui_ImplDX11_NewFrame();
  ImGui_ImplWin32_NewFrame();

  ImGui::NewFrame(); // ���ο� Frame�� �����ϸ鼭 ���� ���¸� �ʱ�ȭ

  ImGui::Begin("Scene Control"); // UI ��ҵ��� �߰��� �� �ִ� ������ ����

  const auto frame_per_second = ImGui::GetIO().Framerate;
  const auto ms_per_frame     = 1000.0f / frame_per_second;
  ImGui::Text("Average %.3f ms/frame (%.1f FPS)", ms_per_frame, frame_per_second);

  register_check_box();

  ImGui::End(); // UI ������ ���� ����, Begin�� End ���̿� �߰��� UI ��ҵ��� Ȯ���ȴ�.

  ImGui::Render(); //Ȯ���� UI ��ҵ��� �ϳ��� ������ ��� ����Ʈ�� ��ȯ�ϰ�, ȭ�鿡 �׸� �غ� ��ģ��.

  ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData()); // GUI ������
}

float GUI_Manager::delta_time(void) const
{
  return ImGui::GetIO().DeltaTime;
}

void GUI_Manager::register_check_box(void) const
{
  const int num_check_box = static_cast<int>(_check_box_labels.size());

  for (int i = 0; i < num_check_box; ++i)
    ImGui::Checkbox(_check_box_labels[i].data(), _check_box_values[i]);
}

} // namespace ms
