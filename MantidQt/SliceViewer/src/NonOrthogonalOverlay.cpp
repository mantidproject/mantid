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
			
			m_pointA = QPointF(0, 0);
			m_pointB = QPointF(1, 0);
			m_pointC = QPointF(0, 1);

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

                void NonOrthogonalOverlay::setSkewMatrix() {
                  Mantid::Kernel::DblMatrix skewMatrix(3, 3, true);
                        API::provideSkewMatrix(skewMatrix, *m_ws);
			skewMatrix.Invert();
                        // transform from double to coord_t
                        std::size_t index = 0;
                        for (std::size_t i = 0; i < skewMatrix.numRows(); ++i) {
                          for (std::size_t j = 0; j < skewMatrix.numCols();
                               ++j) {
                            m_skewMatrix[index] =
                                static_cast<Mantid::coord_t>(skewMatrix[i][j]);
                            ++index;
                          }
                        }
		}

                void NonOrthogonalOverlay::setDefaultAxesPoints() {
                  auto ws = m_ws->get(); // assumes it is a rectangle
                  m_dim0Max = ws->getDimension(0)->getMaximum();
                }
                QPointF NonOrthogonalOverlay::skewMatrixApply(int x, int y) {

                  auto dimX = x * m_skewMatrix[0 + 3 * m_dimX] +
                              y * m_skewMatrix[1 + 3 * m_dimX];
                  auto dimY = x * m_skewMatrix[0 + 3 * m_dimY] +
                              y * m_skewMatrix[1 + 3 * m_dimY];
                  return QPointF(dimX, dimY);
                }

                void NonOrthogonalOverlay::setAxesPoints() {
                  m_pointA = skewMatrixApply(-(m_dim0Max), -(m_dim0Max));
                  m_pointB = skewMatrixApply(m_dim0Max, -(m_dim0Max));
                  m_pointC = skewMatrixApply(-(m_dim0Max), m_dim0Max);
                }

                void NonOrthogonalOverlay::calculateAxesSkew(
                    Mantid::API::IMDWorkspace_sptr *ws, size_t dimX,
                    size_t dimY) {
                  m_ws = ws;
                  m_dimX = dimX;
                  m_dimY = dimY;
                  setDefaultAxesPoints();
                  setSkewMatrix();
                  setAxesPoints();
                }
		//----------------------------------------------------------------------------------------------
		/// Paint the overlay
		void NonOrthogonalOverlay::paintEvent(QPaintEvent * /*event*/) {

			QPainter painter(this);

                        QPen centerPen(QColor(255, 165, 0, 200));

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
