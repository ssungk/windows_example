#ifndef _WINDOWS_EXAMPLE_DESKTOP_DUPLICATION_H_
#define _WINDOWS_EXAMPLE_DESKTOP_DUPLICATION_H_

#include <iostream>
#include <vector>
#include <string>
#include <Windows.h>
#include <atlbase.h>
#include <d3d11.h>
#include <dxgi1_2.h>

class DesktopDuplication
{
public:
  DesktopDuplication();
  virtual ~DesktopDuplication();
  int ScreenNumber();
  void Capture(int index);

private:
  void init();

private:
  CComPtr<ID3D11Device> device_;
  CComPtr<ID3D11DeviceContext> context_;
  std::vector<CComPtr<IDXGIOutputDuplication>> dups_;
  std::vector<CComPtr<ID3D11Texture2D>> textures_;
  std::vector<FILE*> fs_;

};
#endif