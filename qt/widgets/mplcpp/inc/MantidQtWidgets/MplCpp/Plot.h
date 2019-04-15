// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MPLCPP_PLOT_H
#define MPLCPP_PLOT_H

#include "MantidQtWidgets/Common/Python/Object.h"
#include "MantidQtWidgets/MplCpp/DllConfig.h"

#include <QHash>
#include <QString>
#include <QVariant>
#include <boost/none_t.hpp>
#include <boost/optional.hpp>
#include <vector>

using namespace boost;
using namespace MantidQt::Widgets::Common;

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

MANTID_MPLCPP_DLL Python::Object
plot(std::vector<std::string> workspaces,
     boost::optional<std::vector<int>> spectrum_nums,
     boost::optional<std::vector<int>> wksp_indices,
     boost::optional<Python::Object> fig,
     boost::optional<QHash<QString, QVariant>> plot_kwargs,
     boost::optional<QHash<QString, QVariant>> ax_properties,
     boost::optional<std::string> window_title, bool errors = false,
     bool overplot = false);

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt

#endif // MPLCPP_PLOT_H