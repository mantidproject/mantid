// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Plotting/Qwt/MWView.h"
// includes for workspace handling
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidKernel/ReadLock.h"
#include "MantidQtWidgets/Plotting/Qwt/SignalRange.h"
#include <boost/pointer_cast.hpp>
#include <boost/shared_ptr.hpp>
// includes for interface development
#include "MantidQtWidgets/Plotting/Qwt/MantidColorMap.h"
#include "MantidQtWidgets/Plotting/Qwt/QwtRasterDataMD.h"
#include <QSettings>
#include <qwt_color_map.h>
#include <qwt_double_rect.h>
// system includes
#include <cmath>

namespace {
Mantid::Kernel::Logger g_log("MWView");
}

namespace MantidQt {
namespace MantidWidgets {

//               ++++++++++++++++++++++++++++++++
//               ++++++++ Public members ++++++++
//               ++++++++++++++++++++++++++++++++

MWView::MWView(QWidget *parent)
    : QWidget(parent), MantidQt::API::WorkspaceObserver(),
      m_mdSettings(boost::make_shared<MantidQt::API::MdSettings>()),
      m_workspace(),
      m_wellcomeWorkspace(), m_wellcomeName{"__MWViewWellcomeWorkspace"},
      m_dimensions() {
  // Watch for the deletion of the associated workspace
  this->observePreDelete(true);
  m_spect = new QwtPlotSpectrogram();
  m_data = new MantidQt::API::QwtRasterDataMD();
  m_normalization = Mantid::API::NoNormalization;
  this->initLayout();
  this->loadSettings();
  this->updateDisplay();
  this->showWellcomeWorkspace();
}

MWView::~MWView() {
  this->observePreDelete(false); // Disconnect notifications
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
    fileselection = MantidColorMap::chooseColorMap(m_currentColorMapFile, this);
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

/**
 * @brief Initialize objects after loading the workspace, and observe.
 */
void MWView::setWorkspace(Mantid::API::MatrixWorkspace_sptr ws) {
  m_workspace = ws;
  this->checkRangeLimits();
  m_data->setWorkspace(ws);
  m_normalization = ws->displayNormalization();
  m_data->setNormalization(m_normalization);
  this->setVectorDimensions();
  this->findRangeFull(); // minimum and maximum intensities in ws
  m_uiForm.colorBar->setViewRange(m_colorRangeFull);
  m_uiForm.colorBar->updateColorMap();
  m_uiForm.plot2D->setWorkspace(ws);
  m_spect->setColorMap(m_uiForm.colorBar->getColorMap());
}

void MWView::updateDisplay() {
  if (!m_workspace)
    return;
  m_data->setRange(m_uiForm.colorBar->getViewRange());
  std::vector<Mantid::coord_t> slicePoint{0, 0};
  constexpr size_t dimX(0);
  constexpr size_t dimY(1);
  Mantid::Geometry::IMDDimension_const_sptr X = m_dimensions[dimX];
  Mantid::Geometry::IMDDimension_const_sptr Y = m_dimensions[dimY];
  m_data->setSliceParams(dimX, dimY, X, Y, slicePoint);
  double left{X->getMinimum()};
  double top{Y->getMinimum()};
  double width{X->getMaximum() - X->getMinimum()};
  double height{Y->getMaximum() - Y->getMinimum()};
  QwtDoubleRect bounds{left, top, width, height};
  m_data->setBoundingRect(bounds.normalized());
  m_spect->setColorMap(m_uiForm.colorBar->getColorMap());
  m_spect->setData(*m_data);
  m_spect->itemChanged();
  m_uiForm.plot2D->replot();
}

SafeQwtPlot *MWView::getPlot2D() { return m_uiForm.plot2D; }

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

/*                 ***************************
 ***  Protected Members  ***
 ***************************/

/*
 * @brief Clean shown data when associated workspace is deleted
 */
void MWView::preDeleteHandle(
    const std::string &workspaceName,
    const boost::shared_ptr<Mantid::API::Workspace> workspace) {
  UNUSED_ARG(workspaceName);
  Mantid::API::MatrixWorkspace_sptr ws =
      boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(workspace);
  if (ws && ws == m_workspace) {
    this->showWellcomeWorkspace();
  }
}

//               ++++++++++++++++++++++++++++++++
//               ++++++++ Private members +++++++
//               ++++++++++++++++++++++++++++++++

/// Initialize the ui form and connect SIGNALS to SLOTS
void MWView::initLayout() {
  m_uiForm.setupUi(this);
  QObject::connect(m_uiForm.colorBar,
                   SIGNAL(changedColorRange(double, double, bool)), this,
                   SLOT(colorRangeChangedSlot()));
  QObject::connect(m_uiForm.colorBar, SIGNAL(colorBarDoubleClicked()), this,
                   SLOT(loadColorMapSlot()));
  /// initialize the color on the bar and the data
  m_uiForm.colorBar->setViewRange(1, 10);
  m_spect->attach(m_uiForm.plot2D); // attach the spectrogram to the plot
  m_spect->setColorMap(m_uiForm.colorBar->getColorMap());
  m_uiForm.plot2D->autoRefresh();
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
  if (m_mdSettings != nullptr && m_mdSettings->getUsageGeneralMdColorMap()) {
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

// Verify workspace limits
void MWView::checkRangeLimits() {
  std::ostringstream mess;
  for (size_t d = 0; d < m_workspace->getNumDims(); d++) {
    Mantid::coord_t min = m_workspace->getDimension(d)->getMinimum();
    Mantid::coord_t max = m_workspace->getDimension(d)->getMaximum();
    if (max < min) {
      Mantid::coord_t tmp = max;
      max = min;
      min = tmp;
    }
    if (!std::isfinite(min) || !std::isfinite(max)) {
      mess << "Dimension " << m_workspace->getDimension(d)->getName()
           << " has a bad range: (";
      mess << min << ", " << max << ")\n";
    }
  }
  if (!mess.str().empty()) {
    mess << "Bad ranges could cause memory allocation errors. Please fix the "
            "workspace.\nYou can continue using Mantid.";
    throw std::out_of_range(mess.str());
  }
}

/// Find the full range of values in the workspace
void MWView::findRangeFull() {
  if (!m_workspace)
    return;
  Mantid::API::MatrixWorkspace_sptr workspace_used = m_workspace;

  // Acquire a scoped read-only lock on the workspace, preventing it from being
  // written
  // while we iterate through.
  Mantid::Kernel::ReadLock lock(*workspace_used);

  // Iterate through the entire workspace
  m_colorRangeFull =
      API::SignalRange(*workspace_used, m_normalization).interval();
  double minR = m_colorRangeFull.minValue();
  if (minR <= 0 && m_uiForm.colorBar->getScale() == 1) {
    double maxR = m_colorRangeFull.maxValue();
    minR = pow(10., log10(maxR) - 10.);
    m_colorRangeFull = QwtDoubleInterval(minR, maxR);
  }
}

/// Update m_dimensions with the loaded workspace
void MWView::setVectorDimensions() {
  if (!m_workspace) {
    return;
  }
  m_dimensions.clear();
  for (size_t d = 0; d < m_workspace->getNumDims(); d++) {
    Mantid::Geometry::MDHistoDimension_sptr dimension(
        new Mantid::Geometry::MDHistoDimension(
            m_workspace->getDimension(d).get()));
    m_dimensions.push_back(dimension);
  }
}

/*
 * @brief Generates a default workspace to be shown, if no workspace
 * is selected
 */
void MWView::spawnWellcomeWorkspace() {
  if (Mantid::API::AnalysisDataService::Instance().doesExist(m_wellcomeName)) {
    m_wellcomeWorkspace =
        Mantid::API::AnalysisDataService::Instance()
            .retrieveWS<Mantid::API::MatrixWorkspace>(m_wellcomeName);
  } else {
    int numberSpectra = 100;
    double intensity = 10.0;
    auto dataX = std::vector<double>();
    auto dataY = std::vector<double>();
    for (int i = 0; i < numberSpectra; i++) {
      for (int j = 0; j < numberSpectra; j++) {
        dataX.push_back(j * 1.);
        dataY.push_back(intensity * (i * i + j * j) /
                        (2 * numberSpectra * numberSpectra));
      }
    }
    auto createWsAlg =
        Mantid::API::AlgorithmManager::Instance().create("CreateWorkspace");
    createWsAlg->initialize();
    createWsAlg->setChild(true);
    createWsAlg->setLogging(false);
    createWsAlg->setProperty("OutputWorkspace", m_wellcomeName);
    createWsAlg->setProperty("NSpec", numberSpectra);
    createWsAlg->setProperty("DataX", dataX);
    createWsAlg->setProperty("DataY", dataY);
    createWsAlg->execute();
    m_wellcomeWorkspace = createWsAlg->getProperty("OutputWorkspace");
    Mantid::API::AnalysisDataService::Instance().add(m_wellcomeName,
                                                     m_wellcomeWorkspace);
  }
}

/**
 * @brief Replace or start showing the wellcoming workspace
 */
void MWView::showWellcomeWorkspace() {
  this->spawnWellcomeWorkspace();
  this->setWorkspace(m_wellcomeWorkspace);
  this->updateDisplay();
  m_uiForm.colorBar->setScale(0); // reset to linear color scale
}

} // namespace MantidWidgets
} // namespace MantidQt
