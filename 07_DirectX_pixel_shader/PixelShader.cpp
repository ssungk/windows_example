#include "PixelShader.h"
#include <d3dcompiler.h>
#include <directxmath.h>
#include <directxcolors.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

struct SimpleVertex
{
  DirectX::XMFLOAT3 Pos;
  DirectX::XMFLOAT2 Tex;
};

static const char* vertex_shader_code = {
  "struct VS_INPUT                                                  \n"
  "{                                                                \n"
  "  float4 Pos : POSITION;                                         \n"
  "  float2 Tex : TEXCOORD0;                                        \n"
  "};                                                               \n"
  "                                                                 \n"
  "struct PS_INPUT                                                  \n"
  "{                                                                \n"
  "  float4 Pos : SV_POSITION;                                      \n"
  "  float2 Tex : TEXCOORD0;                                        \n"
  "};                                                               \n"
  "                                                                 \n"
  "PS_INPUT VS(VS_INPUT input)                                      \n"
  "{                                                                \n"
  "    return input;                                                \n"
  "}                                                                \n"
};


static const char* pixel_shader_code = {
  "struct PS_INPUT                                                  \n"
  "{                                                                \n"
  "  float4 Pos : SV_POSITION;                                      \n"
  "  float2 Tex : TEXCOORD0;                                        \n"
  "};                                                               \n"
  "                                                                 \n"
  "Texture2D txDiffuse : register(t0);                              \n"
  "Texture2D txDiffuse1 : register(t1);                             \n"
  "                                                                 \n"
  "SamplerState MeshTextureSampler                                  \n"
  "{                                                                \n"
  "  Filter = MIN_MAG_MIP_LINEAR;                                   \n"
  "};                                                               \n"
  "                                                                 \n"
  "static const float3x3 YUVtoRGBCoeffMatrix =                      \n"
  "{                                                                \n"
  "    1.164383f,  1.164383f, 1.164383f,                            \n"
  "    0.000000f, -0.391762f, 2.017232f,                            \n"
  "    1.596027f, -0.812968f, 0.000000f                             \n"
  "};                                                               \n"
  "                                                                 \n"
  "float3 ConvertYUVtoRGB(float3 yuv)                               \n"
  "{                                                                \n"
  "  yuv -= float3(0.062745f, 0.501960f, 0.501960f);                \n"
  "  yuv = mul(yuv, YUVtoRGBCoeffMatrix);                           \n"
  "                                                                 \n"
  "  return saturate(yuv);                                          \n"
  "}                                                                \n"
  "                                                                 \n"
  "                                                                 \n"
  "float4 PS(PS_INPUT input) : SV_Target                            \n"
  "{                                                                \n"
  "  float y = txDiffuse.Sample(MeshTextureSampler, input.Tex);     \n"
  "  float2 uv = txDiffuse1.Sample(MeshTextureSampler, input.Tex);  \n"
  "                                                                 \n"
  "  float3 YUV = float3(y, uv.x, uv.y);                            \n"
  "  float4 YUV4 = float4(YUV.x, YUV.y, YUV.z, 1);                  \n"
  "                                                                 \n"
  "  float3 RGB = ConvertYUVtoRGB(YUV);                             \n"
  "  float4 RGB4 = float4(RGB.x, RGB.y, RGB.z, 1);                  \n"
  "                                                                 \n"
  "  return RGB4;                                                   \n"
  "                                                                 \n"
  "}                                                                \n"
};

PixelShader::PixelShader(int width, int height) :
  width_(width),
  height_(height)
{
  init();
}

PixelShader::~PixelShader()
{

}

