// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MPLCPP_AXES_H
#define MPLCPP_AXES_H

#include "MantidQtWidgets/MplCpp/DllConfig.h"
#include "MantidQtWidgets/MplCpp/Line2D.h"

#include <QString>
#include <functional>
#include <tuple>

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

class MANTID_MPLCPP_DLL Axes : public Common::Python::InstanceHolder {
public:
  explicit Axes(Common::Python::Object obj);

  /// @name General axes manipulation
  /// @{
  void clear();
  /// Function-signature required for operation applied to each artist
  using ArtistOperation = std::function<void(Artist &&)>;
  void forEachArtist(const char *containerAttr, const ArtistOperation &op);
  void removeArtists(const char *containerAttr, const QString label);
  void setXLabel(const char *label);
  void setYLabel(const char *label);
  void setTitle(const char *label);
  /// @}

  /// @name Drawing
  /// @{
  Artist legend(const bool draggable);
  Artist legendInstance() const;
  Line2D plot(std::vector<double> xdata, std::vector<double> ydata,
              const char *format = "b-");
  Line2D plot(std::vector<double> xdata, std::vector<double> ydata,
              const QString format, const QString label);
  Artist text(double x, double y, QString text,
              const char *horizontalAlignment);
  /// @}

  ///@name Scales
  /// @{
  void setXScale(const char *value);
  QString getXScale() const;
  void setYScale(const char *value);
  QString getYScale() const;
  std::tuple<double, double> getXLim() const;
  void setXLim(double min, double max) const;
  std::tuple<double, double> getYLim() const;
  void setYLim(double min, double max) const;
  void relim(bool visibleOnly = false);
  void autoscale(bool enable);
  void autoscaleView(bool scaleX = true, bool scaleY = true);
  void autoscaleView(bool tight, bool scaleX, bool scaleY);
  /// @}
};

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt

#endif // MPLCPP_AXES_H
