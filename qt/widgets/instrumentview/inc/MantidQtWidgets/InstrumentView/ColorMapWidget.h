#ifndef COLORMAPWIDGET_H_
#define COLORMAPWIDGET_H_

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
class ColorMapWidget : public QFrame {
  Q_OBJECT
  enum DragType { Bottom, Top };

public:
  ColorMapWidget(int type, QWidget *parent,
                 const double &minPositiveValue = 0.0001);
  void setupColorBarScaling(const MantidColorMap &);
  void setMinValue(double);
  void setMaxValue(double);
  QString getMinValue() const;
  QString getMaxValue() const;
  QString getNth_power() const;
  void setMinPositiveValue(double);
  int getScaleType() const;
  void setScaleType(int);
  void setNthPower(double);
  /// Load the state of the actor from a Mantid project file.
  void loadFromProject(const std::string &lines);
  /// Save the state of the actor to a Mantid project file.
  std::string saveToProject() const;
signals:
  void scaleTypeChanged(int);
  void minValueChanged(double);
  void maxValueChanged(double);
  void nthPowerChanged(double);

protected:
  void mousePressEvent(QMouseEvent *) override;
  void mouseMoveEvent(QMouseEvent *) override;
  void mouseReleaseEvent(QMouseEvent *) override;
  void updateScale();
  void setMinValueText(double);
  void setMaxValueText(double);
private slots:
  void scaleOptionsChanged(int);
  void nPowerChanged(double);
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

#endif /*COLORMAPWIDGET_H_*/
