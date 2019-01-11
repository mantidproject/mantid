// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_SLICEVIEWER_PEAKOVERLAYVIEWFACTORYBASE_H_
#define MANTID_SLICEVIEWER_PEAKOVERLAYVIEWFACTORYBASE_H_

#include "DllOption.h"
#include "MantidQtWidgets/SliceViewer/PeakOverlayViewFactory.h"
#include <QColor>
#include <QWidget>
#include <qwt_plot.h>

namespace MantidQt {
namespace SliceViewer {

/** Base class for Concrete view factories. Provides common functionality. This
is abstract.

@date 2012-08-24
*/
class EXPORT_OPT_MANTIDQT_SLICEVIEWER PeakOverlayViewFactoryBase
    : public PeakOverlayViewFactory {
protected:
  QwtPlot *m_plot;
  QWidget *m_parent;
  PeakViewColor m_foregroundViewColor;
  PeakViewColor m_backgroundViewColor;
  const int m_plotXIndex;
  const int m_plotYIndex;

public:
  PeakOverlayViewFactoryBase(QwtPlot *plot, QWidget *parent,
                             const int plotXIndex, const int plotYIndex,
                             const size_t colourNumber = 0);
  ~PeakOverlayViewFactoryBase() override;
  std::string getPlotXLabel() const override;
  std::string getPlotYLabel() const override;
};
} // namespace SliceViewer
} // namespace MantidQt

#endif /*MANTID_SLICEVIEWER_PEAKOVERLAYVIEWFACTORYBASE_H_*/
