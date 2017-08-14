#include "MantidQtWidgets/InstrumentView/MiniPlot.h"

#include <QVBoxLayout>

namespace MantidQt {
using Widgets::MplCpp::MplFigureCanvas;
namespace MantidWidgets {

/**
 * Construct an empty plot
 * @param parent A widget to be the parent
 */
MiniPlot::MiniPlot(QWidget *parent)
    : QWidget(parent), m_canvas(new MplFigureCanvas(111, this)) {
  setLayout(new QVBoxLayout);
  layout->addWidget(m_canvas);
}
}
}
