#include "MantidQtMantidWidgets/MWView.h"
// includes for workspace handling
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
// includes for interface development
#include "MantidQtAPI/QwtRasterDataMD.h"
#include "MantidQtAPI/MantidColorMap.h"
#include <qwt_color_map.h>

namespace {
Mantid::Kernel::Logger g_log("MWView");
}

namespace MantidQt {
namespace MantidWidgets {

//               ++++++++++++++++++++++++++++++++
//               ++++++++ Public members ++++++++
//               ++++++++++++++++++++++++++++++++

MWView::MWView(QWidget *parent)
  : QWidget(parent), m_mdSettings{nullptr}, m_workspace{nullptr} {
  m_spect = new QwtPlotSpectrogram();
  m_data = new MantidQt::API::QwtRasterDataMD();
  this->initLayout();
  this->loadSettings();
  this->updateDisplay();
}

MWView::~MWView() {
  saveSettings();
  delete m_data;
  delete m_spect;
}

/** Load a color map from a file
 *
 * @param filename :: file to open; empty to ask via a dialog box.
 */
void MWView::loadColorMap(QString filename) {
  QString fileselection;
  if (filename.isEmpty()) {
    fileselection = MantidColorMap::loadMapDialog(m_currentColorMapFile, this);
    if (fileselection.isEmpty())
      return;
  } else
    fileselection = filename;
  m_currentColorMapFile = fileselection;
  m_uiForm.colorBar->getColorMap().loadMap(fileselection);
  m_spect->setColorMap(m_uiForm.colorBar->getColorMap());
  m_uiForm.colorBar->updateColorMap();
  this->updateDisplay();
}

void MWView::setWorkspace(Mantid::API::MatrixWorkspace_sptr ws) {
  m_workspace = ws;
  this->checkRangeLimits();
  m_data->setWorkspace(ws);
  m_uiForm.plot2D->setWorkspace(ws);
  emit workspaceChanged();
}


//               ++++++++++++++++++++++++++++++++
//               ++++++++ Public slots   ++++++++
//               ++++++++++++++++++++++++++++++++

/// Slot called when the ColorBarWidget changes the range of colors
void MWView::colorRangeChangedSlot() {
  m_spect->setColorMap(m_uiForm.colorBar->getColorMap());
  this->updateDisplay();
}

void MWView::loadColorMapSlot() { this->loadColorMap(QString()); }

/** Set whether to display 0 signal as "transparent" color.
 *
 * @param transparent :: true if you want zeros to be transparent.
 */
void MWView::setTransparentZerosSlot(bool transparent) {
  m_data->setZerosAsNan(transparent);
  this->updateDisplay();
}

//               ++++++++++++++++++++++++++++++++
//               ++++++++ Private members +++++++
//               ++++++++++++++++++++++++++++++++

/// Initialize the ui form and connect SIGNALS to SLOTS
void MWView::initLayout() {
  m_uiForm.setupUi(this);
  m_spect->attach(m_uiForm.plot2D); // attach the spectrogram to the plot
  /// initialize the color on the bar and the data
  m_uiForm.colorBar->setViewRange(1, 10);
  QObject::connect(m_uiForm.colorBar,
                   SIGNAL(changedColorRange(double, double, bool)), this,
                   SLOT(colorRangeChangedSlot()));
  m_spect->setColorMap(m_uiForm.colorBar->getColorMap());
  m_uiForm.plot2D->autoRefresh();
  // Signal/Slot updating the color map
  QObject::connect(m_uiForm.colorBar, SIGNAL(colorBarDoubleClicked()), this,
                   SLOT(loadColorMapSlot()));
  // initZoomer();  // TO BE IMPLEMENTED
}

/** Load QSettings from .ini-type files */
void MWView::loadSettings() {
  QSettings settings;
  settings.beginGroup("Mantid/MWView");
  // Maintain backwards compatibility with use of LogColorScale
  int scaleType = settings.value("ColorScale", -1).toInt();
  if (scaleType == -1) {
    scaleType = settings.value("LogColorScale", 0).toInt();
  }
  double nth_power = settings.value("PowerScaleExponent", 2.0).toDouble();
  // Load Colormap. If the file is invalid the default stored colour map is
  // used.
  // If the user selected a unified color map for the SliceViewer and the VSI,
  // then this is loaded.
  if (m_mdSettings != NULL && m_mdSettings->getUsageGeneralMdColorMap()) {
    m_currentColorMapFile = m_mdSettings->getGeneralMdColorMapFile();
  } else {
    m_currentColorMapFile = settings.value("ColormapFile", "").toString();
  }
  // Set values from settings
  if (!m_currentColorMapFile.isEmpty())
    loadColorMap(m_currentColorMapFile);
  m_uiForm.colorBar->setScale(scaleType);
  m_uiForm.colorBar->setExponent(nth_power);
  bool transparentZeros = settings.value("TransparentZeros", 1).toInt();
  this->setTransparentZerosSlot(transparentZeros);
  settings.endGroup();
}

void MWView::saveSettings() {
  QSettings settings;
  settings.beginGroup("Mantid/MWView");
  settings.setValue("ColormapFile", m_currentColorMapFile);
  settings.setValue("ColorScale", m_uiForm.colorBar->getScale());
  settings.setValue("PowerScaleExponent", m_uiForm.colorBar->getExponent());
  settings.setValue("TransparentZeros", (m_data->isZerosAsNan() ? 1 : 0));
}

void MWView::updateDisplay() {
  if (!m_workspace)
    return;
  m_data->setRange(m_uiForm.colorBar->getViewRange());
  m_spect->setData(*m_data);
  m_spect->itemChanged();
  m_uiForm.plot2D->replot();
}

// Verify workspace limits
void MWView::checkRangeLimits(){
  std::ostringstream mess;
  for (size_t d = 0; d < m_workspace->getNumDims(); d++) {
    Mantid::coord_t min = m_workspace->getDimension(d)->getMinimum();
    Mantid::coord_t max = m_workspace->getDimension(d)->getMaximum();
    if (max < min) {
      Mantid::coord_t tmp = max;
      max = min;
      min = tmp;
    }
    if (boost::math::isnan(min) || boost::math::isinf(min) ||
        boost::math::isnan(max) || boost::math::isinf(max)) {
      mess << "Dimension " << m_workspace->getDimension(d)->getName()
           << " has a bad range: (";
      mess << min << ", " << max << ")" << std::endl;
    }
  }
  if (!mess.str().empty()) {
    mess << "Bad ranges could cause memory allocation errors. Please fix the "
            "workspace.";
    mess << std::endl
         << "You can continue using Mantid.";
    throw std::out_of_range(mess.str());
  }
}

} // namespace MantidQt
} // namespace MantidWidgets
