#include "MantidQtMantidWidgets/MWSpectrogram.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidKernel/ReadLock.h"


namespace MantidQt
{
namespace MantidWidgets
{


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  MWSpectrogram::MWSpectrogram(QWidget * parent)
  : QwtPlot(parent)
  {
  }


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  MWSpectrogram::MWSpectrogram(const QwtText &title, QWidget * parent)
  : QwtPlot(title, parent)
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  MWSpectrogram::~MWSpectrogram()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /** Set the workspace that we read-lock when drawing.
   *
   * @param ws :: shared ptr to workspace
   */
  void MWSpectrogram::setWorkspace(Mantid::API::MatrixWorkspace_sptr ws)
  {
    m_ws = ws;
  }

  //----------------------------------------------------------------------------------------------
  /** Overridden drawCanvas() that protects the
   * workspace from being overwritten while being drawn
   *
   * @param painter :: QPainter
   */
  void MWSpectrogram::drawCanvas(QPainter * painter)
  {
    // Do nothing if the workspace is not valid.
    if (!m_ws) return;
    // Get the scoped read lock.
    Mantid::Kernel::ReadLock lock(*m_ws);
    // Draw using the usual procedure.
    QwtPlot::drawCanvas(painter);
    // lock is released when it goes out of scope.
  }

} // namespace MantidQt
} // namespace MantidWidgets
