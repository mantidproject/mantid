#include "MantidQtWidgets/SliceViewer/PeakRepresentation.h"
#include "MantidQtWidgets/SliceViewer/PeakViewColor.h"
#include <QPainter>

namespace MantidQt {
namespace SliceViewer {

/**
 * Template method which draws a peaks representation
 * @param painter: a QPainter to draw peak information onto the screen
 * @param foregroundColor: the color of the peak markers
 * @param backgroundColor: the color of the background
 * @param viewInformation: information about the view into which the peak is
 * drawn
 */
void PeakRepresentation::draw(
    QPainter &painter, PeakViewColor &foregroundColor,
    PeakViewColor &backgroundColor,
    PeakRepresentationViewInformation viewInformation) {
  // Setup the drawing information, eg positions, radii ...
  auto drawingInformation = getDrawingInformation(viewInformation);

  // Draw
  doDraw(painter, foregroundColor, backgroundColor, drawingInformation,
         viewInformation);
}
} // namespace SliceViewer
} // namespace MantidQt