std::vector<uint8_t> PixelShader::Convert(std::vector<uint8_t> buffer)
{
  D3D11_MAPPED_SUBRESOURCE  mapped;
  HRESULT hr = device_context_->Map(cpu_nv12_, 0, D3D11_MAP_READ, 0, &mapped);
  if (FAILED(hr))
  {
    printf("context_->Map failed\n");
    std::terminate();
  }
  
  memcpy(mapped.pData, buffer.data(), buffer.size());
  
  device_context_->Unmap(cpu_nv12_, 0);


  device_context_->CopyResource(gpu_nv12_, cpu_nv12_);



  CComPtr<ID3D11RenderTargetView> render_target_view_;
  hr = device_->CreateRenderTargetView(gpu_bgrx_.p, nullptr, &render_target_view_);
  if (FAILED(hr))
  {
    printf("CreateRenderTargetView failed\n");
    std::terminate();
  }

  device_context_->OMSetRenderTargets(1, &render_target_view_.p, nullptr);

  D3D11_VIEWPORT vp;

  vp.Width = (FLOAT)width_;
  vp.Height = (FLOAT)height_;
  vp.TopLeftX = 0;
  vp.TopLeftY = 0;

  vp.MinDepth = 0.0f;
  vp.MaxDepth = 1.0f;

  device_context_->RSSetViewports(1, &vp);

  CComPtr<ID3D11ShaderResourceView> shader_resource_view_;
  shader_resource_view_ = nullptr;

  D3D11_SHADER_RESOURCE_VIEW_DESC srv;
  memset(&srv, 0, sizeof(srv));
  srv.Format = DXGI_FORMAT_R8_UNORM;
  srv.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
  srv.Texture2D.MipLevels = 1;

  hr = device_->CreateShaderResourceView(gpu_nv12_.p, &srv, &shader_resource_view_);
  if (FAILED(hr))
  {
    printf("CreateShaderResourceView failed\n");
    std::terminate();
  }

  srv.Format = DXGI_FORMAT_R8G8_UNORM;

  CComPtr<ID3D11ShaderResourceView> shader_resource_view_2 = nullptr;
  hr = device_->CreateShaderResourceView(gpu_nv12_.p, &srv, &shader_resource_view_2);
  if (FAILED(hr))
  {
    printf("CreateShaderResourceView failed\n");
    std::terminate();
  }

  ID3D11ShaderResourceView* resource_view_array_[2];

  resource_view_array_[0] = shader_resource_view_.p;
  resource_view_array_[1] = shader_resource_view_2.p;

  device_context_->PSSetShaderResources(0, 2, resource_view_array_);



  device_context_->CopyResource(cpu_bgrx_, gpu_bgrx_);

  device_context_->DrawIndexed(6, 0, 0);

  
  hr = device_context_->Map(cpu_bgrx_, 0, D3D11_MAP_READ, 0, &mapped);
  if (FAILED(hr))
  {
    printf("context_->Map failed\n");
    std::terminate();
  }

  std::vector<uint8_t> obuffer(width_ * height_ * 4);

  memcpy(obuffer.data(), mapped.pData, obuffer.size());

  device_context_->Unmap(cpu_bgrx_, 0);





 
  return obuffer;
}

