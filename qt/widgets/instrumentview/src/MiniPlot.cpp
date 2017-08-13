#include "MantidQtWidgets/InstrumentView/MiniPlot.h"

namespace MantidQt {
using Widgets::MplCpp::MplFigureCanvas;
namespace MantidWidgets {

/**
 * Construct an empty plot
 * @param parent A widget to be the parent
 */
MiniPlot::MiniPlot(QWidget *parent) : MplFigureCanvas(111, parent) {}
}
}
