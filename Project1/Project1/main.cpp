#include <iostream>
#include <memory>
#include <windows.h>

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

  ms::SPH sph(cptr_device, cptr_context);

  MSG msg = {0};
  while (WM_QUIT != msg.message)
  {
    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
    else
    {
      device_manager.prepare_render();
      GUI_manager.render();

      sph.update(cptr_context);
      sph.render(cptr_context);

      device_manager.switch_buffer();
    }
  }

  return 0;
}
