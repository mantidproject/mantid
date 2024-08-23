// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/InstrumentView/InstrumentActor.h"
#include "MantidQtWidgets/InstrumentView/InstrumentRendererClassic.h"
#include "MantidQtWidgets/InstrumentView/InstrumentRendererMultiList.h"
#include "MantidQtWidgets/InstrumentView/OpenGLError.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IMaskWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidQtWidgets/Common/MessageHandler.h"
#include "MantidTypes/SpectrumDefinition.h"

#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/InstrumentVisitor.h"

#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/ReadLock.h"
#include "MantidKernel/V3D.h"

#include <boost/algorithm/string.hpp>
#include <cmath>

#include <QMessageBox>
#include <QMetaObject>
#include <QSettings>
#include <QThread>

#include <utility>

using namespace Mantid::Kernel::Exception;
using namespace Mantid::Geometry;
using namespace Mantid::API;
using namespace Mantid;

namespace MantidQt::MantidWidgets {
namespace {
bool isPhysicalView() {
  std::string view = Mantid::Kernel::ConfigService::Instance().getString("instrument.view.geometry");

  return boost::iequals("Default", view) || boost::iequals("Physical", view);
}

} // namespace

/**
 * Constructor. Creates a tree of GLActors. Each actor is responsible for
 * displaying insrument components in 3D.
 * Some of the components have "pick ID" assigned to them. Pick IDs can be
 * uniquely converted to a RGB colour value
 * which in turn can be used for picking the component from the screen.
 * @param wsName :: Workspace name
 * @param autoscaling :: True to start with autoscaling option on. If on the min
 * and max of
 *   the colormap scale are defined by the min and max of the data.
 * @param scaleMin :: Minimum value of the colormap scale. Used to assign
 * detector colours. Ignored if autoscaling == true.
 * @param scaleMax :: Maximum value of the colormap scale. Used to assign
 * detector colours. Ignored if autoscaling == true.
 */
InstrumentActor::InstrumentActor(const std::string &wsName, MantidWidgets::IMessageHandler &messageHandler,
                                 bool autoscaling, double scaleMin, double scaleMax, QString settingsGroup)
    : InstrumentActor(AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName), messageHandler, autoscaling,
                      scaleMin, scaleMax, settingsGroup) {}

InstrumentActor::InstrumentActor(MatrixWorkspace_sptr workspace, MantidWidgets::IMessageHandler &messageHandler,
                                 bool autoscaling, double scaleMin, double scaleMax, QString settingsGroup)
    : m_workspace(workspace), m_settingsGroup(std::move(settingsGroup)), m_ragged(true), m_autoscaling(autoscaling),
      m_defaultPos(), m_initialized(false), m_isPhysicalInstrument(false), m_messageHandler(messageHandler) {

  loadSettings();

  m_scaleMin = scaleMin;
  m_scaleMax = scaleMax;

  m_isCompVisible.assign(componentInfo().size(), true);

  resetInstrumentRenderer();
  m_renderer->changeScaleType(m_scaleType);
}

InstrumentActor::~InstrumentActor() {}

void InstrumentActor::initialize(bool resetGeometry, bool setDefaultView) {
  auto sharedWorkspace = m_workspace;

  if (!sharedWorkspace)
    throw std::logic_error("InstrumentActor passed a workspace that isn't a MatrixWorkspace");
  setupPhysicalInstrumentIfExists();

  m_hasGrid = false;
  m_numGridLayers = 0;
  for (size_t i = 0; i < componentInfo().size(); ++i) {
    if (!componentInfo().isDetector(i))
      m_components.emplace_back(i);
    else if (detectorInfo().isMonitor(i))
      m_monitors.emplace_back(i);
    if (componentInfo().componentType(i) == Mantid::Beamline::ComponentType::Grid) {
      m_hasGrid = true;
      m_numGridLayers = componentInfo().children(i).size();
    }
  }

  m_isCompVisible.assign(componentInfo().size(), true);

  resetInstrumentRenderer();
  m_renderer->changeScaleType(m_scaleType);

  // set up the color map
  if (!m_currentCMap.first.isEmpty()) {
    loadColorMap(m_currentCMap, false);
  }

  // set up data ranges and colours
  setUpWorkspace(sharedWorkspace, m_scaleMin, m_scaleMax);

  // If the instrument is empty, maybe only having the sample and source
  if (detectorInfo().size() == 0) {
    m_messageHandler.giveUserWarning("This instrument appears to contain no detectors", "Mantid - Warning");
  }

  // enable drawing now that everything is ready
  m_initialized = true;

  resetColors();

  // send signal back to the InstrumentWidget to finish setting up
  emit initWidget(resetGeometry, setDefaultView);
  emit refreshView();
}

void InstrumentActor::resetInstrumentRenderer() {
  const auto mesaSetting =
      Mantid::Kernel::ConfigService::Instance().getString("MantidOptions.InstrumentView.MesaBugWorkaround");
  if (mesaSetting == "On") {
    m_renderer.reset(new InstrumentRendererMultiList(*this));
    return;
  }
  m_renderer.reset(new InstrumentRendererClassic(*this));
}

void InstrumentActor::cancel() {
  blockSignals(true);

  // cancel any running mantid algorithms to help free the thread
  auto alg = AlgorithmManager::Instance().getAlgorithm(m_algID);
  if (alg && alg->isRunning()) {
    alg->cancel();
  }
}

/**
 * Set up the workspace: calculate the value ranges, set the colours.
 * @param sharedWorkspace :: A shared pointer to the workspace.
 * @param scaleMin :: Minimum limit on the color map axis. If autoscale this
 * value is ignored.
 * @param scaleMax :: Maximum limit on the color map axis. If autoscale this
 * value is ignored.
 */
