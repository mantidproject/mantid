// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef DRAGGABLECOLORBARWIDGET_H_
#define DRAGGABLECOLORBARWIDGET_H_

#include "MantidQtWidgets/Plotting/DllOption.h"
#include <QFrame>

class MantidColorMap;

class QwtScaleWidget;
class QLineEdit;
class QComboBox;
class QLabel;
class DoubleSpinBox;

namespace MantidQt {
namespace MantidWidgets {
/**
 * Displays a color map with numeric axis and editable bounds
 */
class EXPORT_OPT_MANTIDQT_PLOTTING DraggableColorBarWidget : public QFrame {
  Q_OBJECT
  enum DragType { Bottom, Top };

public:
  DraggableColorBarWidget(QWidget *parent,
                          const double &minPositiveValue = 0.0001);
  void setupColorBarScaling(const MantidColorMap & /*colorMap*/);
  void setClim(double vmin, double vmax);
  void setMinValue(double /*value*/);
  void setMaxValue(double /*value*/);
  QString getMinValue() const;
  QString getMaxValue() const;
  QString getNthPower() const;
  void setMinPositiveValue(double /*value*/);
  int getScaleType() const;
  void setScaleType(int /*type*/);
  void setNthPower(double /*nth_power*/);
  /// Load the state of the actor from a Mantid project file.
  void loadFromProject(const std::string &lines);
  /// Save the state of the actor to a Mantid project file.
  std::string saveToProject() const;
signals:
  void scaleTypeChanged(int /*_t1*/);
  void minValueChanged(double /*_t1*/);
  void maxValueChanged(double /*_t1*/);
  void nthPowerChanged(double /*_t1*/);

  // Edited signals only emitted when manual editing of that field
  // occurs
  void minValueEdited(double /*_t1*/);
  void maxValueEdited(double /*_t1*/);

protected:
  void mousePressEvent(QMouseEvent * /*unused*/) override;
  void mouseMoveEvent(QMouseEvent * /*unused*/) override;
  void mouseReleaseEvent(QMouseEvent * /*unused*/) override;
  void updateScale();
  void setMinValueText(double /*value*/);
  void setMaxValueText(double /*value*/);
private slots:
  void scaleOptionsChanged(int /*i*/);
  void nPowerChanged(double /*nth_power*/);
  void minValueChanged();
  void maxValueChanged();

private:
  QwtScaleWidget *m_scaleWidget;
  QLineEdit *m_minValueBox, *m_maxValueBox;
  QComboBox *m_scaleOptions;
  QLabel *m_lblN;
  DoubleSpinBox *m_dspnN;
  double m_minPositiveValue;
  bool m_dragging;
  int m_y;
  DragType m_dtype;
  double m_nth_power;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif // DRAGGABLECOLORBARWIDGET_H_
