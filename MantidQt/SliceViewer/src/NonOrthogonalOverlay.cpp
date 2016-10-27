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
				  m_dim0Max = m_dim0Max*1.1; //to set axis slightly back from slice
                                  m_totalArea =
                                      (m_dim0Max * 2) * (m_dim0Max * 2);
                  m_dim1 = ws->getDimension(1)->getMaximum();
                  m_dim2 = ws->getDimension(2)->getMaximum();
                }
                QPointF NonOrthogonalOverlay::skewMatrixApply(double x,
                                                              double y) {
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
                  double xMin = xint.minValue();
                  double xMax = xint.maxValue();
                  double yMin = yint.minValue();
                  double yMax = yint.maxValue();
                  double totalAreaShown = (xMax - xMin) * (yMax - yMin);
                  // just concn on x axis for now
                  double xVisible = totalAreaShown / m_totalArea;
                  xVisible = xVisible * 100;
                  int displayNum = 0;
                  if (xVisible > 200) {
                    displayNum = 10;
                  } else if (xVisible < 200 && xVisible > 100) {
                    displayNum = 15;
                  } else if (xVisible < 100) {
                    displayNum = 20;
                  } else {
                    displayNum = 15;
                  }

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

                  if (API::isHKLDimensions(*m_ws, m_dimX, m_dimY)) {
					  //make sure all calcs are outside of paintEvent... so probably move ApplyskewMatrix out of it
                    setSkewMatrix();
                    setAxesPoints();
                    calculateTickMarks(10); // tie zoom level to how large
                                            // dataset is, or just zoom level?
                  }
                }

                void NonOrthogonalOverlay::clearAllAxisPointVectors() {
                  m_axisPointVec.clear();
                  m_xAxisTickStartVec.clear();
                  m_yAxisTickStartVec.clear();
                  m_xAxisTickEndVec.clear();
                  m_yAxisTickEndVec.clear();
                }

                void NonOrthogonalOverlay::calculateTickMarks(
                    int tickNum) { // assumes X axis
                  clearAllAxisPointVectors();
                  double axisPoint;
                  double percentageOfLine((m_XEndPoint - m_originPoint) / tickNum);
                  for (int i = 0; i <= tickNum; i++) {
					axisPoint = (percentageOfLine * i) + m_originPoint;
					m_axisPointVec.push_back(axisPoint);
                    m_xAxisTickStartVec.push_back(skewMatrixApply(axisPoint, m_originPoint));
                    m_xAxisTickEndVec.push_back(skewMatrixApply(axisPoint, m_XEndPoint));
                                                // m_originPoint * 1.05));
                    m_yAxisTickStartVec.push_back(skewMatrixApply(m_originPoint, axisPoint));
                    m_yAxisTickEndVec.push_back(skewMatrixApply(m_XEndPoint, axisPoint));
                                                        // m_originPoint * 1.05,
                                                        
                    }

				}

				//----------------------------------------------------------------------------------------------
		/// Paint the overlay
		void NonOrthogonalOverlay::paintEvent(QPaintEvent * /*event*/) {

			QPainter painter(this);

            QPen centerPen(QColor(0, 0, 0, 200)); //black
            QPen gridPen(QColor(160, 160, 160, 200)); // grey

            // --- Draw the central line ---
			if (m_showLine) {
                          centerPen.setWidth(2);
                                centerPen.setCapStyle(Qt::SquareCap);
				painter.setPen(centerPen);
				painter.drawLine(transform(m_pointA), transform(m_pointB));
				painter.drawLine(transform(m_pointA), transform(m_pointC));
                                gridPen.setWidth(1);
                                gridPen.setCapStyle(Qt::FlatCap);
				painter.setPen(gridPen);
				
				for (int i = 0; i < m_axisPointVec.size(); i++) {
                                  painter.drawLine(
                                      transform(m_xAxisTickStartVec[i]),
                                      transform(m_xAxisTickEndVec[i]));
                                  painter.drawLine(
                                      transform(m_yAxisTickStartVec[i]),
                                      transform(m_yAxisTickEndVec[i]));
                                }



				

				
				//to remove axis (also need to renable them after...)
				//m_plot->enableAxis(QwtPlot::yLeft, false);
				//m_plot->enableAxis(QwtPlot::xBottom, false);
                                // m_plot->setAxisScale(QwtPlot::xBottom, 0,
                                // 100);
                        }

		}

	
} // namespace Mantid
} // namespace SliceViewer
