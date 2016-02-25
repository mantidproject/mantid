#include <set>

#include "MantidKernel/IndexSet.h"
#include "MantidKernel/Exception.h"

using namespace Mantid;
using namespace Kernel;

IndexSet::IndexSet(int64_t min, int64_t max, size_t numberOfHistograms) {
  if (min < 0 || min > max)
    throw Exception::IndexError(min, max, "IndexSet - min");
  if (max >= static_cast<int64_t>(numberOfHistograms))
    throw Exception::IndexError(max, numberOfHistograms - 1, "IndexSet - max");

  m_min = static_cast<size_t>(min);
  m_size = static_cast<size_t>(max - min + 1);
}

IndexSet::IndexSet(const std::vector<size_t> indices, size_t numberOfHistograms)
    : m_isRange(false) {
  // We use a set to create unique and ordered indices.
  std::set<size_t> index_set;
  for (const auto &index : indices) {
    if (index >= numberOfHistograms)
      throw Exception::IndexError(index, numberOfHistograms - 1,
                                  "IndexSet - index vector entry");
    index_set.insert(static_cast<size_t>(index));
  }
  m_indices = std::vector<size_t>(begin(index_set), end(index_set));
  m_size = m_indices.size();
}

IndexSet::IndexSet(size_t numberOfHistograms) : m_size(numberOfHistograms) {}
