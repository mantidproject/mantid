// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MPLCPP_PLOT_H
#define MPLCPP_PLOT_H

#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidQtWidgets/MplCpp/DllConfig.h"
#include "MantidQtWidgets/MplCpp/Python/Object.h"

#include <QHash>
#include <QString>
#include <QVariant>
#include <boost/none_t.hpp>
#include <boost/optional.hpp>
#include <vector>

using namespace Mantid::API;
using namespace boost;

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

MANTID_MPLCPP_DLL Python::Object
plot(std::vector<MatrixWorkspace_sptr> workspaces,
     boost::optional<std::vector<std::size_t>> spectrum_nums,
     boost::optional<std::vector<std::size_t>> wksp_indices,
     boost::optional<Python::Object> fig,
     boost::optional<QHash<QString, QVariant>> plot_kwargs,
     boost::optional<QHash<QString, QVariant>> ax_properties,
     boost::optional<std::string> window_title, bool errors = false,
     bool overplot = false);

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt

#endif // MPLCPP_PLOT_H