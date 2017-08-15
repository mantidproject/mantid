#include "MantidQtWidgets/InstrumentView/MiniPlot.h"

namespace MantidQt {
using Widgets::MplCpp::Axes;
using Widgets::MplCpp::MplFigureCanvas;
namespace MantidWidgets {

/**
 * Construct an empty plot
 * @param parent A widget to be the parent
 */
MiniPlot::MiniPlot(QWidget *parent)
    : MplFigureCanvas(111, parent), m_activeCurveLabel(),
      m_storedCurveLabels() {
  setScale(Axes::Scale::X, 0, 1);
  setScale(Axes::Scale::Y, -1.2, 1.2);
}

/**
 * Set a label for the current curve
 * @param label A string key to reference the curve
 */
void MiniPlot::setCurveLabel(QString label) { m_activeCurveLabel = label; }
}
}