void InstrumentActor::setUpWorkspace(const std::shared_ptr<const Mantid::API::MatrixWorkspace> &sharedWorkspace,
                                     double scaleMin, double scaleMax) {
  m_WkspBinMinValue = DBL_MAX;
  m_WkspBinMaxValue = -DBL_MAX;
  const auto &spectrumInfo = sharedWorkspace->spectrumInfo();
  m_detIndex2WsIndex.resize(componentInfo().size(), INVALID_INDEX);
  // PARALLEL_FOR_NO_WSP_CHECK()
  for (size_t wi = 0; wi < spectrumInfo.size(); ++wi) {
    const auto &values = sharedWorkspace->x(wi);
    double xtest = values.front();
    if (!std::isinf(xtest)) {
      if (xtest < m_WkspBinMinValue)
        m_WkspBinMinValue = xtest;
      else if (xtest > m_WkspBinMaxValue)
        m_WkspBinMaxValue = xtest;
    }

    xtest = values.back();
    if (!std::isinf(xtest)) {
      if (xtest < m_WkspBinMinValue)
        m_WkspBinMinValue = xtest;
      else if (xtest > m_WkspBinMaxValue)
        m_WkspBinMaxValue = xtest;
    }

    const auto &specDef = spectrumInfo.spectrumDefinition(wi);
    for (const auto &info : specDef)
      m_detIndex2WsIndex[info.first] = wi;
  }

  // set some values as the variables will be used
  m_DataPositiveMinValue = DBL_MAX;
  m_DataMinValue = -DBL_MAX;
  m_DataMaxValue = DBL_MAX;

  if (!m_autoscaling)
    setDataMinMaxRange(scaleMin, scaleMax);

  setDataIntegrationRange(m_WkspBinMinValue, m_WkspBinMaxValue);
  resetColors();

  // set the ragged flag using a workspace validator
  m_ragged = !sharedWorkspace->isCommonBins();
}

void InstrumentActor::setupPhysicalInstrumentIfExists() {
  if (!isPhysicalView())
    return;

  auto sharedWorkspace = getWorkspace();
  Mantid::Kernel::ReadLock _lock(*sharedWorkspace);

  auto instr = sharedWorkspace->getInstrument()->getPhysicalInstrument();
  if (instr) {
    auto infos = InstrumentVisitor::makeWrappers(*instr);
    m_physicalComponentInfo = std::move(infos.first);
    m_physicalDetectorInfo = std::move(infos.second);
    m_isPhysicalInstrument = true;
  }
}

void InstrumentActor::setComponentVisible(size_t componentIndex) {
  setAllComponentsVisibility(false);
  const auto &compInfo = componentInfo();
  auto children = compInfo.componentsInSubtree(componentIndex);
  m_isCompVisible[componentIndex] = true;
  for (auto child : children)
    m_isCompVisible[child] = true;

  resetColors();
}

void InstrumentActor::setAllComponentsVisibility(bool on) {
  std::fill(m_isCompVisible.begin(), m_isCompVisible.end(), on);
}

bool InstrumentActor::hasChildVisible() const {
  return std::any_of(m_isCompVisible.begin(), m_isCompVisible.end(), [](bool visible) { return visible; });
}

/** Returns the workspace relating to this instrument view.
 *  !!!! DON'T USE THIS TO GET HOLD OF THE INSTRUMENT !!!!
 *  !!!! USE InstrumentActor::getInstrument() BELOW !!!!
 */
MatrixWorkspace_const_sptr InstrumentActor::getWorkspace() const { return m_workspace; }

void InstrumentActor::getBoundingBox(Mantid::Kernel::V3D &minBound, Mantid::Kernel::V3D &maxBound,
                                     const bool excludeMonitors) const {
  const auto &compInfo = componentInfo();
  auto bb = compInfo.boundingBox(compInfo.root(), nullptr, excludeMonitors);
  minBound = bb.minPoint();
  maxBound = bb.maxPoint();
}

/** Returns the mask workspace relating to this instrument view as a
 * MatrixWorkspace
 */
MatrixWorkspace_sptr InstrumentActor::getMaskMatrixWorkspace() const {
  if (!m_maskWorkspace) {
    initMaskHelper();
  }
  return m_maskWorkspace;
}

/** set the mask workspace
 */
void InstrumentActor::setMaskMatrixWorkspace(MatrixWorkspace_sptr wsMask) const { m_maskWorkspace = std::move(wsMask); }

void InstrumentActor::invertMaskWorkspace() const {
  auto invertAlg = AlgorithmManager::Instance().create("BinaryOperateMasks", -1);
  invertAlg->setChild(true);
  invertAlg->setAlwaysStoreInADS(false);
  auto maskWs = getMaskMatrixWorkspace();
  invertAlg->setProperty("InputWorkspace1", maskWs);
  invertAlg->getPointerToProperty("OutputWorkspace")->createTemporaryValue();
  invertAlg->setProperty("OutputWorkspace", maskWs);
  invertAlg->setPropertyValue("OperationType", "NOT");
  invertAlg->execute();

  m_maskWorkspace = maskWs; // This is a no-op but makes it clear we intended to overwrite the member
}

/**
 * Returns the mask workspace relating to this instrument view as a
 * IMaskWorkspace.
 * Guarantees to return a valid pointer
 */
IMaskWorkspace_sptr InstrumentActor::getMaskWorkspace() const {
  if (!m_maskWorkspace) {
    initMaskHelper();
  }
  return std::dynamic_pointer_cast<IMaskWorkspace>(m_maskWorkspace);
}