void PixelShader::init()
{
  HRESULT hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION, &device_, NULL, &device_context_);
  if (FAILED(hr))
  {
    printf("D3D11CreateDevice failed\n");
    std::terminate();
  }

  D3D11_TEXTURE2D_DESC texture_desc = { 0 };
  texture_desc.Width = width_;
  texture_desc.Height = height_;
  texture_desc.MipLevels = 1;
  texture_desc.ArraySize = 1;
  texture_desc.Format = DXGI_FORMAT_NV12;
  texture_desc.SampleDesc.Count = 1;
  texture_desc.SampleDesc.Quality = 0;
  texture_desc.Usage = D3D11_USAGE_STAGING;
  texture_desc.BindFlags = 0;
  texture_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;
  texture_desc.MiscFlags = 0;

  hr = device_->CreateTexture2D(&texture_desc, NULL, &cpu_nv12_);
  if (FAILED(hr))
  {
    printf("CreateTexture2D failed\n");
    std::terminate();
  }

  texture_desc.Format = DXGI_FORMAT_NV12;
  texture_desc.Usage = D3D11_USAGE_DEFAULT;
  texture_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
  texture_desc.CPUAccessFlags = 0;

  hr = device_->CreateTexture2D(&texture_desc, NULL, &gpu_nv12_);
  if (FAILED(hr))
  {
    printf("CreateTexture2D failed\n");
    std::terminate();
  }


  texture_desc.Format = DXGI_FORMAT_B8G8R8X8_UNORM;
  texture_desc.Usage = D3D11_USAGE_DEFAULT;
  texture_desc.BindFlags = D3D11_BIND_RENDER_TARGET;
  texture_desc.CPUAccessFlags = 0;

  hr = device_->CreateTexture2D(&texture_desc, NULL, &gpu_bgrx_);
  if (FAILED(hr))
  {
    printf("CreateTexture2D failed\n");
    std::terminate();
  }

  texture_desc.Format = DXGI_FORMAT_B8G8R8X8_UNORM;
  texture_desc.Usage = D3D11_USAGE_STAGING;
  texture_desc.BindFlags = 0;
  texture_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;

  hr = device_->CreateTexture2D(&texture_desc, NULL, &cpu_bgrx_);
  if (FAILED(hr))
  {
    printf("CreateTexture2D failed\n");
    std::terminate();
  }



  ///////////////////////////////////////////////////////

  ///////// vertex shader /////////
  CComPtr<ID3DBlob> vs_blob;
  CComPtr<ID3DBlob> vs_error_blob;

  DWORD flags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
  // Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
  // Setting this flag improves the shader debugging experience, but still allows 
  // the shaders to be optimized and to run exactly the way they will run in 
  // the release configuration of this program.
  flags |= D3DCOMPILE_DEBUG;

  // Disable optimizations to further improve shader debugging
  flags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

  std::string vsstr(vertex_shader_code);
  hr = D3DCompile(vsstr.c_str(), vsstr.length(), nullptr, nullptr, nullptr, "VS", "vs_5_0", flags, 0, &vs_blob, &vs_error_blob);
  if (FAILED(hr))
  {
    printf("D3DCompile failed");
    std::terminate();
  }

  //HRESULT hr = D3DCompileFromFile(L"vs.hlsl", nullptr, nullptr, "VS", "vs_5_0", flags, 0, &vs_blob, &vs_error_blob);
  //CHECK_HR(hr, "D3DCompileFromFile");

  vertex_shader_ = nullptr;
  hr = device_->CreateVertexShader(vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), nullptr, &vertex_shader_);
  if (FAILED(hr))
  {
    printf("CreateVertexShader failed");
    std::terminate();
  }

  D3D11_INPUT_ELEMENT_DESC layout[] =
  {
      { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
      { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT   , 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
  };

  UINT elements_num = ARRAYSIZE(layout);

  hr = device_->CreateInputLayout(layout, elements_num, vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), &vertex_layout_);

  device_context_->IASetInputLayout(vertex_layout_);



  ///////// pixel shader /////////
  CComPtr<ID3DBlob> ps_blob;
  CComPtr<ID3DBlob> ps_error_blob;

  std::string psstr(pixel_shader_code);
  hr = D3DCompile(psstr.c_str(), psstr.length(), nullptr, nullptr, nullptr, "PS", "ps_5_0", flags, 0, &ps_blob, &ps_error_blob);
  if (FAILED(hr))
  {
    printf("D3DCompile failed");
    std::terminate();
  }

  //hr = D3DCompileFromFile(L"ps.hlsl", nullptr, nullptr, "PS", "ps_5_0", flags, 0, &ps_blob, &ps_error_blob);
  //CHECK_HR(hr, "D3DCompileFromFile");

  hr = device_->CreatePixelShader(ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(), nullptr, &pixel_shader_);
  if (FAILED(hr))
  {
    printf("CreatePixelShader failed");
    std::terminate();
  }


  ///////// vertex buffer /////////
  SimpleVertex vertices[] =
  {
    { DirectX::XMFLOAT3(-1.0f,  1.0f, 0.0f), DirectX::XMFLOAT2(0.0f, 0.0f) },
    { DirectX::XMFLOAT3(1.0f,  1.0f, 0.0f), DirectX::XMFLOAT2(1.0f, 0.0f) },
    { DirectX::XMFLOAT3(-1.0f, -1.0f, 0.0f), DirectX::XMFLOAT2(0.0f, 1.0f) },
    { DirectX::XMFLOAT3(1.0f, -1.0f, 0.0f), DirectX::XMFLOAT2(1.0f, 1.0f) },
  };

  //SimpleVertex vertices[] =
  //{
  //  { DirectX::XMFLOAT3(-1.0f,  2.0f, 0.0f), DirectX::XMFLOAT2(0.0f, 0.0f) },
  //  { DirectX::XMFLOAT3(2.0f,  2.0f, 0.0f), DirectX::XMFLOAT2(1.0f, 0.0f) },
  //  { DirectX::XMFLOAT3(-1.0f, -1.0f, 0.0f), DirectX::XMFLOAT2(0.0f, 1.0f) },
  //  { DirectX::XMFLOAT3(2.0f, -1.0f, 0.0f), DirectX::XMFLOAT2(1.0f, 1.0f) },
  //};


  D3D11_BUFFER_DESC bd = { 0 };
  bd.Usage = D3D11_USAGE_DEFAULT;
  bd.ByteWidth = sizeof(SimpleVertex) * ARRAYSIZE(vertices);
  bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
  bd.CPUAccessFlags = 0;

  D3D11_SUBRESOURCE_DATA InitData = { 0 };
  InitData.pSysMem = vertices;
  hr = device_->CreateBuffer(&bd, &InitData, &vertex_buffer_);
  if (FAILED(hr))
  {
    printf("Create Vertex Buffer failed");
    std::terminate();
  }

  UINT stride = sizeof(SimpleVertex);
  UINT offset = 0;
  device_context_->IASetVertexBuffers(0, 1, &vertex_buffer_.p, &stride, &offset);



  ///////// vertex buffer /////////
  WORD indices[] =
  {
    0, 1, 2,
    2, 1, 3,
  };

  bd.Usage = D3D11_USAGE_DEFAULT;
  bd.ByteWidth = sizeof(WORD) * ARRAYSIZE(indices);
  bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
  bd.CPUAccessFlags = 0;
  InitData.pSysMem = indices;

  hr = device_->CreateBuffer(&bd, &InitData, &index_buffer_);
  if (FAILED(hr))
  {
    printf("Create Index Buffer failed");
    std::terminate();
  }

  device_context_->IASetIndexBuffer(index_buffer_.p, DXGI_FORMAT_R16_UINT, 0);


  device_context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  device_context_->VSSetShader(vertex_shader_, nullptr, 0);
  device_context_->PSSetShader(pixel_shader_, nullptr, 0);
}