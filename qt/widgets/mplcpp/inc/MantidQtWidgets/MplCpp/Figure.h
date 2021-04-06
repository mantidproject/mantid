// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidPythonInterface/core/GlobalInterpreterLock.h"
#include "MantidQtWidgets/Common/Python/Object.h"
#include "MantidQtWidgets/MplCpp/Axes.h"
#include "MantidQtWidgets/MplCpp/DllConfig.h"
#include "MantidQtWidgets/MplCpp/ScalarMappable.h"

#include <QHash>
#include <QVariant>

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

/**
 * A thin C++ wrapper to create a new matplotlib figure
 */
class MANTID_MPLCPP_DLL Figure : public Common::Python::InstanceHolder {
public:
  Figure(Common::Python::Object obj);
  Figure(bool tightLayout = true);

  /**
   * @brief Access (and create if necessar) the active Axes
   * @return An instance of Axes attached to the figure
   */
  template <typename AxesType = Axes> inline AxesType gca() const {
    Mantid::PythonInterface::GlobalInterpreterLock lock;
    return AxesType{pyobj().attr("gca")()};
  }
  /**
   * @param index The index of the axes to return
   * @return The axes instance
   */
  inline Axes axes(size_t index) const {
    Mantid::PythonInterface::GlobalInterpreterLock lock;
    return Axes{pyobj().attr("axes")[index]};
  }

  int number() const;
  void setTightLayout(QHash<QString, QVariant> const &args);
  QColor faceColor() const;
  void setFaceColor(const QColor &color);
  void setFaceColor(const char *color);
  void setWindowTitle(const char *title);
  void show();
  Axes addAxes(double left, double bottom, double width, double height);
  Axes addSubPlot(const int subplotspec, const QString &projection = "");
  Common::Python::Object colorbar(const ScalarMappable &mappable, const Axes &cax,
                                  const Common::Python::Object &ticks = Common::Python::Object(),
                                  const Common::Python::Object &format = Common::Python::Object());
};

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt
