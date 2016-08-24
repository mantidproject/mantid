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
using namespace Mantid::Kernel;

namespace MantidQt {
	namespace SliceViewer {

		//----------------------------------------------------------------------------------------------
		/** Constructor
		 */
		NonOrthogonalOverlay::NonOrthogonalOverlay(QwtPlot *plot, QWidget *parent, Mantid::API::IMDWorkspace_sptr *ws)
			: QWidget(parent), m_plot(plot), m_showLine(true) {

			m_ws = ws;
			calculateAxesSkew();
			m_pointA = QPointF(-4.0, -4.0);
			m_pointB = QPointF(-4.0, 5.0);
			m_pointC = QPointF(5.0, -4.0);
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

		void NonOrthogonalOverlay::calculateAxesSkew() {
			auto test = m_ws->get();
			
		}
		//----------------------------------------------------------------------------------------------
		/// Paint the overlay
		void NonOrthogonalOverlay::paintEvent(QPaintEvent * /*event*/) {
			// Don't paint until created
			// Also, don't paint while right-click dragging (panning) the underlying pic


			QPainter painter(this);

			QPointF diff = m_pointB - m_pointA;
			// Angle of the "width" perpendicular to the line
			double angle = atan2(diff.y(), diff.x()) + M_PI / 2.0;
			QPointF widthOffset(m_width * cos(angle), m_width * sin(angle));

			QPen boxPenDark(QColor(255, 215, 0, 200));
			QPen centerPen(QColor(192, 192, 192, 128));

			boxPenDark.setWidthF(4.0);

			// Go back to normal drawing mode
			painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

			// --- Draw the central line ---
			if (m_showLine) {
				centerPen.setWidth(2);
				centerPen.setCapStyle(Qt::FlatCap);
				painter.setPen(boxPenDark);
				painter.drawLine(transform(m_pointA), transform(m_pointB));
				painter.drawLine(transform(m_pointA), transform(m_pointC));
			}

		}

	
} // namespace Mantid
} // namespace SliceViewer
