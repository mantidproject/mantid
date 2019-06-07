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
#include <QStringList>
#include <QVariant>
#include <boost/none_t.hpp>
#include <boost/optional.hpp>
#include <vector>

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

/// C++ counterpart to MantidAxType in mantid.plots.utility
/// This does duplicate the information above but exposing the
/// Python type to C++ and allowing it to be passed through the
/// plot functions below is not trivial. It was not considered
/// worthwhile for its limited use.
/// Any changes here will require changes in the above module.
enum class MantidAxType : int { Bin = 0, Spectrum = 1 };

/**
 * Makes a call to mantidqt.plotting.plot.
 *
 * Each of the inputs to this function are optional and thus can be replaced
 * with boost::none if no other item should be given. However it does require
 * that boost::none be given for all items that are not the defaulted bools, on
 * every call.
 *
 * @param workspaces A vector of workspace names that are present in the ADS
 *
 * @param spectrumNums A vector of spectrum numbers correlating to the
 * respective workspace in the vector of workspace names
 *
 * @param wkspIndices A vector of workspace indices correlating to the
 * respective workspace in the vector of workspace names
 *
 * @param fig The python object that represents the matplotlib figure
 *
 * @param plotKwargs A QHash<QString, QVariant> that consists of the individual
 * plot options you can use, for examples look at the **kwargs section on this
 * page https://matplotlib.org/api/_as_gen/matplotlib.pyplot.plot.html
 *
 * @param axProperties A QHash<QString, QVariant> that consists of the
 * individual arguments you can give to the Matplotlib Axes object, for
 * examples look at the **kwargs section on this page
 * https://matplotlib.org/api/axes_api.html
 *
 * @param windowTitle The title of the plot window will be set here
 *
 * @param errors Whether or not the plot should contain errorbars
 *
 * @param overplot If the plot should overplot other plots
 *
 * @return Returns the figure that was created by the
 * function in Python
 */
MANTID_MPLCPP_DLL Common::Python::Object
plot(const std::vector<std::string> &workspaces,
     boost::optional<std::vector<int>> spectrumNums,
     boost::optional<std::vector<int>> wkspIndices,
     boost::optional<Common::Python::Object> fig = boost::none,
     boost::optional<QHash<QString, QVariant>> plotKwargs = boost::none,
     boost::optional<QHash<QString, QVariant>> axProperties = boost::none,
     boost::optional<std::string> windowTitle = boost::none,
     bool errors = false, bool overplot = false);

/**
 * \overload plot(const std::vector<std::string> &workspaces,
     boost::optional<std::vector<int>> spectrumNums,
     boost::optional<std::vector<int>> wkspIndices,
     boost::optional<Common::Python::Object> fig = boost::none,
     boost::optional<QHash<QString, QVariant>> plotKwargs = boost::none,
     boost::optional<QHash<QString, QVariant>> axProperties = boost::none,
     boost::optional<std::string> windowTitle = boost::none,
     bool errors = false, bool overplot = false)
 */
MANTID_MPLCPP_DLL Common::Python::Object
plot(const QStringList &workspaces,
     boost::optional<std::vector<int>> spectrumNums,
     boost::optional<std::vector<int>> wkspIndices,
     boost::optional<Common::Python::Object> fig = boost::none,
     boost::optional<QHash<QString, QVariant>> plotKwargs = boost::none,
     boost::optional<QHash<QString, QVariant>> axProperties = boost::none,
     boost::optional<std::string> windowTitle = boost::none,
     bool errors = false, bool overplot = false);

/**
 * Makes a call to mantidqt.plotting.pcolormesh.
 *
 * @param workspaces A vector of workspace names that are present in the ADS
 *
 * @param fig The python object that represents the matplotlib figure.
 *
 * @return Returns the figure that was created by the
 * function in Python
 */
MANTID_MPLCPP_DLL Common::Python::Object
pcolormesh(const QStringList &workspaces,
           boost::optional<Common::Python::Object> fig = boost::none);

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt

#endif // MPLCPP_PLOT_H
