// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MANTIDWIDGETS_COLORBARWIDGET_H_
#define MANTID_MANTIDWIDGETS_COLORBARWIDGET_H_

#include "MantidQtWidgets/LegacyQwt/DllOption.h"
#include "MantidQtWidgets/LegacyQwt/MantidColorMap.h"
#include "ui_ColorBarWidget.h"
#include <QMouseEvent>
#include <QWidget>
#include <qwt_scale_widget.h>

namespace MantidQt {
namespace MantidWidgets {

//=============================================================================
/** Extended version of QwtScaleWidget */
class QwtScaleWidgetExtended : public QwtScaleWidget {
  Q_OBJECT

public:
  QwtScaleWidgetExtended(QWidget *parent = nullptr) : QwtScaleWidget(parent) {
    this->setMouseTracking(true);
  }

  void mouseMoveEvent(QMouseEvent *event) override {
    double val = 1.0 - double(event->y()) / double(this->height());
    emit mouseMoved(event->globalPos(), val);
  }

signals:
  void mouseMoved(QPoint /*_t1*/, double /*_t2*/);
};

//=============================================================================
/** Widget for showing a color bar, modifying its
 * limits, etc.
 *
 * @author Janik Zikovsky
 * @date Oct 31, 2011.
 */
class EXPORT_OPT_MANTIDQT_LEGACYQWT ColorBarWidget : public QWidget {
  Q_OBJECT
  Q_PROPERTY(double minimum READ getMinimum WRITE setMinimum)
  Q_PROPERTY(double maximum READ getMaximum WRITE setMaximum)

public:
  ColorBarWidget(QWidget *parent = nullptr);
  ~ColorBarWidget() override;

  enum CheckboxStrategy {
    ADD_AUTOSCALE_CURRENT_SLICE = 0,
    ADD_AUTOSCALE_ON_LOAD = 1,
    ADD_AUTOSCALE_BOTH = 2,
    ADD_AUTOSCALE_NONE = 3
  };

  void updateColorMap();

  void setViewRange(double min, double max);
  void setViewRange(QwtDoubleInterval range);
  void setMinimum(double min);
  void setMaximum(double max);
  void setRenderMode(bool rendering);

  /// Set which checkboxes are shown in the window
  void setCheckBoxMode(CheckboxStrategy strategy);

  double getMinimum() const;
  double getMaximum() const;
  QwtDoubleInterval getViewRange() const;
  MantidColorMap &getColorMap();

  /// Check if logarithmic scale is selected
  bool getLog();

  int getScale() const;
  void setScale(int /*type*/);

  void setExponent(double /*nth_power*/);
  double getExponent() const;

  /// Set the label text for Auto Scale on Load checkbox label
  void setAutoScaleLabelText(const std::string &newText);

  /// Set the tooltip text for Auto Scale on Load checkbox label
  void setAutoScaleTooltipText(const std::string &newText);

  /// Set the tooltip text for Auto Scale for Current Slice checkbox label
  void setAutoScaleForCurrentSliceLabelText(const std::string &newText);

  /// Set the tooltip text for Auto Scale for Current Slice checkbox label
  void setAutoScaleForCurrentSliceTooltipText(const std::string &newText);

  /// Set the Auto Scale on Load checkbox state
  void setAutoScale(bool autoscale);

  /// Get the Auto Scale on Load checkbox state
  bool getAutoScale() const;

  /// Set the Auto Scale for Current Slice checkbox state
  bool getAutoScaleforCurrentSlice() const;

  /// Load the state of the color bar widget from a Mantid project file
  void loadFromProject(const std::string &lines);

  /// Save the state of the color bar widget to a Mantid project file
  std::string saveToProject() const;

public slots:
  void changedMinimum();
  void changedMaximum();
  void colorBarMouseMoved(QPoint /*globalPos*/, double /*fraction*/);
  void changedScaleType(int /*type*/);
  void changedExponent(double /*nth_power*/);

signals:
  /// Signal sent when the range or log mode of the color scale changes.
  void changedColorRange(double min, double max, bool log);
  /// When the user double-clicks the color bar (e.g. load a new color map)
  void colorBarDoubleClicked();

private:
  void setSpinBoxesSteps();
  void mouseDoubleClickEvent(QMouseEvent *event) override;
  void updateMinMaxGUI();
  void resizeEvent(QResizeEvent *event) override;

  /// Auto-gen UI classes
  Ui::ColorBarWidgetClass ui;

  /// The color bar widget from QWT
  QwtScaleWidget *m_colorBar;

  /// Color map being displayed
  MantidColorMap m_colorMap;

  /// Logarithmic scale?
  bool m_log;

  /// Min value being displayed
  double m_min;

  /// Min value being displayed
  double m_max;

  /// Show the value tooltip (off by default)
  bool m_showTooltip;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif // MANTID_MANTIDWIDGETS_COLORBARWIDGET_H_
