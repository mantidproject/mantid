#ifndef MPLCPP_COLORBARWIDGET_H
#define MPLCPP_COLORBARWIDGET_H
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
#include "MantidQtWidgets/MplCpp/ScalarMappable.h"
#include "ui_Colorbar.h"

#include <tuple>

namespace MantidQt {
namespace Widgets {
namespace MplCpp {
class FigureCanvasQt;
class MantidColorMap;

/**
 * @brief Provides a widget to display a color map on a defined scale with a
 * configurable scale type. It contains controls the scale range
 * and type.

 * The implementation uses matplotlib.colorbar.
 */
class MANTID_MPLCPP_DLL ColorbarWidget : public QWidget {
  Q_OBJECT
public:
  ColorbarWidget(QWidget *parent = nullptr);

  void setClim(boost::optional<double> vmin, boost::optional<double> vmax);
  std::tuple<double, double> clim() const;

  ///@name Legacy API to match DraggableColorBarWidget for instrument view
  ///@{
  void setupColorBarScaling(const MantidColorMap &) {}
  void setMinValue(double min);
  void setMaxValue(double max);
  QString getMinValue() const;
  QString getMaxValue() const;
  QString getNthPower() const { return "NAN"; }
  void setMinPositiveValue(double){
      // This should set the linthresh for symlog...
  };
  int getScaleType() const { return 0; }
  void setScaleType(int) {}
  void setNthPower(double) {}
  void loadFromProject(const std::string &) {}
  std::string saveToProject() const { return ""; }
signals:
  //  void scaleTypeChanged(int);
  void minValueChanged(double);
  void maxValueChanged(double);
  //  void nthPowerChanged(double);
  ///@}

private slots:
  void scaleMinimumEdited();
  void scaleMaximumEdited();

private:
  void initLayout();
  void connectSignals();

private: // data
  Ui::Colorbar m_ui;
  FigureCanvasQt *m_canvas{nullptr};
  ScalarMappable m_mappable;
  Python::Object m_colorbar;
};

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt

#endif // MPLCPP_COLORBARWIDGET_H
