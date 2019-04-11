// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MPLCPP_COLORBARWIDGET_H
#define MPLCPP_COLORBARWIDGET_H

#include "MantidQtWidgets/MplCpp/Colors.h"
#include "MantidQtWidgets/MplCpp/DllConfig.h"
#include "MantidQtWidgets/MplCpp/Figure.h"
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

  void setNorm(const NormalizeBase &norm);
  void setClim(boost::optional<double> vmin, boost::optional<double> vmax);
  std::tuple<double, double> clim() const;

  ///@name Legacy API to match DraggableColorBarWidget for instrument view
  ///@{
  void setupColorBarScaling(const MantidColorMap &mtdCMap);
  void setMinValue(double min);
  void setMaxValue(double max);
  QString getMinValue() const;
  QString getMaxValue() const;
  QString getNthPower() const;
  void
  setMinPositiveValue(double /*unused*/) { /*Unused in this implementation*/
  }
  int getScaleType() const;
  void setScaleType(int /*index*/);
  void setNthPower(double /*gamma*/);
  void loadFromProject(const std::string & /*unused*/) {}
  std::string saveToProject() const { return ""; }
signals:
  // Changed signals emitted for any change
  void scaleTypeChanged(int /*_t1*/);
  void minValueChanged(double /*_t1*/);
  void maxValueChanged(double /*_t1*/);
  void nthPowerChanged(double /*_t1*/);

  // Edited signals only emitted when manual editing of that field
  // occurs
  void minValueEdited(double /*_t1*/);
  void maxValueEdited(double /*_t1*/);
  ///@}

private slots:
  void scaleMinimumEdited();
  void scaleMaximumEdited();
  void scaleTypeSelectionChanged(int index);
  void powerExponentEdited();

private:
  void initLayout();
  void createColorbar(const Python::Object &ticks = Python::Object(),
                      const Python::Object &format = Python::Object());
  void connectSignals();

private: // data
  Ui::Colorbar m_ui;
  FigureCanvasQt *m_canvas{nullptr};
  ScalarMappable m_mappable;
};

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt

#endif // MPLCPP_COLORBARWIDGET_H
