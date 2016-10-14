#include "MantidQtSliceViewer/NonOrthogonalOverlay.h"
#include <qwt_plot.h>
#include <qwt_plot_canvas.h>
#include <qpainter.h>
#include <QRect>
#include <QShowEvent>
#include "MantidKernel/Utils.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/IMDWorkspace.h"
#include <numeric>
using namespace Mantid::Kernel;

namespace MantidQt {
	namespace SliceViewer {

		//----------------------------------------------------------------------------------------------
		/** Constructor
		 */
		NonOrthogonalOverlay::NonOrthogonalOverlay(QwtPlot *plot, QWidget *parent)
			: QWidget(parent), m_plot(plot), m_showLine(true) {
			m_CompskewMatrix[0] = 1.0;
			m_CompskewMatrix[1] = 0.0;
			m_CompskewMatrix[2] = 0.0;
			m_CompskewMatrix[3] = 0.0;
			m_CompskewMatrix[4] = 1.0;
			m_CompskewMatrix[5] = 0.0;
			m_CompskewMatrix[6] = 0.0;
			m_CompskewMatrix[7] = 0.0;
			m_CompskewMatrix[8] = 1.0;
			//m_ws = ws;
			
			m_pointA = QPointF(0, 0);
			m_pointB = QPointF(1, 0);
			m_pointC = QPointF(0, 1);
			//calculateAxesSkew();
			m_width = 0.1;

		}

		//----------------------------------------------------------------------------------------------
		/** Destructor
		 */
		NonOrthogonalOverlay::~NonOrthogonalOverlay() {}


		/// Return the recommended size of the widget
		QSize NonOrthogonalOverlay::sizeHint() const {
			// TODO: Is there a smarter way to find the right size?
			return QSize(20000, 20000);
			// Always as big as the canvas
			// return m_plot->canvas()->size();
		}

		QSize NonOrthogonalOverlay::size() const { return m_plot->canvas()->size(); }
		int NonOrthogonalOverlay::height() const { return m_plot->canvas()->height(); }
		int NonOrthogonalOverlay::width() const { return m_plot->canvas()->width(); }

		//----------------------------------------------------------------------------------------------
		/** Tranform from plot coordinates to pixel coordinates
		 * @param coords :: coordinate point in plot coordinates
		 * @return pixel coordinates */
		QPoint NonOrthogonalOverlay::transform(QPointF coords) const {
			int xA = m_plot->transform(QwtPlot::xBottom, coords.x());
			int yA = m_plot->transform(QwtPlot::yLeft, coords.y());
			return QPoint(xA, yA);
		}

		//----------------------------------------------------------------------------------------------
		/** Inverse transform: from pixels to plot coords
		 * @param pixels :: location in pixels
		 * @return plot coordinates (float)   */
		QPointF NonOrthogonalOverlay::invTransform(QPoint pixels) const {
			double xA = m_plot->invTransform(QwtPlot::xBottom, pixels.x());
			double yA = m_plot->invTransform(QwtPlot::yLeft, pixels.y());
			return QPointF(xA, yA);
		}

		void NonOrthogonalOverlay::setAxesPoints() { //set it to be actually whatever m_x and m_y are
			auto ws = m_ws->get(); //assumes it is a square as well...
			int dim0Max = ws->getDimension(0)->getMaximum();
			m_pointA = QPointF(-(dim0Max), -(dim0Max));
			m_pointB = QPointF(dim0Max, -(dim0Max));
			m_pointC = QPointF(-(dim0Max), dim0Max);

		}
		double NonOrthogonalOverlay::getDotProductForGivenDim(int dim) {
			Mantid::Kernel::DblMatrix skewMatrix(3, 3, true); //need to put this in constructer...
			API::provideSkewMatrix(skewMatrix, *m_ws);
			skewMatrix.Invert();
			Mantid::coord_t coord_skewMatrix[3];
			Mantid::coord_t testH[3]; //rename
			std::size_t index = 0;
			if (dim == 1) {m_startPoint = 0;}
			if (dim == 2) {m_startPoint = 3;}
			if (dim == 3) {m_startPoint = 6;}
			for (std::size_t i = m_startPoint; i < m_startPoint + 3; ++i) {
				coord_skewMatrix[index] = static_cast<Mantid::coord_t>(skewMatrix[index][dim-1]);
				testH[index] = m_CompskewMatrix[i];
				++index;
			}
			auto dotProduct = std::inner_product(std::begin(coord_skewMatrix), std::end(coord_skewMatrix), std::begin(testH), 0.0);
			auto theta = std::acos(dotProduct);
			//auto cross = 
			//return dotProduct;
			return 0;
		}

		void NonOrthogonalOverlay::calculateAxesSkew(Mantid::API::IMDWorkspace_sptr *ws) {
			//to get the angle
			// figure out which dims interested in. if H/X then compare first column of skew against 1, 0, 0 (find dot product)
			// K/Y second column, then against 0, 1, 0
			// L/ third column skew against 0, 0, 1
			// then figure out cross product of the two chosen vectors
			m_ws = ws;
			setAxesPoints();
			// dimensions interested in currently hardcoded
			int dimX = 1;
			int dimY = 2;
			auto dimXdot = getDotProductForGivenDim(dimX);
			auto dimYdot = getDotProductForGivenDim(dimY);

			//then do cross product and figure out how to get sine from 
			auto theta = std::acos(dimXdot);





			
		}
		//----------------------------------------------------------------------------------------------
		/// Paint the overlay
		void NonOrthogonalOverlay::paintEvent(QPaintEvent * /*event*/) {

			QPainter painter(this);

			QPointF diff = m_pointB - m_pointA;
			// Angle of the "width" perpendicular to the line
			double angle = atan2(diff.y(), diff.x()) + M_PI / 2.0;
			QPointF widthOffset(m_width * cos(angle), m_width * sin(angle));


			QPen centerPen(QColor(0, 0, 0, 200));
			// Go back to normal drawing mode
			//painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

			// --- Draw the central line ---
			if (m_showLine) {
				centerPen.setWidth(4);
				centerPen.setCapStyle(Qt::FlatCap);
				painter.setPen(centerPen);
				painter.drawLine(transform(m_pointA), transform(m_pointB));
				painter.drawLine(transform(m_pointA), transform(m_pointC));
			}

		}

	
} // namespace Mantid
} // namespace SliceViewer
