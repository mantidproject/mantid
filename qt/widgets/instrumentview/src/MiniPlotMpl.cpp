#include "MantidQtWidgets/InstrumentView/MiniPlotMpl.h"
#include "MantidQtWidgets/MplCpp/FigureCanvasQt.h"
#include <QVBoxLayout>

namespace {
/// Active curve format
const char *ACTIVE_CURVE_FORMAT = "k-";
} // namespace

namespace MantidQt {
using Widgets::MplCpp::FigureCanvasQt;

namespace MantidWidgets {

/**
 * Construct a blank miniplot with a single subplot
 * @param parent A pointer to its parent widget
 */
MiniPlotMpl::MiniPlotMpl(QWidget *parent)
    : QWidget(parent), m_canvas(new FigureCanvasQt(111)), m_lines() {
  setLayout(new QVBoxLayout);
  layout()->addWidget(m_canvas);
}

void MiniPlotMpl::setData(std::vector<double> x, std::vector<double> y,
                          const std::string &xUnits) {
  m_lines.clear();
  auto axes = m_canvas->gca();
  m_lines.emplace_back(
      axes.plot(std::move(x), std::move(y), ACTIVE_CURVE_FORMAT));
  axes.setXLabel(xUnits.c_str());
  m_canvas->draw();
}

} // namespace MantidWidgets
} // namespace MantidQt
