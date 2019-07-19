// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Plotting/Qwt/ContourPreviewPlot.h"
#include "MantidQtWidgets/Plotting/Qwt/MantidColorMap.h"
#include "MantidQtWidgets/Plotting/Qwt/QwtRasterDataMD.h"
#include "MantidQtWidgets/Plotting/Qwt/SignalRange.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidKernel/ReadLock.h"

#include <boost/pointer_cast.hpp>
#include <boost/shared_ptr.hpp>
#include <qwt_color_map.h>
#include <qwt_double_rect.h>

#include <cmath>
#include <sstream>

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace MantidQt::API;

namespace {
Mantid::Kernel::Logger g_log("ContourPreviewPlot");

MatrixWorkspace_sptr
convertToMatrixWorkspace(boost::shared_ptr<Workspace> const workspace) {
  return boost::dynamic_pointer_cast<MatrixWorkspace>(workspace);
}

template <typename T>
auto getValueFromSettings(QSettings const &settings, QString const &key,
                          T const &defaultValue) {
  return settings.value(key, defaultValue);
}

bool transparentZeros(QSettings const &settings) {
  return getValueFromSettings(settings, "TransparentZeros", 1).toInt();
}

} // namespace

namespace MantidQt {
namespace MantidWidgets {

ContourPreviewPlot::ContourPreviewPlot(QWidget *parent)
    : QWidget(parent), WorkspaceObserver(),
      m_mdSettings(boost::make_shared<MdSettings>()), m_workspace(),
      m_dimensions() {
  this->observePreDelete(true);
  m_spectrogram = std::make_unique<QwtPlotSpectrogram>();
  m_data = std::make_unique<QwtRasterDataMD>();
  m_normalization = NoNormalization;
  m_uiForm.setupUi(this);

  this->setupPlot();
  this->loadSettings();
  this->updateDisplay();
}

ContourPreviewPlot::~ContourPreviewPlot() {
  this->observePreDelete(false); // Disconnect notifications
  saveSettings();
  m_data.reset();
  m_spectrogram.reset();
}

MatrixWorkspace_sptr ContourPreviewPlot::getActiveWorkspace() const {
  return m_workspace;
}

/**
 * Initialize objects after loading the workspace
 */
void ContourPreviewPlot::setWorkspace(MatrixWorkspace_sptr const workspace) {
  m_workspace = workspace;
  this->checkRangeLimits();
  m_data->setWorkspace(workspace);

  // Sets how the signal is normalized
  m_normalization = workspace->displayNormalization();
  m_data->setNormalization(m_normalization);

  this->setVectorDimensions();
  this->findFullRange();

  m_uiForm.plot2D->setWorkspace(workspace);
  m_spectrogram->setColorMap(
      MantidColorMap("", MantidColorMap::ScaleType::Linear));

  this->updateDisplay();
}

SafeQwtPlot *ContourPreviewPlot::getPlot2D() { return m_uiForm.plot2D; }

/**
 * Set the plot to be visible or hidden
 * @param visible :: false to hide the plot
 */
void ContourPreviewPlot::setPlotVisible(bool const &visible) {
  m_uiForm.plot2D->setVisible(visible);
}

/**
 * Checks if the plot is currently visible
 * @returns true if the plot is visible
 */
bool ContourPreviewPlot::isPlotVisible() const {
  return m_uiForm.plot2D->isVisible();
}

/**
 * Sets the label for the x axis
 * @param label :: the label given to the axis
 */
void ContourPreviewPlot::setXAxisLabel(QString const &label) {
  m_uiForm.lbXAxis->setText(label);
}

/**
 * Sets the label for the y axis
 * @param label :: the label given to the axis
 */
void ContourPreviewPlot::setYAxisLabel(QString const &label) {
  m_uiForm.lbYAxis->setText(label);
}

/**
 * Set whether to display 0 signal as "transparent" colour.
 * @param transparent :: true if you want zeros to be transparent.
 */
void ContourPreviewPlot::handleSetTransparentZeros(bool const &transparent) {
  m_data->setZerosAsNan(transparent);
  this->updateDisplay();
}

/**
 * Remove the displayed data when the associated workspace is deleted
 */
void ContourPreviewPlot::preDeleteHandle(
    std::string const &workspaceName,
    boost::shared_ptr<Workspace> const workspace) {
  UNUSED_ARG(workspaceName);
  auto const deletedWorkspace = convertToMatrixWorkspace(workspace);
  if (deletedWorkspace && deletedWorkspace == m_workspace)
    this->clearPlot();
}

/**
 * Clear the plot
 */
void ContourPreviewPlot::clearPlot() { m_uiForm.plot2D->clear(); }

/**
 * Setup the Plot. Attach the spectrogram to the plot
 */
void ContourPreviewPlot::setupPlot() {
  m_spectrogram->attach(m_uiForm.plot2D);
  m_spectrogram->setColorMap(
      MantidColorMap("", MantidColorMap::ScaleType::Linear));
  m_uiForm.plot2D->autoRefresh();
}

/**
 * Load QSettings from .ini-type files
 */
void ContourPreviewPlot::loadSettings() {
  QSettings settings;
  settings.beginGroup("Mantid/ContourPreviewPlot");

  this->handleSetTransparentZeros(transparentZeros(settings));

  settings.endGroup();
}

void ContourPreviewPlot::saveSettings() {
  QSettings settings;
  settings.beginGroup("Mantid/ContourPreviewPlot");
  settings.setValue("TransparentZeros", (m_data->isZerosAsNan() ? 1 : 0));
}

/**
 * Updates the contour plot
 */
void ContourPreviewPlot::updateDisplay() {
  if (m_workspace) {
    std::vector<Mantid::coord_t> slicePoint{0, 0};
    constexpr std::size_t dimX(0);
    constexpr std::size_t dimY(1);
    IMDDimension_const_sptr X = m_dimensions[dimX];
    IMDDimension_const_sptr Y = m_dimensions[dimY];
    m_data->setSliceParams(dimX, dimY, X, Y, slicePoint);

    double const left{X->getMinimum()};
    double const top{Y->getMinimum()};
    double const width{X->getMaximum() - X->getMinimum()};
    double const height{Y->getMaximum() - Y->getMinimum()};
    QwtDoubleRect const bounds{left, top, width, height};
    m_data->setBoundingRect(bounds.normalized());

    m_spectrogram->setColorMap(
        MantidColorMap("", MantidColorMap::ScaleType::Linear));
    m_spectrogram->setData(*m_data);
    m_spectrogram->itemChanged();
    m_uiForm.plot2D->replot();
  }
}

/**
 * Verify the limits of the stored workspace
 */
void ContourPreviewPlot::checkRangeLimits() const {
  std::ostringstream message;
  for (auto i = 0u; i < m_workspace->getNumDims(); ++i)
    checkForInfiniteLimits(dimensionRange(i), i, message);

  if (!message.str().empty()) {
    message << "Bad ranges could cause memory allocation errors. Please fix "
               "the workspace.";
    throw std::out_of_range(message.str());
  }
}

/**
 * Produces an error message if either of the limits are infinite
 */
void ContourPreviewPlot::checkForInfiniteLimits(
    DimensionRange const &range, std::size_t const &index,
    std::ostringstream &message) const {
  if (!std::isfinite(range.first) || !std::isfinite(range.second))
    message << "Dimension " << m_workspace->getDimension(index)->getName()
            << " has a bad range: (" << range.first << ", " << range.second
            << ")\n";
}

std::tuple<double, double>
ContourPreviewPlot::getAxisRange(AxisID axisID) const {
  switch (axisID) {
  case AxisID::XBottom:
    return std::make_tuple(dimensionMinimum(0), dimensionMaximum(0));
  case AxisID::YLeft:
    return std::make_tuple(dimensionMinimum(1), dimensionMaximum(1));
  }
  throw std::runtime_error(
      "Incorrect AxisID provided. Axis types are XBottom and YLeft");
}

DimensionRange
ContourPreviewPlot::dimensionRange(std::size_t const &index) const {
  DimensionRange range =
      std::make_pair(dimensionMinimum(index), dimensionMaximum(index));
  if (range.second < range.first)
    range.swap(range);
  return range;
}

Mantid::coord_t
ContourPreviewPlot::dimensionMinimum(std::size_t const &index) const {
  return m_workspace->getDimension(index)->getMinimum();
}

Mantid::coord_t
ContourPreviewPlot::dimensionMaximum(std::size_t const &index) const {
  return m_workspace->getDimension(index)->getMaximum();
}

/**
 * Finds the full range of values in the workspace
 */
void ContourPreviewPlot::findFullRange() {
  if (m_workspace) {
    auto const workspace = m_workspace;
    Mantid::Kernel::ReadLock lock(*workspace);

    m_colourRangeFull =
        API::SignalRange(*workspace, m_normalization).interval();
    double minimum = m_colourRangeFull.minValue();

    if (minimum <= 0) {
      double const maximum = m_colourRangeFull.maxValue();
      minimum = pow(10., log10(maximum) - 10.);
      m_colourRangeFull = QwtDoubleInterval(minimum, maximum);
    }
  }
}

/**
 * Update m_dimensions using the loaded workspace
 */
void ContourPreviewPlot::setVectorDimensions() {
  if (m_workspace) {
    m_dimensions.clear();

    for (auto i = 0u; i < m_workspace->getNumDims(); ++i) {
      MDHistoDimension_sptr dimension(std::make_unique<MDHistoDimension>(
          m_workspace->getDimension(i).get()));
      m_dimensions.push_back(dimension);
    }
  }
}

} // namespace MantidWidgets
} // namespace MantidQt