/**
 * Returns the mask workspace relating to this instrument view as a
 * IMaskWorkspace
 * if it exists or empty pointer if it doesn't.
 */
IMaskWorkspace_sptr InstrumentActor::getMaskWorkspaceIfExists() const {
  if (!m_maskWorkspace)
    return IMaskWorkspace_sptr();
  return std::dynamic_pointer_cast<IMaskWorkspace>(m_maskWorkspace);
}

/**
 * Apply mask stored in the helper mask workspace to the data workspace.
 */
void InstrumentActor::applyMaskWorkspace() {
  if (m_maskWorkspace) {
    // Mask detectors
    try {
      auto alg = AlgorithmManager::Instance().create("MaskDetectors", -1);
      alg->setProperty("Workspace", m_workspace);
      alg->setProperty("MaskedWorkspace", m_maskWorkspace);
      alg->setChild(true);
      alg->setAlwaysStoreInADS(false);
      alg->execute();
      // After the algorithm finishes the InstrumentWindow catches the
      // after-replace notification
      // and updates this instrument actor.
    } catch (std::runtime_error const &) {
      m_messageHandler.giveUserWarning("An error occurred when applying the mask.", "Mantid - Warning");
    }
  }

  // Mask bins
  try {
    m_maskBinsData.mask(m_workspace);
  } catch (std::logic_error &) {
    m_messageHandler.giveUserWarning("An error occurred when applying the mask to bins.", "Mantid - Warning");
  }
  clearMasks();
}

/**
 * Removes the mask workspace.
 */
void InstrumentActor::clearMasks() {
  bool needColorRecalc = false;
  if (m_maskWorkspace) {
    needColorRecalc = getMaskWorkspace()->getNumberMasked() > 0;
  }
  m_maskWorkspace.reset();
  if (!m_maskBinsData.isEmpty()) {
    m_maskBinsData.clear();
    auto workspace = getWorkspace();
    calculateIntegratedSpectra(*workspace);
    needColorRecalc = true;
  }
  if (needColorRecalc) {
    resetColors();
  }
}

Instrument_const_sptr InstrumentActor::getInstrument() const {
  auto sharedWorkspace = getWorkspace();
  Mantid::Kernel::ReadLock _lock(*sharedWorkspace);

  if (isPhysicalView()) {
    // First see if there is a 'physical' instrument available. Use it if there
    // is.
    auto instr = sharedWorkspace->getInstrument()->getPhysicalInstrument();
    if (instr)
      return instr;
  }

  return sharedWorkspace->getInstrument();
}

const ColorMap &InstrumentActor::getColorMap() const { return m_renderer->getColorMap(); }

size_t InstrumentActor::getDetectorByDetID(Mantid::detid_t detID) const {
  const auto &detInfo = detectorInfo();
  return detInfo.indexOf(detID);
}

Mantid::detid_t InstrumentActor::getDetID(size_t pickID) const {
  const auto &detInfo = detectorInfo();
  if (pickID < detInfo.size()) {
    return detInfo.detectorIDs()[pickID];
  }
  return -1;
}

std::vector<Mantid::detid_t> InstrumentActor::getDetIDs(const std::vector<size_t> &dets) const {
  std::vector<Mantid::detid_t> detIDs;
  std::transform(dets.cbegin(), dets.cend(), std::back_inserter(detIDs), [this](auto det) { return getDetID(det); });
  return detIDs;
}

/**
 * Get a component id of a picked component.
 */
Mantid::Geometry::ComponentID InstrumentActor::getComponentID(size_t pickID) const {
  auto compID = Mantid::Geometry::ComponentID();
  const auto &compInfo = componentInfo();
  if (pickID < compInfo.size())
    compID = compInfo.componentID(pickID)->getComponentID();
  return compID;
}

/** Retrieve the workspace index corresponding to a particular detector
 *  @param index The detector index
 *  @returns  The workspace index containing data for this detector
 *  @throws Exception::NotFoundError If the detector is not represented in the
 * workspace
 */
size_t InstrumentActor::getWorkspaceIndex(size_t index) const { return m_detIndex2WsIndex[index]; }

std::vector<size_t> InstrumentActor::getWorkspaceIndices(const std::vector<size_t> &dets) const {
  auto detIDs = getDetIDs(dets);
  return m_workspace->getIndicesFromDetectorIDs(detIDs);
}

/**
 * Set an interval in the data workspace x-vector's units in which the data are
 * to be
 * integrated to calculate the detector colours.
 *
 * @param xmin :: The lower bound.
 * @param xmax :: The upper bound.
 */
void InstrumentActor::setIntegrationRange(const double &xmin, const double &xmax) {
  setDataIntegrationRange(xmin, xmax);
  resetColors();
}

/** Gives the total signal in the spectrum relating to the given detector
 *  @param index The detector index
 *  @return The signal
 */
double InstrumentActor::getIntegratedCounts(size_t index) const {
  auto i = getWorkspaceIndex(index);
  if (i == INVALID_INDEX)
    return InstrumentActor::INVALID_VALUE;
  return m_integratedSignal.at(i);
}

/**
 * Sum counts in detectors for purposes of rough plotting against the units on
 * the x-axis.
 * Checks (approximately) if the workspace is ragged or not and uses the
 * appropriate summation
 * method.
 *
 * @param dets :: A list of detector Indices to sum.
 * @param x :: (output) Time of flight values (or whatever values the x axis
 * has) to plot against.
 * @param y :: (output) The sums of the counts for each bin.
 * @param size :: Size of the output vectors. If not given it
 * will be determined automatically.
 */
