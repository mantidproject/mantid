// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MPLCPP_SIPUTILS_H
#define MPLCPP_SIPUTILS_H

#include "MantidQtWidgets/MplCpp/Python/Object.h"
#include <sip.h>
#include <stdexcept>

namespace MantidQt {
namespace Widgets {
namespace MplCpp {
namespace Python {

namespace detail {
const sipAPIDef *sipAPI();
} // namespace detail

template <typename T> T *extract(const Object &obj);

} // namespace Python
} // namespace MplCpp
} // namespace Widgets

} // namespace MantidQt

#endif // MPLCPP_SIPUTILS_H
