#include "core/ProcessMemory.h"
#include "ui/CheetosGUI.h"

int main(int argc, char** argv)
{
  ProcessMemory mem;
  CheetosGUI    ui(mem);
  ui.Run();

  return 0;
}