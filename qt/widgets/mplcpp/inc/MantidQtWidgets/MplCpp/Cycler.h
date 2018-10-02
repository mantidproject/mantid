#ifndef MPLCPP_CYCLER_H
#define MPLCPP_CYCLER_H
/*
  Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
*/
#include "MantidQtWidgets/MplCpp/DllConfig.h"
#include "MantidQtWidgets/MplCpp/Python/Object.h"

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
