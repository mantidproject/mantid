#ifndef MANTID_SLICEVIEWER_NONORTHOGONALOVERLAY_H_
#define MANTID_SLICEVIEWER_NONORTHOGONALOVERLAY_H_

#include "DllOption.h"
#include <QtCore/QtCore>
#include <QtGui/qwidget.h>
#include <qwt_plot.h>
#include <qpainter.h>
#include "MantidKernel/System.h"
#include "MantidQtAPI/QwtRasterDataMD.h"
#include "MantidQtAPI/QwtRasterDataMDNonOrthogonal.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidQtAPI/NonOrthogonal.h"
#include "MantidKernel/Matrix.h"

namespace MantidQt {
namespace SliceViewer {

/** GUI for overlaying a nonorthogonal axes onto the plot
  in the SliceViewer. Should be generic to overlays on any QwtPlot.

  @date 2016-08-23

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
class EXPORT_OPT_MANTIDQT_SLICEVIEWER NonOrthogonalOverlay : public QWidget {
	Q_OBJECT

public:
	NonOrthogonalOverlay(QwtPlot *plot, QWidget *parent);
	~NonOrthogonalOverlay() override;

        bool m_showLine;

        void calculateAxesSkew(Mantid::API::IMDWorkspace_sptr *ws, size_t dimX,
                               size_t dimY);

      private:
        Mantid::coord_t m_skewMatrix[9];
        void setAxesPoints();
        void setSkewMatrix();
        void setDefaultAxesPoints();
        QPointF skewMatrixApply(double x, double y);
        double m_dim0Max;
        double m_dim1;
        double m_dim2;
        size_t m_missingHKL;
        Mantid::API::IMDWorkspace_sptr *m_ws;

        Mantid::coord_t m_CompskewMatrix[9];
        size_t m_dimY;
        size_t m_dimX;

        QSize sizeHint() const override;
        QSize size() const;
        int height() const;
        int width() const;

		double m_originPoint;
		double m_XEndPoint;
		double m_YEndPoint;
		std::vector <double> m_axisPointVec;
        /// First point of the line (in coordinates of the plot)
	QPointF m_pointA;
	/// Second point of the line (in coordinates of the plot)
	QPointF m_pointB;
	/// Third point of the line (in coordinates of the plot)
	QPointF m_pointC;
	/// Width of the line (in coordinates of the plot)
	double m_width;
	//QRect drawHandle(QPainter &painter, QPointF coords, QColor brush);
	void paintEvent(QPaintEvent *event) override;
	QPoint transform(QPointF coords) const;
	QPointF invTransform(QPoint pixels) const;
	/// QwtPlot containing this
	QwtPlot *m_plot;
	void calculateGridlines(int gridLineNum);
};

} // namespace SliceViewer
} // namespace Mantid

#endif /* MANTID_SLICEVIEWER_NONORTHOGONALOVERLAY_H_ */