void InstrumentActor::sumDetectors(const std::vector<size_t> &dets, std::vector<double> &x, std::vector<double> &y,
                                   size_t size) const {
  // don't bother if no detectors are supplied
  if (dets.empty() || size == 0) {
    x.clear();
    y.clear();
    return;
  }

  Mantid::API::MatrixWorkspace_const_sptr ws = getWorkspace();
  const auto blocksize = ws->blocksize();
  assert(size > 0);
  if (size > blocksize) {
    size = blocksize;
  }

  if (m_ragged) {
    // could be slower than uniform
    sumDetectorsRagged(dets, x, y, size);
  } else {
    // should be faster than ragged
    sumDetectorsUniform(dets, x, y);
  }
}

/**
 * @overload InstrumentActor::sumDetectors(const std::vector<size_t> &dets,
 * std::vector<double> &x, std::vector<double> &y, size_t size = 0)
 */
void InstrumentActor::sumDetectors(const std::vector<size_t> &dets, std::vector<double> &x,
                                   std::vector<double> &y) const {
  sumDetectors(dets, x, y, getWorkspace()->blocksize());
}

/**
 * Sum counts in detectors for purposes of rough plotting against the units on
 * the x-axis.
 * Assumes that all spectra share the x vector.
 *
 * @param dets :: A list of detector Indices to sum.
 * @param x :: (output) Time of flight values (or whatever values the x axis
 * has) to plot against.
 * @param y :: (output) The sums of the counts for each bin.
 */
void InstrumentActor::sumDetectorsUniform(const std::vector<size_t> &dets, std::vector<double> &x,
                                          std::vector<double> &y) const {
  auto firstWorkspaceIndex = [this](const std::vector<size_t> &dets) {
    if (dets.empty())
      return INVALID_INDEX;
    for (auto i : dets) {
      auto const index = getWorkspaceIndex(i);
      if (index != INVALID_INDEX)
        return index;
    }
    return INVALID_INDEX;
  };

  auto const wi = firstWorkspaceIndex(dets);

  if (wi == INVALID_INDEX) {
    x.clear();
    y.clear();
    return;
  }

  // find the bins inside the integration range
  size_t imin, imax;
  getBinMinMaxIndex(wi, imin, imax);

  Mantid::API::MatrixWorkspace_const_sptr ws = getWorkspace();
  const auto &XPoints = ws->points(wi);
  x.assign(XPoints.begin() + imin, XPoints.begin() + imax);
  y.resize(x.size(), 0);
  // sum the spectra
  for (const auto det : dets) {
    const auto index = getWorkspaceIndex(det);
    if (index == INVALID_INDEX)
      continue;
    const auto &Y = ws->y(index);
    std::transform(y.begin(), y.end(), Y.begin() + imin, y.begin(), std::plus<double>());
  }
}

/**
 * Sum counts in detectors for purposes of rough plotting against the units on
 * the x-axis.
 * Assumes that all spectra have different x vectors.
 *
 * @param dets :: A list of detector IDs to sum.
 * @param x :: (output) Time of flight values (or whatever values the x axis
 * has) to plot against.
 * @param y :: (output) The sums of the counts for each bin.
 * @param size :: (input) Size of the output vectors.
 */
void InstrumentActor::sumDetectorsRagged(const std::vector<size_t> &dets, std::vector<double> &x,
                                         std::vector<double> &y, size_t size) const {
  Mantid::API::MatrixWorkspace_const_sptr inputWs = getWorkspace();
  //  create a workspace to hold the data from the selected detectors
  auto detectorWs = Mantid::API::WorkspaceFactory::Instance().create(inputWs, dets.size());

  // x-axis limits
  double xStart = maxBinValue();
  double xEnd = minBinValue();

  size_t nSpec = 0; // number of actual spectra to add
  // fill in the temp workspace with the data from the detectors
  for (const auto det : dets) {
    const auto index = getWorkspaceIndex(det);
    if (index == INVALID_INDEX)
      continue;
    detectorWs->setHistogram(nSpec, inputWs->histogram(index));
    double xmin = detectorWs->x(nSpec).front();
    double xmax = detectorWs->x(nSpec).back();
    if (xmin < xStart)
      xStart = xmin;
    if (xmax > xEnd)
      xEnd = xmax;
    ++nSpec;
  }

  if (nSpec == 0) {
    x.clear();
    y.clear();
    return;
  }

  // limits should exceed the integration range
  if (xStart < minBinValue())
    xStart = minBinValue();

  if (xEnd > maxBinValue())
    xEnd = maxBinValue();

  double dx = (xEnd - xStart) / static_cast<double>(size - 1);
  std::string params = QString("%1,%2,%3").arg(xStart).arg(dx).arg(xEnd).toStdString();

  try {
    // rebin all spectra to the same binning
    auto alg = AlgorithmManager::Instance().create("Rebin", -1);
    alg->setChild(true);
    alg->setAlwaysStoreInADS(false);
    alg->setProperty("InputWorkspace", detectorWs);
    alg->getPointerToProperty("OutputWorkspace")->createTemporaryValue();
    alg->setPropertyValue("Params", params);
    alg->execute();

    MatrixWorkspace_sptr outputWs = alg->getProperty("OutputWorkspace");

    const auto &commonX = outputWs->points(0);
    const auto &firstY = outputWs->y(0);
    x.assign(std::cbegin(commonX), std::cend(commonX));
    y.assign(std::cbegin(firstY), std::cend(firstY));

    // add the spectra
    for (size_t i = 0; i < nSpec; ++i) {
      const auto &specY = outputWs->y(i);
      std::transform(std::cbegin(y), std::cend(y), std::cbegin(specY), std::begin(y), std::plus<double>());
    }
  } catch (std::invalid_argument &) {
    // wrong Params for any reason
    x.resize(size, (xEnd + xStart) / 2);
    y.resize(size, 0.0);
  }
}

