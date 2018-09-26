#ifndef MANTIDQTWIDGETS_INSTRUMENTVIEW_MINIPLOTMPL_H
#define MANTIDQTWIDGETS_INSTRUMENTVIEW_MINIPLOTMPL_H

#include "MantidQtWidgets/InstrumentView/DllOption.h"
#include <QWidget>

namespace MantidQt {
namespace MantidWidgets {
class PeakMarker2D;

class EXPORT_OPT_MANTIDQT_INSTRUMENTVIEW MiniPlotMpl : public QWidget {
  Q_OBJECT
public:
  explicit MiniPlotMpl(QWidget *parent = nullptr);

  void setData(const double *x, const double *y, int dataSize,
               const std::string &xUnits = "") {}
  void setLabel(const QString &label) {}
  QString label() const { return "LABEL"; }
  void setYAxisLabelRotation(double degrees) {}
  void addPeakLabel(const PeakMarker2D *) {}
  void clearPeakLabels() {}
  bool hasCurve() const { return false; }
  void store() {}
  bool hasStored() const { return false; }
  QStringList getLabels() const { return {}; }
  void removeCurve(const QString &label) {}
  QColor getCurveColor(const QString &label) const { return QColor(); }
  void recalcXAxisDivs() {}
  void recalcYAxisDivs() {}
  bool isYLogScale() const { return false; }
  const std::string &getXUnits() const { return "UNIT"; }
  void replot() {};
public slots:
  void setXScale(double from, double to) {}
  void setYScale(double from, double to) {}
  void clearCurve() {}
  void recalcAxisDivs() {}
  void setYLogScale() {}
  void setYLinearScale() {}
  void clearAll() {}
signals:
  void showContextMenu();
  void clickedAt(double, double);
};
} // namespace MantidWidgets
} // namespace MantidQt

#endif // MANTIDQTWIDGETS_INSTRUMENTVIEW_MINIPLOTMPL_H
