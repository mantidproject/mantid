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

Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
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
