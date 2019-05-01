// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_SLICEVIEWER_NONORTHOGONALOVERLAY_H_
#define MANTID_SLICEVIEWER_NONORTHOGONALOVERLAY_H_

#include "DllOption.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidKernel/Matrix.h"
#include "MantidQtWidgets/Common/NonOrthogonal.h"
#include "MantidQtWidgets/Plotting/Qwt/QwtRasterDataMD.h"
#include "MantidQtWidgets/Plotting/Qwt/QwtRasterDataMDNonOrthogonal.h"
#include <QPainter>
#include <QWidget>
#include <qwt_plot.h>
#include <qwt_valuelist.h>

namespace MantidQt {
namespace SliceViewer {

/** GUI for overlaying a nonorthogonal axes onto the plot
  in the SliceViewer. Should be generic to overlays on any QwtPlot.

  @date 2016-08-23
*/
class EXPORT_OPT_MANTIDQT_SLICEVIEWER NonOrthogonalOverlay : public QWidget {
  Q_OBJECT

public:
  NonOrthogonalOverlay(QwtPlot *plot, QWidget *parent);
  ~NonOrthogonalOverlay() override;

  void enable();

  void disable();

  void updateXGridlines(QwtValueList xAxisTicks, double xAngle);
  void updateYGridlines(QwtValueList yAxisTicks, double yAngle);

private:
  QSize sizeHint() const override;
  QSize size() const;

  int height() const;
  int width() const;

  QPoint transform(QPointF coords) const;
  QPointF invTransform(QPoint pixels) const;

  void drawYLines(QPainter &painter, QPen &gridPen, int widthScreen,
                  QwtValueList yAxisTicks, double yAngle);

  void drawXLines(QPainter &painter, QPen &gridPen, int heightScreen,
                  QwtValueList xAxisTicks, double xAngle);

  void paintEvent(QPaintEvent *event) override;

  bool m_enabled;

  QwtPlot *m_plot;
  Mantid::API::IMDWorkspace_sptr *m_ws;

  QwtValueList m_xAxisTicks;
  double m_xAngle;
  QwtValueList m_yAxisTicks;
  double m_yAngle;
};

} // namespace SliceViewer
} // namespace MantidQt

#endif /* MANTID_SLICEVIEWER_NONORTHOGONALOVERLAY_H_ */
