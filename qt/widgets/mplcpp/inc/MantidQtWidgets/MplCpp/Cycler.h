// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MPLCPP_CYCLER_H
#define MPLCPP_CYCLER_H

#include "MantidQtWidgets/MplCpp/DllConfig.h"
#pragma push_macro("slots")
#undef slots
#include "MantidQtWidgets/Common/Python/Object.h"
#pragma pop_macro("slots")

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

/**
 * @brief The Cycler class combines the functionality of Cycler
 * object from the cycler module with Python's itertools.cycle functionality
 * to create an interable that endlessly loops around a sequence of values.
 * The call operator is used to produce the next value in the cycle
 */
class MANTID_MPLCPP_DLL Cycler : public Python::InstanceHolder {
public:
  Cycler(Python::Object obj);

  /// Return the next value in the sequence
  Python::Dict operator()() const;
};

/// Create a cycler from a string of values
MANTID_MPLCPP_DLL Cycler cycler(const char *label, const char *iterable);

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt

#endif // MPLCPP_CYCLER_H
