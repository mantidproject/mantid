#include "../inc/MantidQtSliceViewer/LinePlotOptions.h"

LinePlotOptions::LinePlotOptions(QWidget *parent)
    : QWidget(parent)
{
	ui.setupUi(this);

	// Connect all the radio buttons
  QObject::connect(ui.radPlotAuto, SIGNAL(toggled(bool)), this, SLOT(radPlot_changed()));
  QObject::connect(ui.radPlotX, SIGNAL(toggled(bool)), this, SLOT(radPlot_changed()));
  QObject::connect(ui.radPlotY, SIGNAL(toggled(bool)), this, SLOT(radPlot_changed()));
  QObject::connect(ui.radPlotDistance, SIGNAL(toggled(bool)), this, SLOT(radPlot_changed()));

  QObject::connect(ui.radNoNormalization, SIGNAL(toggled(bool)), this, SLOT(radNormalization_changed()));
  QObject::connect(ui.radNumEventsNormalization, SIGNAL(toggled(bool)), this, SLOT(radNormalization_changed()));
  QObject::connect(ui.radVolumeNormalization, SIGNAL(toggled(bool)), this, SLOT(radNormalization_changed()));

}

LinePlotOptions::~LinePlotOptions()
{

}

//------------------------------------------------------------------------------
/** Set the names of the dimensions in X/Y of the slice viewer
 *
 * @param xName :: name of the X dimension
 * @param yName :: name of the Y dimension
 */
void LinePlotOptions::setXYNames(const std::string & xName, const std::string & yName)
{
  ui.radPlotX->setText(QString::fromStdString("X (" + xName + ")"));
  ui.radPlotY->setText(QString::fromStdString("Y (" + yName + ")"));
}

//------------------------------------------------------------------------------
/** Get the choice of X-axis to plot
 *
 * @return option from PlotAxisChoice enum */
MantidQwtIMDWorkspaceData::PlotAxisChoice LinePlotOptions::getPlotAxis() const
{
  return m_plotAxis;
}


/** Set the choice of X-axis to plot
 *
 * @param choice :: option from PlotAxisChoice enum */
void LinePlotOptions::setPlotAxis(MantidQwtIMDWorkspaceData::PlotAxisChoice choice)
{
  m_plotAxis = choice;
  switch(m_plotAxis)
  {
  case MantidQwtIMDWorkspaceData::PlotX:
    ui.radPlotX->setChecked(true);
    break;
  case MantidQwtIMDWorkspaceData::PlotY:
    ui.radPlotY->setChecked(true);
    break;
  case MantidQwtIMDWorkspaceData::PlotDistance:
    ui.radPlotDistance->setChecked(true);
    break;
  default:
    ui.radPlotAuto->setChecked(true);
    break;
  }
}


//------------------------------------------------------------------------------
/** Get the normalization method to use
 * @return choice of normalization */
Mantid::API::MDNormalization LinePlotOptions::getNormalization() const
{
  return m_normalize;
}


/** Set the normalization method to use
 *
 * @param method :: choice of normalization
 */
void LinePlotOptions::setNormalization(Mantid::API::MDNormalization method)
{
  m_normalize = method;
  // Update gui
  switch(m_normalize)
  {
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
void LinePlotOptions::radPlot_changed()
{
  if (ui.radPlotDistance->isChecked())
    m_plotAxis = MantidQwtIMDWorkspaceData::PlotDistance;
  else if (ui.radPlotX->isChecked())
    m_plotAxis = MantidQwtIMDWorkspaceData::PlotX;
  else if (ui.radPlotY->isChecked())
    m_plotAxis = MantidQwtIMDWorkspaceData::PlotY;
  else
    m_plotAxis = MantidQwtIMDWorkspaceData::PlotAuto;
  // Send out a signal
  emit changedPlotAxis();
}


//------------------------------------------------------------------------------
/** Slot called when any of the normalization choice radio buttons are clicked
 */
void LinePlotOptions::radNormalization_changed()
{
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

