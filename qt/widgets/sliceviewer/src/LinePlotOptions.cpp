#include "MantidQtWidgets/SliceViewer/LinePlotOptions.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidQtWidgets/Common/TSVSerialiser.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Geometry;

LinePlotOptions::LinePlotOptions(QWidget *parent, bool logScaleOption)
    : QWidget(parent), m_plotAxis(MantidQwtIMDWorkspaceData::PlotAuto),
      m_normalize(Mantid::API::VolumeNormalization) {
  ui.setupUi(this);

  ui.widgetLogOptions->setVisible(logScaleOption);

  addPlotRadioButton("Auto", "Automatically choose between plotting X or Y "
                             "depending on the angle of the line");
  addPlotRadioButton(
      "Distance",
      "Use the distance from the start of the line as the X axis of the plot");
  // Default to "Auto"
  m_radPlots[0]->setChecked(true);

  // Connect all the radio buttons
  QObject::connect(ui.radNoNormalization, SIGNAL(toggled(bool)), this,
                   SLOT(radNormalization_changed()));
  QObject::connect(ui.radNumEventsNormalization, SIGNAL(toggled(bool)), this,
                   SLOT(radNormalization_changed()));
  QObject::connect(ui.radVolumeNormalization, SIGNAL(toggled(bool)), this,
                   SLOT(radNormalization_changed()));
  QObject::connect(ui.ckLog10, SIGNAL(toggled(bool)), this,
                   SLOT(onYScalingChanged()));
}

LinePlotOptions::~LinePlotOptions() {}

//------------------------------------------------------------------------------
/** Add a radio button to the plot options
 *
 * @param text :: text on the radio button
 * @param tooltip :: tooltip
 * @param bIntegrated :: flag to indicate that the dimension is integrated.
 */
void LinePlotOptions::addPlotRadioButton(const std::string &text,
                                         const std::string &tooltip,
                                         const bool bIntegrated) {
  QRadioButton *rad;
  rad = new QRadioButton(ui.widgetPlotAxis);
  rad->setText(QString::fromStdString(text));
  rad->setToolTip(QString::fromStdString(tooltip));
  rad->setEnabled(!bIntegrated);
  // Insert it one before the horizontal spacer.
  QBoxLayout *layout = qobject_cast<QBoxLayout *>(ui.widgetPlotAxis->layout());
  layout->insertWidget(layout->count() - 1, rad);
  m_radPlots.push_back(rad);
  QObject::connect(rad, SIGNAL(toggled(bool)), this, SLOT(radPlot_changed()));
}

//------------------------------------------------------------------------------
/** Set the original workspace, to show the axes plot choice */
void LinePlotOptions::setOriginalWorkspace(Mantid::API::IMDWorkspace_sptr ws) {
  if (!ws)
    return;

  for (size_t d = 0; d < (ws->getNumDims()); d++) {
    IMDDimension_const_sptr dim = ws->getDimension(d);
    std::string text = dim->getName();
    std::string tooltip =
        "Use the " + dim->getName() + " dimension as the X plot axis.";
    const bool bIntegrated = dim->getIsIntegrated();
    // Index into the radio buttons array
    int index = int(d) + 2;
    if (m_radPlots.size() > index) {
      m_radPlots[index]->setText(QString::fromStdString(text));
      m_radPlots[index]->setToolTip(QString::fromStdString(tooltip));
    } else
      addPlotRadioButton(text, tooltip, bIntegrated);
  }
}

//------------------------------------------------------------------------------
/** Get the choice of X-axis to plot
 *
 * -1 : Distance from start
 * -2 : Auto-determine which axis
 * 0+ : The dimension index in the original workspace.
 *
 * @return int */
int LinePlotOptions::getPlotAxis() const { return m_plotAxis; }

//------------------------------------------------------------------------------
/** Set the choice of X-axis to plot.
 *
 * -1 : Distance from start
 * -2 : Auto-determine which axis
 * 0+ : The dimension index in the original workspace.
 *
 * @param choice :: int */
void LinePlotOptions::setPlotAxis(int choice) {
  m_plotAxis = choice;
  // Since the radPlots start corresponding with -2, the index = choice + 2
  int index = m_plotAxis + 2;
  if (index >= m_radPlots.size())
    m_plotAxis = m_radPlots.size() - 1 - 2;
  // Check the right radio button
  m_radPlots[index]->setChecked(true);
}

//------------------------------------------------------------------------------
/** Get the normalization method to use
 * @return choice of normalization */
Mantid::API::MDNormalization LinePlotOptions::getNormalization() const {
  return m_normalize;
}

/** Set the normalization method to use
 *
 * @param method :: choice of normalization
 */
void LinePlotOptions::setNormalization(Mantid::API::MDNormalization method) {
  m_normalize = method;
  // Update gui
  switch (m_normalize) {
  case Mantid::API::NoNormalization:
    ui.radNoNormalization->setChecked(true);
    break;
  case Mantid::API::VolumeNormalization:
    ui.radVolumeNormalization->setChecked(true);
    break;
  case Mantid::API::NumEventsNormalization:
    ui.radNumEventsNormalization->setChecked(true);
    break;
  }
}

//------------------------------------------------------------------------------
/** Slot called when any of the X plot choice radio buttons are clicked
 */
void LinePlotOptions::radPlot_changed() {
  for (int i = 0; i < m_radPlots.size(); i++) {
    if (m_radPlots[i]->isChecked())
      // Options start at -2 (index 0 in the radPlots)
      m_plotAxis = i - 2;
  }
  // Send out a signal
  emit changedPlotAxis();
}

//------------------------------------------------------------------------------
/** Slot called when any of the normalization choice radio buttons are clicked
 */
void LinePlotOptions::radNormalization_changed() {
  if (ui.radNoNormalization->isChecked())
    m_normalize = Mantid::API::NoNormalization;
  else if (ui.radVolumeNormalization->isChecked())
    m_normalize = Mantid::API::VolumeNormalization;
  else if (ui.radNumEventsNormalization->isChecked())
    m_normalize = Mantid::API::NumEventsNormalization;
  else
    m_normalize = Mantid::API::NoNormalization;
  // Send out a signal
  emit changedNormalization();
}

/**
 * Handler for changes to the Y-axis log scale.
 */
void LinePlotOptions::onYScalingChanged() { emit changedYLogScaling(); }

/**
 * Getter for the currently set option of the isLogScaled control.
 * @return
 */
bool LinePlotOptions::isLogScaledY() const { return ui.ckLog10->isChecked(); }

void LinePlotOptions::loadFromProject(const std::string &lines) {
  MantidQt::API::TSVSerialiser tsv(lines);

  int plotAxis, normalization;
  bool logScale;

  tsv.selectLine("PlotAxis");
  tsv >> plotAxis;
  tsv.selectLine("LogYScale");
  tsv >> logScale;
  tsv.selectLine("Normalization");
  tsv >> normalization;

  auto norm = static_cast<Mantid::API::MDNormalization>(normalization);
  setPlotAxis(plotAxis);
  ui.ckLog10->setChecked(logScale);
  setNormalization(norm);
}

std::string LinePlotOptions::saveToProject() const {
  MantidQt::API::TSVSerialiser tsv;

  tsv.writeLine("PlotAxis") << getPlotAxis();
  tsv.writeLine("LogYScale") << isLogScaledY();
  tsv.writeLine("Normalization") << static_cast<int>(getNormalization());

  return tsv.outputLines();
}
