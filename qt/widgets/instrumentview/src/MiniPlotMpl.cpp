#include "MantidQtWidgets/InstrumentView/MiniPlotMpl.h"
#include "MantidQtWidgets/MplCpp/FigureCanvasQt.h"
#include <QVBoxLayout>

namespace MantidQt {
using Widgets::MplCpp::FigureCanvasQt;

namespace MantidWidgets {

/**
 * Construct a blank miniplot with a single subplot
 * @param parent A pointer to its parent widget
 */
MiniPlotMpl::MiniPlotMpl(QWidget *parent)
    : QWidget(parent), m_canvas(new FigureCanvasQt(111)) {
  setLayout(new QVBoxLayout);
  layout()->addWidget(m_canvas);
}

void MiniPlotMpl::setData(std::vector<double> x, std::vector<double> y,
                          const std::string &xUnits) {}

} // namespace MantidWidgets
} // namespace MantidQt
