// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MPLCPP_MANTID_AXES_H
#define MPLCPP_MANTID_AXES_H

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtWidgets/MplCpp/Axes.h"

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

/**
 * C++ wrapper around a matplotlib MantidAxes object instance
 */
class MANTID_MPLCPP_DLL MantidAxes : public Axes {
public:
  MantidAxes(Python::Object pyObj);

  Line2D plot(const Mantid::API::MatrixWorkspace_sptr &workspace,
              const size_t wkspIndex, const QString format,
              const QString label);
};

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt

#endif // MPLCPP_AXES_H
