// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Plotting/Qwt/SafeQwtPlot.h"
#include "MantidAPI/Workspace.h"
#include "MantidKernel/ReadLock.h"
#include "MantidKernel/System.h"

using namespace Mantid::Kernel;

namespace MantidQt {
namespace MantidWidgets {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
SafeQwtPlot::SafeQwtPlot(QWidget *parent) : QwtPlot(parent) {}

//----------------------------------------------------------------------------------------------
/** Constructor
 */
SafeQwtPlot::SafeQwtPlot(const QwtText &title, QWidget *parent)
    : QwtPlot(title, parent) {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
SafeQwtPlot::~SafeQwtPlot() {}

//----------------------------------------------------------------------------------------------
/** Set the workspace that we read-lock when drawing.
 *
 * @param ws :: shared ptr to workspace
 */
void SafeQwtPlot::setWorkspace(Mantid::API::Workspace_sptr ws) { m_ws = ws; }

//----------------------------------------------------------------------------------------------
/** Overridden drawCanvas() that protects the
 * workspace from being overwritten while being drawn
 *
 * @param painter :: QPainter
 */
void SafeQwtPlot::drawCanvas(QPainter *painter) {
  // Do nothing if the workspace is not valid.
  if (!m_ws)
    return;
  // Get the scoped read lock.
  ReadLock lock(*m_ws);
  // Draw using the usual procedure.
  QwtPlot::drawCanvas(painter);
  // lock is released when it goes out of scope.
}

} // namespace MantidWidgets
} // namespace MantidQt
