#include "MantidQtSliceViewer/LineViewer.h"
#include <qwt_plot_curve.h>
#include "MantidKernel/VMD.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"

using namespace Mantid;
using namespace Mantid::Kernel;

namespace MantidQt
{
namespace SliceViewer
{


LineViewer::LineViewer(QWidget *parent)
    : QWidget(parent)
{
	ui.setupUi(this);

	// --------- Create the plot -----------------
  m_plotLayout = new QHBoxLayout(ui.frmPlot);
  m_plot = new QwtPlot();
  m_plot->autoRefresh();
  m_plot->setBackgroundColor(QColor(255,255,255)); // White background
  m_plotLayout->addWidget(m_plot, 1);

  // Make the splitter use the minimum size for the controls and not stretch out
  ui.splitter->setStretchFactor(0, 0);
  ui.splitter->setStretchFactor(1, 1);

}

LineViewer::~LineViewer()
{

}

//-----------------------------------------------------------------------------------------------
/** With the workspace set, create the dimension text boxes */
void LineViewer::createDimensionWidgets()
{
  // Create all necessary widgets
  if (m_startText.size() < int(m_ws->getNumDims()))
  {
    for (size_t d=m_startText.size(); d<m_ws->getNumDims(); d++)
    {
      QLabel * dimLabel = new QLabel(this);
      dimLabel->setAlignment(Qt::AlignHCenter);
      ui.gridLayout->addWidget(dimLabel, 0, int(d)+1);
      m_dimensionLabel.push_back(dimLabel);

      QLineEdit * startText = new QLineEdit(this);
      QLineEdit * endText = new QLineEdit(this);
      QLineEdit * widthText = new QLineEdit(this);
      startText->setMaximumWidth(120);
      endText->setMaximumWidth(120);
      widthText->setMaximumWidth(120);
      startText->setToolTip("Start point of the line in this dimension");
      endText->setToolTip("End point of the line in this dimension");
      widthText->setToolTip("Width of the line in this dimension");
      ui.gridLayout->addWidget(startText, 1, int(d)+1);
      ui.gridLayout->addWidget(endText, 2, int(d)+1);
      ui.gridLayout->addWidget(widthText, 3, int(d)+1);
      m_startText.push_back(startText);
      m_endText.push_back(endText);
      m_widthText.push_back(widthText);
    }
  }

  // ------ Update the widgets -------------------------
  for (int d=0; d<int(m_ws->getNumDims()); d++)
  {
    m_dimensionLabel[d]->setText( QString::fromStdString(m_ws->getDimension( size_t(d))->getName() ) );
  }

}


// ==============================================================================================
// ================================== External Setters ==========================================
// ==============================================================================================
//-----------------------------------------------------------------------------------------------
/** Set the workspace being sliced
 *
 * @param ws :: IMDWorkspace */
void LineViewer::setWorkspace(Mantid::API::IMDWorkspace_sptr ws)
{
  m_ws = ws;
  createDimensionWidgets();
}


/** Set the start point of the line to integrate
 * @param start :: vector for the start point */
void LineViewer::setStart(Mantid::Kernel::VMD start)
{
  m_start = start;
}

/** Set the end point of the line to integrate
 * @param end :: vector for the end point */
void LineViewer::setEnd(Mantid::Kernel::VMD end)
{
  m_end = end;
}

/** Set the number of bins in the line
 * @param nbins :: # of bins */
void LineViewer::setNumBins(size_t numBins)
{
  m_numBins = numBins;
}



// ==============================================================================================
// ================================== Rendering =================================================
// ==============================================================================================
/** Calculate and show the preview (non-integrated) line */
void LineViewer::showPreview()
{
  if (!m_ws) return;

  // Use the width of the plot (in pixels) to choose the fineness)
  // That way, there is ~1 point per pixel = as fine as it needs to be
  size_t numPoints = size_t(m_plot->width());
  if (numPoints < 20) numPoints = 20;

  VMD step = (m_end-m_start) / double(numPoints);
  double stepLength = step.norm();

  // These will be the curve as plotted
  double * x = new double[numPoints];
  double * y = new double[numPoints];

  for (size_t i=0; i<numPoints; i++)
  {
    // Coordinate along the line
    VMD coord = m_start + step * double(i);
    // Signal in the WS at that coordinate
    signal_t signal = m_ws->getSignalAtCoord(coord);
    // Make into array
    x[i] = stepLength * double(i);
    y[i] = signal;
  }

  // Make the curve
  QwtPlotCurve *curve = new QwtPlotCurve("Preview");
  curve->setData(x,y, int(numPoints));
  curve->attach(m_plot);
  m_plot->replot();
  m_plot->setTitle("Preview Plot");

  delete [] x;
  delete [] y;
}


} // namespace
}
