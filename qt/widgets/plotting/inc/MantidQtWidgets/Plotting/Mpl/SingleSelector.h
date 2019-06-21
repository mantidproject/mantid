// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_PLOTTING_MPL_SINGLESELECTOR_H_
#define MANTIDQT_PLOTTING_MPL_SINGLESELECTOR_H_

#include "MantidQtWidgets/MplCpp/SingleMarker.h"
#include "MantidQtWidgets/Plotting/DllOption.h"

#include <QWidget>

namespace MantidQt {
namespace MantidWidgets {
class PreviewPlot;

/**
 * Displays a line for selecting a value on a previewplot in MPL
 */
class EXPORT_OPT_MANTIDQT_PLOTTING SingleSelector : public QObject {
  Q_OBJECT

public:
  enum SelectType { XSINGLE, YSINGLE };

  SingleSelector(PreviewPlot *plot, SelectType type = XSINGLE,
                 double position = 0.0, bool visible = true,
                 const QColor &colour = Qt::black);

  void setColour(const QColor &colour);

  void setBounds(const std::pair<double, double> &bounds);
  void setBounds(const double minimum, const double maximum);
  void setLowerBound(const double minimum);
  void setUpperBound(const double maximum);

  void setPosition(const double position);
  double getPosition() const;

  void setVisible(bool visible);

  void detach();

signals:
  void valueChanged(double position);

private slots:
  void handleMouseDown(const QPoint &point);
  void handleMouseMove(const QPoint &point);
  void handleMouseUp(const QPoint &point);

  void redrawMarker();

private:
  std::tuple<double, double> getAxisRange(const SelectType &type) const;
  QString selectTypeAsQString(const SelectType &type) const;

  /// The preview plot containing the range selector
  PreviewPlot *m_plot;
  /// The single marker
  std::unique_ptr<MantidQt::Widgets::MplCpp::SingleMarker> m_singleMarker;
  /// Is the marker visible or hidden
  bool m_visible;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif // MANTIDQT_PLOTTING_MPL_SINGLESELECTOR_H_
