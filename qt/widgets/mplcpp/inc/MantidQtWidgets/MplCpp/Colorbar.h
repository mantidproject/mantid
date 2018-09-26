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
#include "MantidQtWidgets/MplCpp/Python/Object.h"

#include <QWidget>

namespace MantidQt {
namespace Widgets {
namespace MplCpp {
class MantidColorMap;

/**
 * @brief Provides a widget to display a color map
 * on a defined scale with a configurable scale type. The implementation
 * uses matplotlib.colorbar
 */
class MANTID_MPLCPP_DLL Colorbar : public QWidget {
  Q_OBJECT
public:
  Colorbar(QWidget *parent = nullptr);

  ///@name Legacy API to match DraggableColorBarWidget for instrument view
  ///@{
  Colorbar(int type, QWidget *parent = nullptr);
  void setupColorBarScaling(const MantidColorMap &) {}
  void setMinValue(double min) {}
  void setMaxValue(double max) {}
  QString getMinValue() const { return "NAN"; }
  QString getMaxValue() const { return "NAN"; }
  QString getNthPower() const { return "NAN"; }
  void setMinPositiveValue(double){
      // This should set the linthresh for synlog
  };
  int getScaleType() const { return 0; }
  void setScaleType(int) {}
  void setNthPower(double) {}
  void loadFromProject(const std::string &) {}
  std::string saveToProject() const { return ""; }
  ///@}
};

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt

#endif // MPLCPP_COLORBARWIDGET_H
