#include <iostream>
#include <memory>
#include <windows.h>

#include "Billboard_Sphere.h"
#include "Camera.h"
#include "Device_Manager.h"
#include "GUI_Manager.h"
#include "SPH.h"
#include "Square.h"
#include "Window_Manager.h"

#include <numbers>

int main()
{
  using DirectX::SimpleMath::Matrix;

  constexpr int   num_pixel_width  = 1280;
  constexpr int   num_pixel_height = 960;
  constexpr float pi               = std::numbers::pi_v<float>;

  ms::Window_Manager window_manager(num_pixel_width, num_pixel_height);
  const auto         main_window = window_manager.main_window();

  ms::Device_Manager device_manager(main_window, num_pixel_width, num_pixel_height);
  const auto         cptr_device  = device_manager.device_cptr();
  const auto         cptr_context = device_manager.context_cptr();

  ms::GUI_Manager GUI_manager(main_window, device_manager);

  const float aspect_ratio = device_manager.aspect_ratio();
  ms::Camera  camera(aspect_ratio);

  const wchar_t* texture_file_name = L"rsc/wood_table.jpg";

  std::vector<std::unique_ptr<ms::Mesh>> meshes(2);

  meshes[0] = std::make_unique<ms::Square>(cptr_device, cptr_context, texture_file_name);
  meshes[0]->set_model_matrix(Matrix::CreateScale(10.0) * Matrix::CreateRotationX(0.5 * pi));
  meshes[1] = std::make_unique<ms::Billboard_Sphere>(cptr_device, cptr_context);

  //ms::SPH sph(cptr_device, cptr_context);

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

      camera.update(GUI_manager.delta_time());

      for (const auto& mesh : meshes)
      {
        mesh->update(camera, cptr_context);
        mesh->render(cptr_context);
      }

      //sph.update(cptr_context);
      //sph.render(cptr_context);

      GUI_manager.render();

      device_manager.switch_buffer();
    }
  }

  return 0;
}
