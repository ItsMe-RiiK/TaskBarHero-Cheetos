#pragma once
#include "Il2CppOffsets.h"
#include "ProcessMemory.h"

#include <cstdint>
#include <optional>
#include <vector>

/* -----------------------------------------------------------------------
 * Generic IL2CPP List<T> reader for reference types (class instances).
 *
 * IL2CPP List<T> layout:
 *   listPtr + 0x10 -> T[] _items  (backing array pointer)
 *   listPtr + 0x18 -> int _size   (actual element count)
 *
 * IL2CPP Array header:
 *   arrayPtr + 0x18 -> int length  (array capacity)
 *   arrayPtr + 0x20 -> first element (pointer-sized for ref types)
 * ----------------------------------------------------------------------- */
class Il2CppListReader
{
public:
  explicit Il2CppListReader(const ProcessMemory& mem) :
      m_mem(mem)
  {
  }

  // Returns the actual size (_size) of the list.
  std::optional<int32_t> GetSize(uintptr_t listPtr) const
  {
    return m_mem.ReadInt32(listPtr + Il2CppListOffsets::Size);
  }

  // Reads all element pointers from a List<T> where T is a reference type.
  // Returns a vector of pointers to the individual T instances.
  std::vector<uintptr_t> ReadElementPointers(uintptr_t listPtr) const
  {
    std::vector<uintptr_t> result;

    auto itemsPtr = m_mem.ReadPointer(listPtr + Il2CppListOffsets::Items);
    if (!itemsPtr || *itemsPtr == 0)
      return result;

    auto size = m_mem.ReadInt32(listPtr + Il2CppListOffsets::Size);
    if (!size || *size <= 0 || *size > 10000)
      return result;  // sanity cap

    for (int32_t i = 0; i < *size; i++) {
      uintptr_t elemAddr = *itemsPtr + Il2CppArrayOffsets::Data + (i * sizeof(uintptr_t));
      auto      elemPtr  = m_mem.ReadPointer(elemAddr);
      if (elemPtr && *elemPtr != 0) {
        result.push_back(*elemPtr);
      }
    }
    return result;
  }

private:
  const ProcessMemory& m_mem;
};
