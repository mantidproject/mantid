#ifndef MANTID_MANTIDWIDGETS_COLORBARWIDGET_H_
#define MANTID_MANTIDWIDGETS_COLORBARWIDGET_H_

#include <QtGui/QWidget>
#include "ui_ColorBarWidget.h"
#include <qwt_color_map.h>
#include <qwt_scale_widget.h>
#include "MantidQtAPI/MantidColorMap.h"
#include <QKeyEvent>
#include <QtGui>
#include "MantidQtMantidWidgets/WidgetDllOption.h"

namespace MantidQt {
namespace MantidWidgets {

//=============================================================================
/** Extended version of QwtScaleWidget */
class QwtScaleWidgetExtended : public QwtScaleWidget {
  Q_OBJECT

public:
  QwtScaleWidgetExtended(QWidget *parent = NULL) : QwtScaleWidget(parent) {
    this->setMouseTracking(true);
  }

  void mouseMoveEvent(QMouseEvent * event) override
  {
    double val = 1.0 - double(event->y()) / double(this->height());
    emit mouseMoved(event->globalPos(), val);
  }

signals:
  void mouseMoved(QPoint, double);
};

//=============================================================================
/** Widget for showing a color bar, modifying its
 * limits, etc.
 *
 * @author Janik Zikovsky
 * @date Oct 31, 2011.
 */
class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS ColorBarWidget : public QWidget {
  Q_OBJECT
  Q_PROPERTY(double minimum READ getMinimum WRITE setMinimum)
  Q_PROPERTY(double maximum READ getMaximum WRITE setMaximum)

public:
  ColorBarWidget(QWidget *parent = 0);
  ~ColorBarWidget();

  void updateColorMap();

  void setViewRange(double min, double max);
  void setViewRange(QwtDoubleInterval range);
  void setMinimum(double min);
  void setMaximum(double max);
  void setRenderMode(bool rendering);

  double getMinimum() const;
  double getMaximum() const;
  QwtDoubleInterval getViewRange() const;
  MantidColorMap &getColorMap();

  bool getLog();

  int getScale();
  void setScale(int);

  void setExponent(double);
  double getExponent();

  void setAutoScale(bool autoscale);
  bool getAutoScale() const;

  bool getAutoColorScaleforCurrentSlice() const;

public slots:
  void changedMinimum();
  void changedMaximum();
  void colorBarMouseMoved(QPoint, double);
  void changedScaleType(int);
  void changedExponent(double);

signals:
  /// Signal sent when the range or log mode of the color scale changes.
  void changedColorRange(double min, double max, bool log);
  /// When the user double-clicks the color bar (e.g. load a new color map)
  void colorBarDoubleClicked();

private:
  void setSpinBoxesSteps();
  void mouseDoubleClickEvent(QMouseEvent * event) override;
  void updateMinMaxGUI();
  void resizeEvent(QResizeEvent * event) override;

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
} // namespace Mantid

#endif // MANTID_MANTIDWIDGETS_COLORBARWIDGET_H_
