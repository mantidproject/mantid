#ifndef MANTID_KERNEL_INDEXSET_H_
#define MANTID_KERNEL_INDEXSET_H_

#include <vector>

#include "MantidKernel/DllConfig.h"

namespace Mantid {
namespace Kernel {

class MANTID_KERNEL_DLL IndexSet {
public:
  IndexSet(size_t fullRange);
  IndexSet(int64_t min, int64_t max, size_t fullRange);
  IndexSet(const std::vector<size_t> indices, size_t fullRange);

  size_t size() const { return m_size; }

  size_t operator[](size_t index) const {
    if (m_isRange)
      return m_min + index;
    return m_indices[index];
  }

private:
  bool m_isRange = true;
  // Default here to avoid uninitialized warning for m_isRange = true.
  size_t m_min = 0;
  size_t m_size;
  std::vector<size_t> m_indices;
};

} // Namespace Kernel
} // Namespace Mantid

#endif /* MANTID_KERNEL_INDEXSET_H_ */