/**
 * Recalculate the detector colors based on the integrated values in
 * m_integratedSignal and
 * the masking information in ....
 */
void InstrumentActor::resetColors() {
  if (!m_initialized) {
    return;
  }
  m_renderer->reset();
  emit colorMapChanged();
}

void InstrumentActor::updateColors() { setIntegrationRange(m_BinMinValue, m_BinMaxValue); }

/**
 * @param on :: True or false for on or off.
 */
void InstrumentActor::showGuides(bool on) {
  m_showGuides = on;
  resetColors();
}

GLColor InstrumentActor::getColor(size_t index) const { return m_renderer->getColor(index); }

void InstrumentActor::draw(bool picking) const {
  if (!m_initialized) {
    return;
  }
  m_renderer->renderInstrument(m_isCompVisible, m_showGuides, picking);
}

/**
 * @param fname :: A color map file name.
 * @param reset_colors :: An option to reset the detector colors.
 */
void InstrumentActor::loadColorMap(const std::pair<QString, bool> &cmap, bool reset_colors) {
  m_renderer->loadColorMap(cmap);
  m_currentCMap = cmap;
  if (reset_colors)
    resetColors();
}

//------------------------------------------------------------------------------
/** Get the detector position
 *
 * @param pickID :: pick Index maching the getDetector() calls;
 * @return the real-space position of the detector
 */
const Mantid::Kernel::V3D InstrumentActor::getDetPos(size_t pickID) const {
  const auto &detInfo = detectorInfo();
  if (pickID < detInfo.size()) {
    return detInfo.position(pickID);
  }
  return m_defaultPos;
}

const std::vector<Mantid::detid_t> &InstrumentActor::getAllDetIDs() const {
  const auto &detInfo = detectorInfo();
  return detInfo.detectorIDs();
}

/**
 * @param type :: 0 - linear, 1 - log10.
 */
void InstrumentActor::changeScaleType(int type) {
  m_renderer->changeScaleType(ColorMap::ScaleType(type));
  resetColors();
}

void InstrumentActor::changeNthPower(double nth_power) {
  m_renderer->changeNthPower(nth_power);
  resetColors();
}

void InstrumentActor::loadSettings() {
  QSettings settings;
  settings.beginGroup(m_settingsGroup);
  m_scaleType = ColorMap::ScaleType(settings.value("ScaleType", 0).toInt());
  // Load Colormap. If the file is invalid the default stored colour map is used
  m_currentCMap.first = settings.value("ColormapFile", ColorMap::defaultColorMap()).toString();
  m_currentCMap.second = settings.value("ColormapFileHighlightZeros", false).toBool();
  // Set values from settings
  m_showGuides = settings.value("ShowGuides", false).toBool();
  settings.endGroup();
}

void InstrumentActor::saveSettings() const {
  QSettings settings;
  settings.beginGroup(m_settingsGroup);
  settings.setValue("ColormapFile", m_currentCMap.first);
  settings.setValue("ColormapFileHighlightZeros", m_currentCMap.second);
  settings.setValue("ScaleType", static_cast<int>(m_renderer->getColorMap().getScaleType()));
  settings.setValue("ShowGuides", m_showGuides);
  settings.endGroup();
}

void InstrumentActor::setMinValue(double vmin) {
  if (m_autoscaling)
    return;
  if (vmin >= m_DataMaxScaleValue)
    return;
  m_DataMinScaleValue = vmin;
  resetColors();
}

void InstrumentActor::setMaxValue(double vmax) {
  if (m_autoscaling)
    return;
  if (vmax <= m_DataMinScaleValue)
    return;
  m_DataMaxScaleValue = vmax;
  resetColors();
}

void InstrumentActor::setMinMaxRange(double vmin, double vmax) {
  if (m_autoscaling)
    return;
  setDataMinMaxRange(vmin, vmax);
  resetColors();
}

bool InstrumentActor::wholeRange() const {
  return m_BinMinValue == m_WkspBinMinValue && m_BinMaxValue == m_WkspBinMaxValue;
}

size_t InstrumentActor::ndetectors() const { return m_detIndex2WsIndex.size() - m_components.size(); }

/**
 * Set autoscaling of the y axis. If autoscaling is on the minValue() and
 * maxValue()
 * return the actual min and max values in the data. If autoscaling is off
 *  minValue() and maxValue() are fixed and do not change after changing the x
 * integration range.
 * @param on :: On or Off.
 */
void InstrumentActor::setAutoscaling(bool on) {
  m_autoscaling = on;
  if (on) {
    m_DataMinScaleValue = m_DataMinValue;
    m_DataMaxScaleValue = m_DataMaxValue;
    resetColors();
  }
}

/**
 * Extracts the current applied mask to the main workspace
 * @returns the current applied mask to the main workspace
 */
Mantid::API::MatrixWorkspace_sptr InstrumentActor::extractCurrentMask() const {
  auto alg = AlgorithmManager::Instance().create("ExtractMask", -1);

  alg->setProperty("InputWorkspace", m_workspace);
  alg->getPointerToProperty("OutputWorkspace")->createTemporaryValue();
  alg->setLogging(false);
  alg->setAlwaysStoreInADS(false);
  // grab the algorithm ID to use for cancelling execution on quit events
  m_algID = alg->getAlgorithmID();
  alg->execute();
  return alg->getProperty("OutputWorkspace");
}

