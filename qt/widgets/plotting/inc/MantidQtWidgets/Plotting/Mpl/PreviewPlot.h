// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_PLOTTING_MPL_PREVIEWPLOT_H_
#define MANTIDQT_PLOTTING_MPL_PREVIEWPLOT_H_

#include "MantidQtWidgets/Plotting/Axis.h"
#include "MantidQtWidgets/Plotting/DllOption.h"

#include "MantidAPI/MatrixWorkspace_fwd.h"

#include <QWidget>

namespace MantidQt {
namespace MantidWidgets {

/**
 * Displays several workpaces on a matplotlib figure
 */
class EXPORT_OPT_MANTIDQT_PLOTTING PreviewPlot : public QWidget {
  Q_OBJECT

public:
  PreviewPlot(QWidget *parent = nullptr, bool init = true);

  void addSpectrum(const QString &curveName,
                   const Mantid::API::MatrixWorkspace_sptr &ws,
                   const size_t wsIndex = 0,
                   const QColor &curveColour = QColor());
  void addSpectrum(const QString &curveName, const QString &wsName,
                   const size_t wsIndex = 0,
                   const QColor &curveColour = QColor());
  void removeSpectrum(const QString &curveName);
  void setAxisRange(QPair<double, double> range,
                    int axisID = static_cast<int>(Axis::XBottom));

public slots:
  void clear();
  void resizeX();
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif // MANTIDQT_PLOTTING_MPL_PREVIEWPLOT_H_
