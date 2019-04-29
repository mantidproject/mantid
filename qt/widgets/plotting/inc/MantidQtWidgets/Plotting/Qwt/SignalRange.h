// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_API_SIGNALRANGE_H_
#define MANTIDQT_API_SIGNALRANGE_H_

#include "MantidAPI/IMDWorkspace.h"
#include "MantidQtWidgets/Plotting/DllOption.h"

#include <qwt_double_interval.h>

namespace MantidQt {
namespace API {
/**
 Calculates the signal range from a given workspace and optional MDFunction
*/
class EXPORT_OPT_MANTIDQT_PLOTTING SignalRange {
public:
  SignalRange(const Mantid::API::IMDWorkspace &workspace,
              const Mantid::API::MDNormalization normalization =
                  Mantid::API::NoNormalization);
  SignalRange(const Mantid::API::IMDWorkspace &workspace,
              Mantid::Geometry::MDImplicitFunction &function,
              const Mantid::API::MDNormalization normalization =
                  Mantid::API::NoNormalization);

  /// Disable default constructor
  SignalRange() = delete;

  /// Returns the range of the workspace signal values
  QwtDoubleInterval interval() const;

private:
  /// Find the min/max signal values in the entire workspace
  void findFullRange(const Mantid::API::IMDWorkspace &workspace,
                     Mantid::Geometry::MDImplicitFunction *function);

  /// Get the range of signal, in parallel, given an iterator
  QwtDoubleInterval getRange(
      const std::vector<std::unique_ptr<Mantid::API::IMDIterator>> &iterators);
  /// Get the range of signal given an iterator
  QwtDoubleInterval getRange(Mantid::API::IMDIterator *it);

  /// The range of the signal data
  QwtDoubleInterval m_interval;
  /// The normalization used for the signals
  Mantid::API::MDNormalization m_normalization;
};

} // namespace API
} // namespace MantidQt

#endif /* MANTIDQT_API_SIGNALRANGE_H_ */