/**
 * Initialize the helper mask workspace with the mask from the data workspace.
 */
void InstrumentActor::initMaskHelper() const {
  if (m_maskWorkspace)
    return;
  try {
    // extract the mask (if any) from the data to the mask workspace
    m_maskWorkspace = extractCurrentMask();
  } catch (std::invalid_argument const &ex) {
    m_messageHandler.giveUserWarning(std::string("Error extracting mask: ") + ex.what(), "Mantid - Warning");
  } catch (...) {
    // don't know what to do here yet ...
    m_messageHandler.giveUserWarning(
        "Instrument Viewer is not supported yet for workspaces containing a detector scan.", "Mantid - Warning");
  }
}

/**
 * Checks if the actor has a mask workspace attached.
 */
bool InstrumentActor::hasMaskWorkspace() const { return m_maskWorkspace != nullptr; }

/**
 * Find a rotation from one orthonormal basis set (Xfrom,Yfrom,Zfrom) to
 * another orthonormal basis set (Xto,Yto,Zto). Both sets must be right-handed
 * (or same-handed, I didn't check). The method doesn't check the sets for
 * orthogonality
 * or normality. The result is a rotation quaternion such that:
 *   R.rotate(Xfrom) == Xto
 *   R.rotate(Yfrom) == Yto
 *   R.rotate(Zfrom) == Zto
 * @param Xfrom :: The X axis of the original basis set
 * @param Yfrom :: The Y axis of the original basis set
 * @param Zfrom :: The Z axis of the original basis set
 * @param Xto :: The X axis of the final basis set
 * @param Yto :: The Y axis of the final basis set
 * @param Zto :: The Z axis of the final basis set
 * @param R :: The output rotation as a quaternion
 * @param out :: Debug printout flag
 */
void InstrumentActor::BasisRotation(const Mantid::Kernel::V3D &Xfrom, const Mantid::Kernel::V3D &Yfrom,
                                    const Mantid::Kernel::V3D &Zfrom, const Mantid::Kernel::V3D &Xto,
                                    const Mantid::Kernel::V3D &Yto, const Mantid::Kernel::V3D &Zto,
                                    Mantid::Kernel::Quat &R, bool out) {
  // Find transformation from (X,Y,Z) to (XX,YY,ZZ)
  // R = R1*R2*R3, where R1, R2, and R3 are Euler rotations

  //  std::cerr<<"RCRotation-----------------------------\n";
  //  std::cerr<<"From "<<Xfrom<<Yfrom<<Zfrom<<'\n';
  //  std::cerr<<"To   "<<Xto<<Yto<<Zto<<'\n';

  double sZ = Zfrom.scalar_prod(Zto);
  if (fabs(sZ - 1) < TOLERANCE) // vectors the same
  {
    double sX = Xfrom.scalar_prod(Xto);
    if (fabs(sX - 1) < TOLERANCE) {
      R = Mantid::Kernel::Quat();
    } else if (fabs(sX + 1) < TOLERANCE) {
      R = Mantid::Kernel::Quat(180, Zfrom);
    } else {
      R = Mantid::Kernel::Quat(Xfrom, Xto);
    }
  } else if (fabs(sZ + 1) < TOLERANCE) // rotated by 180 degrees
  {
    if (fabs(Xfrom.scalar_prod(Xto) - 1) < TOLERANCE) {
      R = Mantid::Kernel::Quat(180., Xfrom);
    } else if (fabs(Yfrom.scalar_prod(Yto) - 1) < TOLERANCE) {
      R = Mantid::Kernel::Quat(180., Yfrom);
    } else {
      R = Mantid::Kernel::Quat(180., Xto) * Mantid::Kernel::Quat(Xfrom, Xto);
    }
  } else {
    // Rotation R1 of system (X,Y,Z) around Z by alpha
    Mantid::Kernel::Quat R1;

    const auto X1 = normalize(Zfrom.cross_prod(Zto));

    double sX = Xfrom.scalar_prod(Xto);
    if (fabs(sX - 1) < TOLERANCE) {
      R = Mantid::Kernel::Quat(Zfrom, Zto);
      return;
    }

    sX = Xfrom.scalar_prod(X1);
    if (fabs(sX - 1) < TOLERANCE) {
      R1 = Mantid::Kernel::Quat();
    } else if (fabs(sX + 1) < TOLERANCE) // 180 degree rotation
    {
      R1 = Mantid::Kernel::Quat(180., Zfrom);
    } else {
      R1 = Mantid::Kernel::Quat(Xfrom, X1);
    }
    if (out)
      std::cerr << "R1=" << R1 << '\n';

    // Rotation R2 around X1 by beta
    Mantid::Kernel::Quat R2(Zfrom, Zto); // vectors are different
    if (out)
      std::cerr << "R2=" << R2 << '\n';

    // Rotation R3 around ZZ by gamma
    Mantid::Kernel::Quat R3;
    sX = Xto.scalar_prod(X1);
    if (fabs(sX - 1) < TOLERANCE) {
      R3 = Mantid::Kernel::Quat();
    } else if (fabs(sX + 1) < TOLERANCE) // 180 degree rotation
    {
      R3 = Mantid::Kernel::Quat(180., Zto);
    } else {
      R3 = Mantid::Kernel::Quat(X1, Xto);
    }
    if (out)
      std::cerr << "R3=" << R3 << '\n';

    // Combined rotation
    R = R3 * R2 * R1;
  }
}

