#include "AsyncCallback.h"
#include <shlwapi.h>

AsyncCallback::AsyncCallback() :
  ref_(1)
{

}

  AsyncCallback::~AsyncCallback()
{

}

STDMETHODIMP AsyncCallback::QueryInterface(REFIID riid, void** ppv)
{
  static const QITAB qit[] =
  {
      QITABENT(AsyncCallback, IMFAsyncCallback),
      { 0 }
  };
  return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) AsyncCallback::AddRef()
{
  return InterlockedIncrement(&ref_);
}

STDMETHODIMP_(ULONG) AsyncCallback::Release()
{
  long ref = InterlockedDecrement(&ref_);
  if (ref == 0)
  {
    delete this;
  }
  return ref;
}

STDMETHODIMP AsyncCallback::GetParameters(DWORD* pdwFlags, DWORD* pdwQueue)
{
  // Implementation of this method is optional.
  return E_NOTIMPL;
}