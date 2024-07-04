#include <iostream>
#include <memory>
#include <windows.h>

#include "Box.h"
#include "SPH.h"

#include "Device_Manager.h"
#include "GUI_Manager.h"
#include "Window_Manager.h"

int main()
{
  constexpr int num_pixel_width  = 1280;
  constexpr int num_pixel_height = 960;

  ms::Window_Manager window_manager(num_pixel_width, num_pixel_height);
  const auto         main_window = window_manager.main_window();

  ms::Device_Manager device_manager(main_window, num_pixel_width, num_pixel_height);
  const auto         cptr_device  = device_manager.device_cptr();
  const auto         cptr_context = device_manager.context_cptr();

  ms::GUI_Manager GUI_manager(main_window, device_manager);

  ms::SPH sph(cptr_device);

  //ms::Box box(cptr_device);
  //box.register_GUI_component(GUI_manager);

  MSG msg = {0};
  while (WM_QUIT != msg.message)
  {
    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
      // message가 있을 때 수행하는 작업
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
    else
    {
      // message가 없을 때 수행하는 작업
      device_manager.prepare_render();
      GUI_manager.render();

      sph.update(cptr_context);
      sph.render(cptr_context);

      //const auto dt           = GUI_manager.delta_time();
      //const auto aspect_ratio = device_manager.aspect_ratio();

      //box.Update(dt, aspect_ratio, cptr_context);
      //box.Render(cptr_context);

      // Switch the back buffer and the front buffer
      // 주의: GUI_MANAGER.render 다음에 Present() 호출 why?
      device_manager.switch_buffer();
    }
  }

  return 0;
}
