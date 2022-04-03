#include "DesktopDuplication.h"

int main()
{
  // Initialize Desktop Duplication related API in constructor
  DesktopDuplication dup;

  // Capture and save the number of screens
  for (int i = 0; i < dup.ScreenNumber(); i++)
    dup.Capture(i);  

  return 0;
}