#include "MantidQtWidgets/InstrumentView/InstrumentActor.h"
#include "MantidQtWidgets/Common/TSVSerialiser.h"
#include "MantidQtWidgets/InstrumentView/InstrumentRenderer.h"
#include "MantidQtWidgets/InstrumentView/OpenGLError.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/CommonBinsValidator.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/IMaskWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
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
#include <QSettings>

#include <limits>
#include <numeric>

using namespace Mantid::Kernel::Exception;
using namespace Mantid::Geometry;
using namespace Mantid::API;
using namespace Mantid;

namespace MantidQt {
namespace MantidWidgets {
namespace {
bool isPhysicalView() {
  std::string view = Mantid::Kernel::ConfigService::Instance().getString(
      "instrument.view.geometry");

  return boost::iequals("Default", view) || boost::iequals("Physical", view);
}

} // namespace

const size_t InstrumentActor::INVALID_INDEX =
    std::numeric_limits<size_t>::max();
double InstrumentActor::m_tolerance = 0.00001;

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
InstrumentActor::InstrumentActor(const QString &wsName, bool autoscaling,
                                 double scaleMin, double scaleMax)
    : m_workspace(AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
          wsName.toStdString())),
      m_ragged(true), m_autoscaling(autoscaling), m_defaultPos(),
      m_isPhysicalInstrument(false) {
  // settings
  loadSettings();

  auto sharedWorkspace = m_workspace.lock();

  if (!sharedWorkspace)
    throw std::logic_error(
        "InstrumentActor passed a workspace that isn't a MatrixWorkspace");
  setupPhysicalInstrumentIfExists();

  for (size_t i = 0; i < componentInfo().size(); ++i) {
    if (!componentInfo().isDetector(i))
      m_components.push_back(i);
    else if (detectorInfo().isMonitor(i))
      m_monitors.push_back(i);
  }

  m_isCompVisible.assign(componentInfo().size(), true);

  m_renderer.reset(new InstrumentRenderer(*this));
  m_renderer->changeScaleType(m_scaleType);

  // set up the color map
  if (!m_currentColorMapFilename.isEmpty()) {
    loadColorMap(m_currentColorMapFilename, false);
  }

  // set up data ranges and colours
  setUpWorkspace(sharedWorkspace, scaleMin, scaleMax);

  // If the instrument is empty, maybe only having the sample and source
  if (detectorInfo().size() == 0) {
    QMessageBox::warning(nullptr, "MantidPlot - Warning",
                         "This instrument appears to contain no detectors",
                         "OK");
  }
}

/**
 * Destructor
 */
InstrumentActor::~InstrumentActor() { saveSettings(); }

/**
 * Set up the workspace: calculate the value ranges, set the colours.
 * @param sharedWorkspace :: A shared pointer to the workspace.
 * @param scaleMin :: Minimum limit on the color map axis. If autoscale this
 * value is ignored.
 * @param scaleMax :: Maximum limit on the color map axis. If autoscale this
 * value is ignored.
 */
void InstrumentActor::setUpWorkspace(
    boost::shared_ptr<const Mantid::API::MatrixWorkspace> sharedWorkspace,
    double scaleMin, double scaleMax) {
  m_WkspBinMinValue = DBL_MAX;
  m_WkspBinMaxValue = -DBL_MAX;
  const auto &spectrumInfo = sharedWorkspace->spectrumInfo();
  m_detIndex2WsIndex.resize(componentInfo().size(), INVALID_INDEX);
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
    for (auto info : specDef)
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
  auto wsValidator = Mantid::API::CommonBinsValidator();
  m_ragged = !wsValidator.isValid(sharedWorkspace).empty();
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
  setChildVisibility(false);
  const auto &compInfo = componentInfo();
  auto children = compInfo.componentsInSubtree(componentIndex);
  m_isCompVisible[componentIndex] = true;
  for (auto child : children)
    m_isCompVisible[child] = true;

  resetColors();
}

void InstrumentActor::setChildVisibility(bool on) {
  std::fill(m_isCompVisible.begin(), m_isCompVisible.end(), on);
}

bool InstrumentActor::hasChildVisible() const {
  return std::any_of(m_isCompVisible.begin(), m_isCompVisible.end(),
                     [](bool visible) { return visible; });
}

/** Returns the workspace relating to this instrument view.
 *  !!!! DON'T USE THIS TO GET HOLD OF THE INSTRUMENT !!!!
 *  !!!! USE InstrumentActor::getInstrument() BELOW !!!!
 */
MatrixWorkspace_const_sptr InstrumentActor::getWorkspace() const {
  auto sharedWorkspace = m_workspace.lock();

  if (!sharedWorkspace) {
    throw std::runtime_error("Instrument view: workspace doesn't exist");
  }

  return sharedWorkspace;
}

void InstrumentActor::getBoundingBox(Mantid::Kernel::V3D &minBound,
                                     Mantid::Kernel::V3D &maxBound) const {
  const auto &compInfo = componentInfo();
  auto bb = compInfo.boundingBox(compInfo.root());
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
void InstrumentActor::setMaskMatrixWorkspace(
    MatrixWorkspace_sptr wsMask) const {
  m_maskWorkspace = wsMask;
}

void InstrumentActor::invertMaskWorkspace() const {
  Mantid::API::MatrixWorkspace_sptr outputWS;

  const std::string maskName = "__InstrumentActor_MaskWorkspace_invert";
  Mantid::API::AnalysisDataService::Instance().addOrReplace(
      maskName, getMaskMatrixWorkspace());
  Mantid::API::IAlgorithm *invertAlg =
      Mantid::API::FrameworkManager::Instance().createAlgorithm(
          "BinaryOperateMasks", -1);
  invertAlg->setChild(true);
  invertAlg->setPropertyValue("InputWorkspace1", maskName);
  invertAlg->setPropertyValue("OutputWorkspace", maskName);
  invertAlg->setPropertyValue("OperationType", "NOT");
  invertAlg->execute();

  m_maskWorkspace = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
      Mantid::API::AnalysisDataService::Instance().retrieve(maskName));
  Mantid::API::AnalysisDataService::Instance().remove(maskName);
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
  return boost::dynamic_pointer_cast<IMaskWorkspace>(m_maskWorkspace);
}

/**
 * Returns the mask workspace relating to this instrument view as a
 * IMaskWorkspace
 * if it exists or empty pointer if it doesn't.
 */
IMaskWorkspace_sptr InstrumentActor::getMaskWorkspaceIfExists() const {
  if (!m_maskWorkspace)
    return IMaskWorkspace_sptr();
  return boost::dynamic_pointer_cast<IMaskWorkspace>(m_maskWorkspace);
}

/**
 * Apply mask stored in the helper mask workspace to the data workspace.
 */
void InstrumentActor::applyMaskWorkspace() {
  auto wsName = getWorkspace()->getName();
  if (m_maskWorkspace) {
    // Mask detectors
    try {
      Mantid::API::IAlgorithm *alg =
          Mantid::API::FrameworkManager::Instance().createAlgorithm(
              "MaskDetectors", -1);
      alg->setPropertyValue("Workspace", wsName);
      alg->setProperty("MaskedWorkspace", m_maskWorkspace);
      alg->execute();
      // After the algorithm finishes the InstrumentWindow catches the
      // after-replace notification
      // and updates this instrument actor.
    } catch (...) {
      QMessageBox::warning(nullptr, "MantidPlot - Warning",
                           "An error accured when applying the mask.", "OK");
    }
  }

  // Mask bins
  m_maskBinsData.mask(wsName);

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

std::vector<size_t> InstrumentActor::getMonitors() const { return m_monitors; }

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

const MantidColorMap &InstrumentActor::getColorMap() const {
  return m_renderer->getColorMap();
}

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

QList<Mantid::detid_t>
InstrumentActor::getDetIDs(const std::vector<size_t> &dets) const {
  QList<Mantid::detid_t> detIDs;
  detIDs.reserve(static_cast<int>(dets.size()));
  for (auto det : dets)
    detIDs.append(getDetID(det));
  return detIDs;
}

/**
 * Get a component id of a picked component.
 */
Mantid::Geometry::ComponentID
InstrumentActor::getComponentID(size_t pickID) const {
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
size_t InstrumentActor::getWorkspaceIndex(size_t index) const {
  return m_detIndex2WsIndex[index];
}

/**
 * Set an interval in the data workspace x-vector's units in which the data are
 * to be
 * integrated to calculate the detector colours.
 *
 * @param xmin :: The lower bound.
 * @param xmax :: The upper bound.
 */
void InstrumentActor::setIntegrationRange(const double &xmin,
                                          const double &xmax) {
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
    return -1.0;
  return m_specIntegrs.at(i);
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
 * @param size :: (optional input) Size of the output vectors. If not given it
 * will be determined automatically.
 */
void InstrumentActor::sumDetectors(const std::vector<size_t> &dets,
                                   std::vector<double> &x,
                                   std::vector<double> &y, size_t size) const {
  Mantid::API::MatrixWorkspace_const_sptr ws = getWorkspace();
  if (size > ws->blocksize() || size == 0) {
    size = ws->blocksize();
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
 * Sum counts in detectors for purposes of rough plotting against the units on
 * the x-axis.
 * Assumes that all spectra share the x vector.
 *
 * @param dets :: A list of detector Indices to sum.
 * @param x :: (output) Time of flight values (or whatever values the x axis
 * has) to plot against.
 * @param y :: (output) The sums of the counts for each bin.
 */
void InstrumentActor::sumDetectorsUniform(const std::vector<size_t> &dets,
                                          std::vector<double> &x,
                                          std::vector<double> &y) const {

  bool isDataEmpty = dets.empty();

  auto wi = getWorkspaceIndex(dets[0]);

  if (wi == INVALID_INDEX)
    isDataEmpty = true;

  if (isDataEmpty) {
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
  for (auto det : dets) {
    auto index = getWorkspaceIndex(det);
    if (index == INVALID_INDEX)
      continue;
    const auto &Y = ws->y(index);
    std::transform(y.begin(), y.end(), Y.begin() + imin, y.begin(),
                   std::plus<double>());
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
void InstrumentActor::sumDetectorsRagged(const std::vector<size_t> &dets,
                                         std::vector<double> &x,
                                         std::vector<double> &y,
                                         size_t size) const {
  if (dets.empty() || size == 0) {
    x.clear();
    y.clear();
    return;
  }

  Mantid::API::MatrixWorkspace_const_sptr ws = getWorkspace();
  //  create a workspace to hold the data from the selected detectors
  Mantid::API::MatrixWorkspace_sptr dws =
      Mantid::API::WorkspaceFactory::Instance().create(ws, dets.size());

  // x-axis limits
  double xStart = maxBinValue();
  double xEnd = minBinValue();

  size_t nSpec = 0; // number of actual spectra to add
  // fill in the temp workspace with the data from the detectors
  for (auto det : dets) {
    auto index = getWorkspaceIndex(det);
    if (index == INVALID_INDEX)
      continue;
    dws->setHistogram(nSpec, ws->histogram(index));
    double xmin = dws->x(nSpec).front();
    double xmax = dws->x(nSpec).back();
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
  std::string params =
      QString("%1,%2,%3").arg(xStart).arg(dx).arg(xEnd).toStdString();
  std::string outName = "_TMP_sumDetectorsRagged";

  try {
    // rebin all spectra to the same binning
    Mantid::API::IAlgorithm *alg =
        Mantid::API::FrameworkManager::Instance().createAlgorithm("Rebin", -1);
    alg->setProperty("InputWorkspace", dws);
    alg->setPropertyValue("OutputWorkspace", outName);
    alg->setPropertyValue("Params", params);
    alg->execute();

    ws = boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
        Mantid::API::AnalysisDataService::Instance().retrieve(outName));
    Mantid::API::AnalysisDataService::Instance().remove(outName);

    const auto &X = ws->x(0);
    const auto &Y = ws->y(0);
    x.assign(X.begin(), X.end());
    y.assign(Y.begin(), Y.end());

    // add the spectra
    for (size_t i = 0; i < nSpec; ++i) {
      const auto &Y = ws->y(i);
      std::transform(y.begin(), y.end(), Y.begin(), y.begin(),
                     std::plus<double>());
    }
  } catch (std::invalid_argument &) {
    // wrong Params for any reason
    x.resize(size, (xEnd + xStart) / 2);
    y.resize(size, 0.0);
  }
}

/**
 * Recalculate the detector colors based on the integrated values in
 * m_specIntegrs and
 * the masking information in ....
 */
void InstrumentActor::resetColors() {
  m_renderer->reset();
  emit colorMapChanged();
}

void InstrumentActor::updateColors() {
  setIntegrationRange(m_BinMinValue, m_BinMaxValue);
  resetColors();
}

/**
 * @param on :: True or false for on or off.
 */
void InstrumentActor::showGuides(bool on) {
  m_showGuides = on;
  resetColors();
}

GLColor InstrumentActor::getColor(size_t index) const {
  return m_renderer->getColor(index);
}

void InstrumentActor::draw(bool picking) const {
  m_renderer->renderInstrument(m_isCompVisible, m_showGuides, picking);
}

/**
 * @param fname :: A color map file name.
 * @param reset_colors :: An option to reset the detector colors.
 */
void InstrumentActor::loadColorMap(const QString &fname, bool reset_colors) {
  m_renderer->loadColorMap(fname);
  m_currentColorMapFilename = fname;
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
  m_renderer->changeScaleType(type);
  resetColors();
}

void InstrumentActor::changeNthPower(double nth_power) {
  m_renderer->changeNthPower(nth_power);
  resetColors();
}

void InstrumentActor::loadSettings() {
  QSettings settings;
  settings.beginGroup("Mantid/InstrumentWidget");
  m_scaleType = static_cast<GraphOptions::ScaleType>(
      settings.value("ScaleType", 0).toInt());
  // Load Colormap. If the file is invalid the default stored colour map is used
  m_currentColorMapFilename = settings.value("ColormapFile", "").toString();
  // Set values from settings
  m_showGuides = settings.value("ShowGuides", false).toBool();
  settings.endGroup();
}

void InstrumentActor::saveSettings() {
  QSettings settings;
  settings.beginGroup("Mantid/InstrumentWidget");
  settings.setValue("ColormapFile", m_currentColorMapFilename);
  settings.setValue("ScaleType", (int)m_renderer->getColorMap().getScaleType());
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
  return m_BinMinValue == m_WkspBinMinValue &&
         m_BinMaxValue == m_WkspBinMaxValue;
}

size_t InstrumentActor::ndetectors() const {
  return m_detIndex2WsIndex.size() - m_components.size();
}

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
  const std::string maskName = "__InstrumentActor_MaskWorkspace";
  Mantid::API::IAlgorithm *alg =
      Mantid::API::FrameworkManager::Instance().createAlgorithm("ExtractMask",
                                                                -1);
  alg->setPropertyValue("InputWorkspace", getWorkspace()->getName());
  alg->setPropertyValue("OutputWorkspace", maskName);
  alg->execute();

  Mantid::API::MatrixWorkspace_sptr maskWorkspace =
      boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
          Mantid::API::AnalysisDataService::Instance().retrieve(maskName));
  Mantid::API::AnalysisDataService::Instance().remove(maskName);
  return maskWorkspace;
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
  } catch (...) {
    // don't know what to do here yet ...
    QMessageBox::warning(nullptr, "MantidPlot - Warning",
                         "An error occurred when extracting the mask.", "OK");
  }
}

/**
 * Checks if the actor has a mask workspace attached.
 */
bool InstrumentActor::hasMaskWorkspace() const {
  return m_maskWorkspace != nullptr;
}

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
void InstrumentActor::BasisRotation(const Mantid::Kernel::V3D &Xfrom,
                                    const Mantid::Kernel::V3D &Yfrom,
                                    const Mantid::Kernel::V3D &Zfrom,
                                    const Mantid::Kernel::V3D &Xto,
                                    const Mantid::Kernel::V3D &Yto,
                                    const Mantid::Kernel::V3D &Zto,
                                    Mantid::Kernel::Quat &R, bool out) {
  // Find transformation from (X,Y,Z) to (XX,YY,ZZ)
  // R = R1*R2*R3, where R1, R2, and R3 are Euler rotations

  //  std::cerr<<"RCRotation-----------------------------\n";
  //  std::cerr<<"From "<<Xfrom<<Yfrom<<Zfrom<<'\n';
  //  std::cerr<<"To   "<<Xto<<Yto<<Zto<<'\n';

  double sZ = Zfrom.scalar_prod(Zto);
  if (fabs(sZ - 1) < m_tolerance) // vectors the same
  {
    double sX = Xfrom.scalar_prod(Xto);
    if (fabs(sX - 1) < m_tolerance) {
      R = Mantid::Kernel::Quat();
    } else if (fabs(sX + 1) < m_tolerance) {
      R = Mantid::Kernel::Quat(180, Zfrom);
    } else {
      R = Mantid::Kernel::Quat(Xfrom, Xto);
    }
  } else if (fabs(sZ + 1) < m_tolerance) // rotated by 180 degrees
  {
    if (fabs(Xfrom.scalar_prod(Xto) - 1) < m_tolerance) {
      R = Mantid::Kernel::Quat(180., Xfrom);
    } else if (fabs(Yfrom.scalar_prod(Yto) - 1) < m_tolerance) {
      R = Mantid::Kernel::Quat(180., Yfrom);
    } else {
      R = Mantid::Kernel::Quat(180., Xto) * Mantid::Kernel::Quat(Xfrom, Xto);
    }
  } else {
    // Rotation R1 of system (X,Y,Z) around Z by alpha
    Mantid::Kernel::V3D X1;
    Mantid::Kernel::Quat R1;

    X1 = Zfrom.cross_prod(Zto);
    X1.normalize();

    double sX = Xfrom.scalar_prod(Xto);
    if (fabs(sX - 1) < m_tolerance) {
      R = Mantid::Kernel::Quat(Zfrom, Zto);
      return;
    }

    sX = Xfrom.scalar_prod(X1);
    if (fabs(sX - 1) < m_tolerance) {
      R1 = Mantid::Kernel::Quat();
    } else if (fabs(sX + 1) < m_tolerance) // 180 degree rotation
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
    if (fabs(sX - 1) < m_tolerance) {
      R3 = Mantid::Kernel::Quat();
    } else if (fabs(sX + 1) < m_tolerance) // 180 degree rotation
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
void InstrumentActor::rotateToLookAt(const Mantid::Kernel::V3D &eye,
                                     const Mantid::Kernel::V3D &up,
                                     Mantid::Kernel::Quat &R) {
  if (eye.nullVector()) {
    throw std::runtime_error(
        "The eye vector is null in InstrumentActor::rotateToLookAt.");
  }

  // Basis vectors of the OpenGL reference frame. Z points into the screen, Y
  // points up.
  const Mantid::Kernel::V3D X(1, 0, 0);
  const Mantid::Kernel::V3D Y(0, 1, 0);
  const Mantid::Kernel::V3D Z(0, 0, 1);

  Mantid::Kernel::V3D x, y, z;
  z = eye;
  z.normalize();
  y = up;
  x = y.cross_prod(z);
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
void InstrumentActor::getBinMinMaxIndex(size_t wi, size_t &imin,
                                        size_t &imax) const {
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

void InstrumentActor::calculateIntegratedSpectra(
    const Mantid::API::MatrixWorkspace &workspace) {
  // Use the workspace function to get the integrated spectra
  workspace.getIntegratedSpectra(m_specIntegrs, m_BinMinValue, m_BinMaxValue,
                                 wholeRange());
  m_maskBinsData.subtractIntegratedSpectra(workspace, m_specIntegrs);
}

void InstrumentActor::setDataIntegrationRange(const double &xmin,
                                              const double &xmax) {
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
  if (monitorIndices.size() == m_specIntegrs.size()) {
    // there are only monitors - cannot skip them
    monitorIndices.clear();
  }

  if (m_specIntegrs.empty()) {
    // in case there are no spectra set some arbitrary values
    m_DataMinValue = 1.0;
    m_DataMaxValue = 10.0;
    m_DataPositiveMinValue = 1.0;
  } else {
    m_DataMinValue = DBL_MAX;
    m_DataMaxValue = -DBL_MAX;

    if (std::any_of(m_specIntegrs.begin(), m_specIntegrs.end(),
                    [](double val) { return !std::isfinite(val); }))
      throw std::runtime_error(
          "The workspace contains values that cannot be displayed (infinite "
          "or NaN).\n"
          "Please run ReplaceSpecialValues algorithm for correction.");

    const auto &spectrumInfo = workspace->spectrumInfo();

    // Ignore monitors if multiple detectors aren't grouped.
    for (size_t i = 0; i < m_specIntegrs.size(); i++) {
      const auto &spectrumDefinition = spectrumInfo.spectrumDefinition(i);
      if (spectrumDefinition.size() == 1 &&
          std::find(monitorIndices.begin(), monitorIndices.end(), i) !=
              monitorIndices.end())
        continue;

      auto sum = m_specIntegrs[i];

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
  std::vector<size_t> wsIndices;
  wsIndices.reserve(indices.size());
  for (auto det : indices) {
    auto index = getWorkspaceIndex(det);
    if (index == INVALID_INDEX)
      continue;
    wsIndices.emplace_back(index);
  }
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
  std::map<Mantid::Geometry::ComponentID, std::vector<std::string>>
      mapCmptToNameVector;

  auto paramNames = comp->getParameterNamesByComponent();
  for (auto itParamName = paramNames.begin(); itParamName != paramNames.end();
       ++itParamName) {
    // build the data structure I need Map comp id -> vector of names
    std::string paramName = itParamName->first;
    Mantid::Geometry::ComponentID paramCompId = itParamName->second;
    // attempt to insert this will fail silently if the key already exists
    if (mapCmptToNameVector.find(paramCompId) == mapCmptToNameVector.end()) {
      mapCmptToNameVector.emplace(paramCompId, std::vector<std::string>());
    }
    // get the vector out and add the name
    mapCmptToNameVector[paramCompId].emplace_back(paramName);
  }

  // walk out from the selected component
  const Mantid::Geometry::IComponent *paramComp = comp.get();
  boost::shared_ptr<const Mantid::Geometry::IComponent> parentComp;
  while (paramComp) {
    auto id = paramComp->getComponentID();
    auto &compParamNames = mapCmptToNameVector[id];
    if (compParamNames.size() > 0) {
      text += QString::fromStdString(
          "\nParameters from: " + paramComp->getName() + "\n");
      std::sort(compParamNames.begin(), compParamNames.end(),
                Mantid::Kernel::CaseInsensitiveStringComparator());
      for (auto itParamName = compParamNames.begin();
           itParamName != compParamNames.end(); ++itParamName) {
        std::string paramName = *itParamName;
        // no need to search recursively as we are asking from the matching
        // component
        std::string paramValue =
            paramComp->getParameterAsString(paramName, false);
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

std::string InstrumentActor::getDefaultAxis() const {
  return getInstrument()->getDefaultAxis();
}

std::string InstrumentActor::getDefaultView() const {
  return getInstrument()->getDefaultView();
}

std::string InstrumentActor::getInstrumentName() const {
  const auto &compInfo = componentInfo();
  return compInfo.name(compInfo.root());
}

std::vector<std::string>
InstrumentActor::getStringParameter(const std::string &name,
                                    bool recursive) const {
  return getInstrument()->getStringParameter(name, recursive);
}
/**
 * Save the state of the instrument actor to a project file.
 * @return string representing the current state of the instrumet actor.
 */
std::string InstrumentActor::saveToProject() const {
  API::TSVSerialiser tsv;
  const std::string currentColorMap = getCurrentColorMap().toStdString();

  if (!currentColorMap.empty())
    tsv.writeLine("FileName") << currentColorMap;

  tsv.writeSection("binmasks", m_maskBinsData.saveToProject());
  return tsv.outputLines();
}

/**
 * Load the state of the instrument actor from a project file.
 * @param lines :: string representing the current state of the instrumet actor.
 */
void InstrumentActor::loadFromProject(const std::string &lines) {
  API::TSVSerialiser tsv(lines);
  if (tsv.selectLine("FileName")) {
    QString filename;
    tsv >> filename;
    loadColorMap(filename);
  }

  if (tsv.selectSection("binmasks")) {
    std::string binMaskLines;
    tsv >> binMaskLines;
    m_maskBinsData.loadFromProject(binMaskLines);
  }
}

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
} // namespace MantidWidgets
} // namespace MantidQt