/**
 * Calculate a rotation to look in a particular direction.
 *
 * @param eye :: A direction to look in
 * @param up :: A vector showing the 'up' direction after the rotation. It
 * doesn't have to be normal to eye
 *   just non-collinear. If up is collinear to eye the actual 'up' direction is
 * undefined.
 * @param R :: The result rotation.
 */
void InstrumentActor::rotateToLookAt(const Mantid::Kernel::V3D &eye, const Mantid::Kernel::V3D &up,
                                     Mantid::Kernel::Quat &R) {
  if (eye.nullVector()) {
    throw std::runtime_error("The eye vector is null in InstrumentActor::rotateToLookAt.");
  }

  // Basis vectors of the OpenGL reference frame. Z points into the screen, Y
  // points up.
  constexpr Mantid::Kernel::V3D X(1, 0, 0);
  constexpr Mantid::Kernel::V3D Y(0, 1, 0);
  constexpr Mantid::Kernel::V3D Z(0, 0, 1);

  const auto z = normalize(eye);
  auto y = up;
  auto x = y.cross_prod(z);
  if (x.nullVector()) {
    // up || eye
    if (z.X() != 0.0) {
      x.setY(1.0);
    } else if (z.Y() != 0.0) {
      x.setZ(1.0);
    } else {
      x.setX(1.0);
    }
  }
  x.normalize();
  y = z.cross_prod(x);

  BasisRotation(x, y, z, X, Y, Z, R);
}

/**
 * Find the offsets in the spectrum's x vector of the bounds of integration.
 * @param wi :: The works[ace index of the spectrum.
 * @param imin :: Index of the lower bound: x_min == x(wi)[imin]
 * @param imax :: Index of the upper bound: x_max == x(wi)[imax]
 */
void InstrumentActor::getBinMinMaxIndex(size_t wi, size_t &imin, size_t &imax) const {
  Mantid::API::MatrixWorkspace_const_sptr ws = getWorkspace();
  const auto &x = ws->x(wi);

  auto x_begin = x.begin();
  auto x_end = x.end();
  if (x_begin == x_end)
    throw std::runtime_error("No bins found to plot");
  if (ws->isHistogramData())
    --x_end;

  if (wholeRange()) {
    imin = 0;
    imax = static_cast<size_t>(x_end - x_begin);
  } else {
    auto x_from = std::lower_bound(x_begin, x_end, minBinValue());
    auto x_to = std::upper_bound(x_begin, x_end, maxBinValue());
    imin = static_cast<size_t>(x_from - x_begin);
    imax = static_cast<size_t>(x_to - x_begin);
    if (imax <= imin) {
      if (x_from == x_end) {
        --x_from;
        x_to = x_end;
      } else {
        x_to = x_from + 1;
      }
      imin = static_cast<size_t>(x_from - x_begin);
      imax = static_cast<size_t>(x_to - x_begin);
    }
  }
}

/**
 * Set the minimum and the maximum data values on the color map scale.
 */
void InstrumentActor::setDataMinMaxRange(double vmin, double vmax) {
  if (vmin < m_DataMinValue) {
    vmin = m_DataMinValue;
  }
  if (vmin >= vmax)
    return;
  m_DataMinScaleValue = vmin;
  m_DataMaxScaleValue = vmax;
}

void InstrumentActor::calculateIntegratedSpectra(const Mantid::API::MatrixWorkspace &workspace) {
  // Use the workspace function to get the integrated spectra
  workspace.getIntegratedSpectra(m_integratedSignal, m_BinMinValue, m_BinMaxValue, wholeRange());
  // replace any values that are not finite
  std::replace_if(
      m_integratedSignal.begin(), m_integratedSignal.end(), [](double x) { return !std::isfinite(x); },
      InstrumentActor::INVALID_VALUE);

  m_maskBinsData.subtractIntegratedSpectra(workspace, m_integratedSignal);
}

void InstrumentActor::setDataIntegrationRange(const double &xmin, const double &xmax) {
  m_BinMinValue = xmin;
  m_BinMaxValue = xmax;

  auto workspace = getWorkspace();
  calculateIntegratedSpectra(*workspace);
  std::set<size_t> monitorIndices;

  for (auto monitor : m_monitors) {
    auto index = getWorkspaceIndex(monitor);
    if (index == INVALID_INDEX)
      continue;
    monitorIndices.emplace(index);
  }
  // check that there is at least 1 non-monitor spectrum
  if (monitorIndices.size() == m_integratedSignal.size()) {
    // there are only monitors - cannot skip them
    monitorIndices.clear();
  }

  if (m_integratedSignal.empty()) {
    // in case there are no spectra set some arbitrary values
    m_DataMinValue = 1.0;
    m_DataMaxValue = 10.0;
    m_DataPositiveMinValue = 1.0;
  } else {
    m_DataMinValue = DBL_MAX;
    m_DataMaxValue = -DBL_MAX;

    const auto &spectrumInfo = workspace->spectrumInfo();
    auto maskWksp = getMaskWorkspaceIfExists();

    // Ignore monitors if multiple detectors aren't grouped.
    for (size_t i = 0; i < m_integratedSignal.size(); i++) {
      const auto &spectrumDefinition = spectrumInfo.spectrumDefinition(i);
      // Ignore monitors if they are masked on the view
      if (spectrumDefinition.size() == 1 &&
          (std::find(monitorIndices.begin(), monitorIndices.end(), i) != monitorIndices.end() ||
           (maskWksp && maskWksp->isMasked(static_cast<int>(i)))))
        continue;

      auto sum = m_integratedSignal[i];

      if (sum == InstrumentActor::INVALID_VALUE)
        continue;
      if (sum < m_DataMinValue)
        m_DataMinValue = sum;
      if (sum > m_DataMaxValue)
        m_DataMaxValue = sum;
      if (sum > 0 && sum < m_DataPositiveMinValue)
        m_DataPositiveMinValue = sum;
    }
  }

  if (m_autoscaling) {
    m_DataMinScaleValue = m_DataMinValue;
    m_DataMaxScaleValue = m_DataMaxValue;
  }
}

