#ifndef _WINDOWS_EXAMPLE_ASYNC_CALLBACK_H_
#define _WINDOWS_EXAMPLE_ASYNC_CALLBACK_H_

#include <mftransform.h>

class AsyncCallback : public IMFAsyncCallback
{
public:
  AsyncCallback();
  virtual ~AsyncCallback();
  STDMETHODIMP QueryInterface(REFIID riid, void** ppv);
  STDMETHODIMP_(ULONG) AddRef();
  STDMETHODIMP_(ULONG) Release();
  STDMETHODIMP GetParameters(DWORD* pdwFlags, DWORD* pdwQueue);
  STDMETHODIMP Invoke(IMFAsyncResult* pAsyncResult) = 0;

private:
  long    ref_;
};

#endif