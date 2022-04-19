#ifndef _WINDOWS_EXAMPLE_PIXEL_SHADER_H_
#define _WINDOWS_EXAMPLE_PIXEL_SHADER_H_

#include <iostream>
#include <vector>
#include <atlbase.h>
#include <d3d11.h>

class PixelShader
{
public:
  PixelShader(int width, int height);
  virtual ~PixelShader();
  std::vector<uint8_t> Convert(std::vector<uint8_t> buffer);

private:
  void init();

private:
  int width_, height_;

  CComPtr<ID3D11Device> device_;
  CComPtr<ID3D11DeviceContext> device_context_;
  CComPtr<ID3D11RenderTargetView> render_target_view_;
  CComPtr<ID3D11VertexShader> vertex_shader_;
  CComPtr<ID3D11PixelShader> pixel_shader_;
  CComPtr<ID3D11InputLayout> vertex_layout_;
  CComPtr<ID3D11Buffer> vertex_buffer_;
  CComPtr<ID3D11Buffer> index_buffer_;



  CComPtr<ID3D11Texture2D> cpu_nv12_;
  CComPtr<ID3D11Texture2D> gpu_nv12_;
  CComPtr<ID3D11Texture2D> gpu_bgrx_;
  CComPtr<ID3D11Texture2D> cpu_bgrx_;

};
#endif