/// Add a range of bins for masking
void InstrumentActor::addMaskBinsData(const std::vector<size_t> &indices) {
  // Ensure we do not have duplicate workspace indices.
  std::set<size_t> wi;
  for (auto det : indices) {
    auto index = getWorkspaceIndex(det);
    if (index == INVALID_INDEX)
      continue;
    wi.insert(index);
  }

  // We will be able to do this more efficiently in C++17
  std::vector<size_t> wsIndices(wi.cbegin(), wi.cend());

  if (!indices.empty()) {
    m_maskBinsData.addXRange(m_BinMinValue, m_BinMaxValue, wsIndices);
    auto workspace = getWorkspace();
    calculateIntegratedSpectra(*workspace);
    resetColors();
  }
}

/// Show if bin masks have been defined.
bool InstrumentActor::hasBinMask() const { return !m_maskBinsData.isEmpty(); }

QString InstrumentActor::getParameterInfo(size_t index) const {
  auto instr = getInstrument();
  const auto &compInfo = componentInfo();

  auto compID = compInfo.componentID(index);
  auto comp = instr->getComponentByID(compID);

  QString text = "";
  std::map<Mantid::Geometry::ComponentID, std::vector<std::string>> mapCmptToNameVector;

  auto paramNames = comp->getParameterNamesByComponent();
  for (auto &itParamName : paramNames) {
    // build the data structure I need Map comp id -> vector of names
    std::string paramName = itParamName.first;
    Mantid::Geometry::ComponentID paramCompId = itParamName.second;
    mapCmptToNameVector.emplace(paramCompId, std::vector<std::string>());
    // get the vector out and add the name
    mapCmptToNameVector[paramCompId].emplace_back(std::move(paramName));
  }

  // walk out from the selected component
  const Mantid::Geometry::IComponent *paramComp = comp.get();
  std::shared_ptr<const Mantid::Geometry::IComponent> parentComp;
  while (paramComp) {
    auto id = paramComp->getComponentID();
    auto &compParamNames = mapCmptToNameVector[id];
    if (compParamNames.size() > 0) {
      text += QString::fromStdString("\nParameters from: " + paramComp->getName() + "\n");
      std::sort(compParamNames.begin(), compParamNames.end(), Mantid::Kernel::CaseInsensitiveStringComparator());
      for (const auto &paramName : compParamNames) {
        // no need to search recursively as we are asking from the matching
        // component
        std::string paramValue = "";
        if (paramComp->getParameterVisible(paramName)) {
          paramValue = paramComp->getParameterAsString(paramName, false);
        }
        if (paramValue != "") {
          text += QString::fromStdString(paramName + ": " + paramValue + "\n");
        }
      }
    }
    parentComp = paramComp->getParent();
    paramComp = parentComp.get();
  }

  return text;
}

std::string InstrumentActor::getDefaultAxis() const { return getInstrument()->getDefaultAxis(); }

std::string InstrumentActor::getDefaultView() const { return getInstrument()->getDefaultView(); }

std::string InstrumentActor::getInstrumentName() const {
  const auto &compInfo = componentInfo();
  return compInfo.name(compInfo.root());
}

std::vector<std::string> InstrumentActor::getStringParameter(const std::string &name, bool recursive) const {
  return getInstrument()->getStringParameter(name, recursive);
}
/**
 * Save the state of the instrument actor to a project file.
 * @return string representing the current state of the instrumet actor.
 */
std::string InstrumentActor::saveToProject() const {
  throw std::runtime_error("InstrumentActor::saveToProject() not implemented for Qt >= 5");
}

/**
 * Load the state of the instrument actor from a project file.
 * @param lines :: string representing the current state of the instrumet actor.
 */
void InstrumentActor::loadFromProject(const std::string &lines) {
  Q_UNUSED(lines);
  throw std::runtime_error("InstrumentActor::saveToProject() not implemented for Qt >= 5");
}

bool InstrumentActor::hasGridBank() const { return m_hasGrid; }

size_t InstrumentActor::getNumberOfGridLayers() const { return m_numGridLayers; }

void InstrumentActor::setGridLayer(bool isUsingLayer, int layer) const {
  m_renderer->enableGridBankLayers(isUsingLayer, layer);
  m_renderer->reset();
  emit colorMapChanged();
}

const InstrumentRenderer &InstrumentActor::getInstrumentRenderer() const { return *m_renderer; }

/** If instrument.geometry.view is set to Default or Physical, then the physical
 * instrument componentInfo is returned. Othewise this returns the neutronic
 * version.
 */
const Mantid::Geometry::ComponentInfo &InstrumentActor::componentInfo() const {
  if (m_isPhysicalInstrument)
    return *m_physicalComponentInfo;
  else
    return getWorkspace()->componentInfo();
}

/** If instrument.geometry.view is set to Default or Physical, then the physical
 * instrument detectorInfo is returned. Othewise this returns the neutronic
 * version.
 */
const Mantid::Geometry::DetectorInfo &InstrumentActor::detectorInfo() const {
  if (m_isPhysicalInstrument)
    return *m_physicalDetectorInfo;
  else
    return getWorkspace()->detectorInfo();
}
} // namespace MantidQt::MantidWidgets
