#include "MantidQtWidgets/MplCpp/ColorbarWidget.h"
#include "MantidQtWidgets/MplCpp/Colors.h"
#include "MantidQtWidgets/MplCpp/Figure.h"
#include "MantidQtWidgets/MplCpp/FigureCanvasQt.h"

#include <QComboBox>
#include <QDoubleValidator>
#include <QLineEdit>
#include <QVBoxLayout>

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

namespace {
// These values control the dimensions of the axes used
// to hold the colorbar. The aspect ratio is set to give
// the usual long thin colobar
constexpr double AXES_LEFT = 0.4;
constexpr double AXES_BOTTOM = 0.05;
constexpr double AXES_WIDTH = 0.2;
constexpr double AXES_HEIGHT = 0.9;
} // namespace

/**
 * @brief Construct a default color bar with a linear scale. The default limits
 * are set to [0, 1] so setRange would need to be called at a minimum
 * @param parent A pointer to the parent widget
 */
ColorbarWidget::ColorbarWidget(QWidget *parent)
    : QWidget(parent), m_ui(),
      m_mappable(Normalize(0, 1), getCMap(defaultCMapName())), m_colorbar() {
  initLayout();
  connectSignals();
}

/**
 * Update the range of the scale
 * @param vmin An optional new minimum of the scale
 * @param vmax An optional new maximum of the scale
 */
void ColorbarWidget::setClim(boost::optional<double> vmin,
                             boost::optional<double> vmax) {
  m_mappable.setClim(vmin, vmax);
  m_canvas->draw();

  if (vmin.is_initialized()) {
    m_ui.scaleMinEdit->setText(QString::number(vmin.get()));
    emit minValueChanged(vmin.get());
  }
  if (vmax.is_initialized()) {
    m_ui.scaleMaxEdit->setText(QString::number(vmax.get()));
    emit maxValueChanged(vmax.get());
  }
}

/**
 * @return A tuple giving the current colorbar scale limits
 */
std::tuple<double, double> ColorbarWidget::clim() const {
  return std::make_tuple<double, double>(m_ui.scaleMinEdit->text().toDouble(),
                                         m_ui.scaleMaxEdit->text().toDouble());
}

// ------------------------------ Legacy API -----------------------------------

/**
 * Update the minimum value of the normalization scale
 * @param vmin New minimum of the scale
 */
void ColorbarWidget::setMinValue(double vmin) { setClim(vmin, boost::none); }

/**
 * Update the maximum value of the normalization scale
 * @param vmin New maximum of the scale
 */
void ColorbarWidget::setMaxValue(double vmax) { setClim(boost::none, vmax); }

/**
 * @return The minimum color scale value as a string
 */
QString ColorbarWidget::getMinValue() const {
  return QString::number(std::get<0>(clim()));
}

/**
 * @return The maximum color scale value as a string
 */
QString ColorbarWidget::getMaxValue() const {
  return QString::number(std::get<1>(clim()));
}

// --------------------------- Private slots -----------------------------------
/**
 * Called when a user has edited the minimum scale value
 */
void ColorbarWidget::scaleMinimumEdited() {
  // The validator ensures the text is a double
  setClim(m_ui.scaleMinEdit->text().toDouble(), boost::none);
}

/**
 * Called when a user has edited the maximum scale value
 */
void ColorbarWidget::scaleMaximumEdited() {
  // The validator ensures the text is a double
  setClim(boost::none, m_ui.scaleMaxEdit->text().toDouble());
}

/**
 * Called when a new selection in the scale type box is made
 */
void ColorbarWidget::scaleTypeSelectionChanged(int index) {
  if (index == 2) { // Power
    m_ui.powerEdit->show();
  } else {
    m_ui.powerEdit->hide();
  }
}

/**
 * Setup the layout of the child widgets
 */
void ColorbarWidget::initLayout() {
  // Create figure and colorbar
  Figure fig{false};
  fig.pyobj().attr("set_facecolor")("w");
  Axes cbAxes{fig.addAxes(AXES_LEFT, AXES_BOTTOM, AXES_WIDTH, AXES_HEIGHT)};
  m_colorbar = fig.colorbar(m_mappable, cbAxes);

  // Create widget layout including canvas to draw the figure
  m_ui.setupUi(this);
  // remove placeholder widget and add figure canvas
  delete m_ui.mplColorbar;
  m_canvas = new FigureCanvasQt(fig, this);
  m_ui.mplColorbar = m_canvas;
  m_ui.verticalLayout->insertWidget(1, m_canvas);

  // Set validators on the scale input
  m_ui.scaleMinEdit->setValidator(new QDoubleValidator());
  m_ui.scaleMaxEdit->setValidator(new QDoubleValidator());

  // Set default properties for scale type 0
  scaleTypeSelectionChanged(0);
}

/**
 * Wire up the signals for the child widgets
 */
void ColorbarWidget::connectSignals() {
  connect(m_ui.scaleMinEdit, SIGNAL(editingFinished()), this,
          SLOT(scaleMinimumEdited()));
  connect(m_ui.scaleMaxEdit, SIGNAL(editingFinished()), this,
          SLOT(scaleMaximumEdited()));

  connect(m_ui.scaleTypeOpt, SIGNAL(currentIndexChanged(int)), this,
          SLOT(scaleTypeSelectionChanged(int)));
}

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt
