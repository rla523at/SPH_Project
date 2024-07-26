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
  // Frame 초기화 및 새로운 Frame을 그릴 준비
  ImGui_ImplDX11_NewFrame();
  ImGui_ImplWin32_NewFrame();

  ImGui::NewFrame(); // 새로운 Frame을 시작하면서 내부 상태를 초기화

  ImGui::Begin("Scene Control"); // UI 요소들을 추가할 수 있는 윈도우 생성

  const auto frame_per_second = ImGui::GetIO().Framerate;
  const auto ms_per_frame     = 1000.0f / frame_per_second;
  ImGui::Text("Average %.3f ms/frame (%.1f FPS)", ms_per_frame, frame_per_second);

  register_check_box();

  ImGui::End(); // UI 윈도우 정의 종료, Begin과 End 사이에 추가된 UI 요소들이 확정된다.

  ImGui::Render(); //확정된 UI 요소들을 하나의 렌더링 명령 리스트로 변환하고, 화면에 그릴 준비를 마친다.

  ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData()); // GUI 렌더링
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
