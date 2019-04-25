// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Plotting/Qwt//SignalRange.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidKernel/MultiThreaded.h"
#include <cmath>
namespace MantidQt {
namespace API {
//-------------------------------------------------------------------------
// Public methods
//-------------------------------------------------------------------------

/**
 * Create a signal range that covers the whole workspace. The signal
 * values are treated with the specified normalization.
 * [Default: NoNormalization]
 * @param workspace A reference to a workspace object
 * @param normalization The type of normalization
 */
SignalRange::SignalRange(const Mantid::API::IMDWorkspace &workspace,
                         const Mantid::API::MDNormalization normalization)
    : m_interval(), m_normalization(normalization) {
  findFullRange(workspace, nullptr);
}

/**
 * Find the signal range that region defined by the function gives on the
 * workspace
 * using the given normalization
 * @param workspace A reference to a workspace object
 * @param function A reference to an MDImplicitFunction object that defines a
 * region
 *                 of the workspace
 * @param normalization The type of normalization
 */
SignalRange::SignalRange(const Mantid::API::IMDWorkspace &workspace,
                         Mantid::Geometry::MDImplicitFunction &function,
                         const Mantid::API::MDNormalization normalization)
    : m_interval(), m_normalization(normalization) {
  findFullRange(workspace, &function);
}

/**
 * @return A QwtDoubleInterval defining the range
 */
QwtDoubleInterval SignalRange::interval() const { return m_interval; }

//-------------------------------------------------------------------------
// Private methods
//-------------------------------------------------------------------------
/**
 * @param workspace A reference to the workspace the explore
 * @param function A pointer to an MDImplicitFunction object that defines a
 * region
 *                 of the workspace. NULL indicates use whole workspace
 */
void SignalRange::findFullRange(
    const Mantid::API::IMDWorkspace &workspace,
    Mantid::Geometry::MDImplicitFunction *function) {
  auto iterators =
      workspace.createIterators(PARALLEL_GET_MAX_THREADS, function);
  m_interval = getRange(iterators);
}

/**
 * @param iterators :: vector of IMDIterator of what to find
 * @return the min/max range, or 0-1.0 if not found
 */
QwtDoubleInterval SignalRange::getRange(
    const std::vector<std::unique_ptr<Mantid::API::IMDIterator>> &iterators) {
  std::vector<QwtDoubleInterval> intervals(iterators.size());
  // cppcheck-suppress syntaxError
      PRAGMA_OMP( parallel for schedule(dynamic, 1))
      for (int i = 0; i < int(iterators.size()); i++) {
        auto it = iterators[i].get();
        intervals[i] = this->getRange(it);
        // don't delete iterator in parallel. MSVC doesn't like it
        // when the iterator points to a mock object.
      }

      // Combine the overall min/max
      double minSignal = DBL_MAX;
      double maxSignal = -DBL_MAX;
      for (size_t i = 0; i < iterators.size(); i++) {
        double signal;
        signal = intervals[i].minValue();
        if (!std::isfinite(signal))
          continue;
        minSignal = std::min(signal, minSignal);

        signal = intervals[i].maxValue();
        if (!std::isfinite(signal))
          continue;
        maxSignal = std::max(signal, maxSignal);
      }

      if (minSignal == DBL_MAX) {
        minSignal = 0.0;
        maxSignal = 1.0;
      }
      if (minSignal < maxSignal)
        return QwtDoubleInterval(minSignal, maxSignal);
      else {
        if (minSignal != 0)
          // Possibly only one value in range
          return QwtDoubleInterval(minSignal * 0.5, minSignal * 1.5);
        else
          // Other default value
          return QwtDoubleInterval(0., 1.0);
      }
}

/**
 * @param it :: IMDIterator of what to find
 * @return the min/max range, or INFINITY if not found
 */
QwtDoubleInterval SignalRange::getRange(Mantid::API::IMDIterator *it) {
  if ((it == nullptr) || (!it->valid()))
    return QwtDoubleInterval(0., 1.0);

  // Use the current normalization
  it->setNormalization(m_normalization);

  double minSignal = DBL_MAX;
  double maxSignal = -DBL_MAX;
  auto inf = std::numeric_limits<double>::infinity();
  do {
    double signal = it->getNormalizedSignal();
    // Skip any 'infs' as it screws up the color scale
    if (!std::isinf(signal)) {
      if (signal < minSignal)
        minSignal = signal;
      if (signal > maxSignal)
        maxSignal = signal;
    }
  } while (it->next());

  if (minSignal == DBL_MAX) {
    minSignal = inf;
    maxSignal = inf;
  }
  return QwtDoubleInterval(minSignal, maxSignal);
}

} // namespace API
} // namespace MantidQt
