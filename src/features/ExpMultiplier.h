#pragma once
#include "../core/ProcessMemory.h"
#include <string>

class ExpMultiplier
{
public:
  ExpMultiplier(ProcessMemory& mem);
  ~ExpMultiplier();

  struct Result
  {
    bool        success;
    std::string msg;
  };

  Result ApplyMaxExp(float multiplier);

private:
  ProcessMemory& m_mem;
};
