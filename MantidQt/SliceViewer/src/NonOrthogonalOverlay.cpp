#include "MantidQtSliceViewer/NonOrthogonalOverlay.h"
#include <qwt_plot.h>
#include <qwt_plot_canvas.h>
#include <qpainter.h>
#include <QRect>
#include <QShowEvent>
#include "MantidQtSliceViewer\QConversionScale.h"
#include "MantidKernel/Utils.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidQtAPI/NonOrthogonal.h"
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
			m_showLine = false;
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
				for (std::size_t j = 0; j < skewMatrix.numCols(); ++j) {
					m_skewMatrix[index] =
						static_cast<Mantid::coord_t>(skewMatrix[i][j]);
					++index;
				}
			}
		}

        void NonOrthogonalOverlay::setDefaultAxesPoints() {
            auto ws = m_ws->get(); // assumes it is a rectangle
            m_dim0Max = ws->getDimension(0)->getMaximum();
			//m_dim0Max = m_dim0Max*1.1; //to set axis slightly back from slice
            m_dim1 = ws->getDimension(1)->getMaximum();
            m_dim2 = ws->getDimension(2)->getMaximum();
        }

        QPointF NonOrthogonalOverlay::skewMatrixApply(double x, double y) {
            // keyed array,
            std::vector<double> dimensions(3, 0);
            dimensions.at(m_dimX) = x;
            dimensions.at(m_dimY) = y;
            // Make sure stops trying to calculate this m_dimX or Y is 3+
            // put some of this into setAxisPoints()
            double angle_H = dimensions[0];
            double angle_K = dimensions[1];
            double angle_L = dimensions[2];

            auto dimX = angle_H * m_skewMatrix[0 + 3 * m_dimX] +
                        angle_K * m_skewMatrix[1 + 3 * m_dimX] +
                        angle_L * m_skewMatrix[2 + 3 * m_dimX];
            auto dimY = angle_H * m_skewMatrix[0 + 3 * m_dimY] +
                        angle_K * m_skewMatrix[1 + 3 * m_dimY] +
                        angle_L * m_skewMatrix[2 + 3 * m_dimY];

			return QPointF(dimX, dimY);
				  
        }

        void NonOrthogonalOverlay::zoomChanged(QwtDoubleInterval xint,
                                                QwtDoubleInterval yint) {
            m_xMinVis = xint.minValue();
            m_xMaxVis = xint.maxValue();
            m_yMinVis = yint.minValue();
            m_yMaxVis = yint.maxValue();

            int displayNum = 20; // can mess around increasing or
                                // decreasing grid later, maybe make an
                                // option on sliceviewgui

            calculateTickMarks(displayNum);
				  
        }

        void NonOrthogonalOverlay::setAxesPoints() {
	        m_originPoint = -(m_dim0Max);
			m_XEndPoint = m_dim0Max;
			m_YEndPoint = m_dim0Max;
			m_pointA = skewMatrixApply(m_originPoint, m_originPoint);
			m_pointB = skewMatrixApply(m_XEndPoint, m_originPoint);
			m_pointC = skewMatrixApply(m_originPoint, m_YEndPoint);
        }

        void NonOrthogonalOverlay::calculateAxesSkew(
            Mantid::API::IMDWorkspace_sptr *ws, size_t dimX,
            size_t dimY) {
            m_ws = ws;
            m_dimX = dimX;
            m_dimY = dimY;
            // m_missingHKL = //currently finding missing dim via process
            // of elimination
            //    API::getMissingHKLDimensionIndex(*m_ws, m_dimX, m_dimY);
            setDefaultAxesPoints();

            if (API::isHKLDimensions(
                    *m_ws, m_dimX,
                    m_dimY)) { // and skew actually _wants_ to be visible
            // make sure all calcs are outside of paintEvent... so
            // probably move ApplyskewMatrix out of it
            setSkewMatrix();
            setAxesPoints();
			                 // dataset is, or just zoom level?
				

            }
        }

        void NonOrthogonalOverlay::clearAllAxisPointVectors() {
            m_axisXPointVec.clear();
			m_axisYPointVec.clear();
            m_xAxisTickStartVec.clear();
            m_yAxisTickStartVec.clear();
            m_xAxisTickEndVec.clear();
            m_yAxisTickEndVec.clear();
			m_xNumbers.clear();
			m_yNumbers.clear();
        }

		void NonOrthogonalOverlay::calculateTickMarks(
			int tickNum) { // assumes X axis
			clearAllAxisPointVectors();
			double xBuffer = (m_xMaxVis - m_xMinVis);
			double yBuffer = (m_yMaxVis - m_yMinVis);
			m_xMaxVis = m_xMaxVis + xBuffer;
			m_xMinVis = m_xMinVis - xBuffer;
			m_yMaxVis = m_yMaxVis + yBuffer;
			m_yMinVis = m_yMinVis - yBuffer;
			double axisPointX;
			double axisPointY;
			double percentageOfLineX = (
				((m_xMaxVis ) - (m_xMinVis)) /
				tickNum);
			double percentageOfLineY = (
				((m_yMaxVis ) - (m_yMinVis)) /
				tickNum);
			for (int i = 0; i <= tickNum; i++) {
				axisPointX = (percentageOfLineX * i) + m_xMinVis;
				axisPointY = (percentageOfLineY * i) + m_yMinVis;
				m_axisXPointVec.push_back(axisPointX);
				m_xNumbers.push_back(skewMatrixApply(axisPointX, m_yMinVis));
				m_xNumbers[i].setY(m_yMinVis + yBuffer);
				m_yNumbers.push_back(skewMatrixApply(m_xMinVis, axisPointY));
				m_yNumbers[i].setX(m_xMinVis + xBuffer);
				m_xAxisTickStartVec.push_back(
					skewMatrixApply(axisPointX, (m_yMinVis)));
				m_xAxisTickEndVec.push_back(
					skewMatrixApply(axisPointX, (m_yMaxVis)));
				m_axisYPointVec.push_back(axisPointY);
				m_yAxisTickStartVec.push_back(
					skewMatrixApply((m_xMinVis), axisPointY));
				m_yAxisTickEndVec.push_back(
					skewMatrixApply((m_xMaxVis), axisPointY));
			}
			update();
		}

				//----------------------------------------------------------------------------------------------
		/// Paint the overlay
		void NonOrthogonalOverlay::paintEvent(QPaintEvent * /*event*/) {

			QPainter painter(this);

            QPen centerPen(QColor(0, 0, 0, 200)); //black
            QPen gridPen(QColor(160, 160, 160, 100)); //grey
			QPen numberPen(QColor(160, 160, 160, 255));
            // --- Draw the central line ---
			if (m_showLine) {
                centerPen.setWidth(2);
                centerPen.setCapStyle(Qt::SquareCap);
				painter.setPen(centerPen);
				painter.drawLine(transform(m_pointA), transform(m_pointB));
				painter.drawLine(transform(m_pointA), transform(m_pointC));
                gridPen.setWidth(1);
                gridPen.setCapStyle(Qt::FlatCap);
				gridPen.setStyle(Qt::DashLine);
				
				
				for (int i = 0; i < m_axisYPointVec.size(); i++) {
					painter.setPen(gridPen);
					painter.drawLine(
						transform(m_xAxisTickStartVec[i]),
						transform(m_xAxisTickEndVec[i]));
					painter.drawLine(transform(m_yAxisTickStartVec[i]), transform(m_yAxisTickEndVec[i]));
					painter.setPen(numberPen);
					painter.drawText(transform(m_xNumbers[i]), QString::number(m_axisXPointVec[i], 'g', 3));
					painter.drawText(transform(m_yNumbers[i]), QString::number(m_axisYPointVec[i], 'g', 3));
				  }

          }
			
		}

	
} // namespace Mantid
} // namespace SliceViewer
