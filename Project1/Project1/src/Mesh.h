#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <directxtk/SimpleMath.h>

//Forward Declaration
namespace ms
{
class Camera;
class GUI_Manager;
} // namespace ms

//Abbreviation
namespace ms
{
using Microsoft::WRL::ComPtr;
using DirectX::SimpleMath::Matrix;
}

namespace ms
{

class Mesh
{
public:
  virtual void update(const Camera& camera, const ComPtr<ID3D11DeviceContext> cptr_context) = 0;
  virtual void render(const ComPtr<ID3D11DeviceContext> cptr_context)                       = 0;
  virtual void register_GUI_component(GUI_Manager& GUI_manager)                             = 0;
  virtual void set_model_matrix(const Matrix& m_model)                                      = 0;
};

} // namespace ms