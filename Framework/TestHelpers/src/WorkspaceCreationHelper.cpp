// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/*********************************************************************************
 *  PLEASE READ THIS!!!!!!!
 *
 *  This collection of functions MAY NOT be used in any test from a package
 *below
 *  DataObjects (e.g. Kernel, Geometry, API).
 *  Conversely, this file MAY NOT be modified to use anything from a package
 *higher
 *  than DataObjects (e.g. any algorithm), even if going via the factory.
 *********************************************************************************/
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/InstrumentCreationHelper.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/ScanningWorkspaceBuilder.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidHistogramData/HistogramDx.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidKernel/MersenneTwister.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidKernel/make_cow.h"

#include <cmath>
#include <sstream>
#include <utility>

namespace WorkspaceCreationHelper {
using namespace Mantid;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::HistogramData;
using Mantid::MantidVec;
using Mantid::MantidVecPtr;
using Mantid::Types::Core::DateAndTime;
using Mantid::Types::Event::TofEvent;

MockAlgorithm::MockAlgorithm(size_t nSteps)
    : m_Progress(std::make_unique<API::Progress>(this, 0.0, 1.0, nSteps)) {}

EPPTableRow::EPPTableRow(const double peakCentre_, const double sigma_,
                         const double height_, const FitStatus fitStatus_)
    : peakCentre(peakCentre_), sigma(sigma_), height(height_),
      fitStatus(fitStatus_) {}

EPPTableRow::EPPTableRow(const int index, const double peakCentre_,
                         const double sigma_, const double height_,
                         const FitStatus fitStatus_)
    : workspaceIndex(index), peakCentre(peakCentre_), sigma(sigma_),
      height(height_), fitStatus(fitStatus_) {}

/**
 * Deletes a workspace
 * @param name :: The name of the workspace
 */
void removeWS(const std::string &name) {
  Mantid::API::AnalysisDataService::Instance().remove(name);
}

/**
 * Creates bin or point based histograms based on the data passed
 * in for Y and E values and the bool specified.
 *
 * @param isHistogram :: Specifies whether the returned histogram
 * should use points or bin edges for the x axis. True gives bin edges.
 * @param yAxis :: Takes an rvalue (move) of the y axis for the new histogram
 * @param eAxis :: Takes an rvalue (move) of the e axis for the new histogram
 *
 * @return :: Returns a histogram with the user specified X axis type
 * and the data the user passed in.
 */
template <typename YType, typename EType>
Histogram createHisto(bool isHistogram, YType &&yAxis, EType &&eAxis) {
  // We don't need to check if y.size() == e.size() as the histogram
  // type does this at construction
  const size_t yValsSize = yAxis.size();
  if (isHistogram) {
    BinEdges xAxis(yValsSize + 1, LinearGenerator(1, 1));
    Histogram histo{std::move(xAxis), std::forward<YType>(yAxis),
                    std::forward<EType>(eAxis)};
    return histo;
  } else {
    Points xAxis(yValsSize, LinearGenerator(1, 1));
    Histogram pointsHisto{std::move(xAxis), std::forward<YType>(yAxis),
                          std::forward<EType>(eAxis)};
    return pointsHisto;
  }
}

Workspace2D_sptr create1DWorkspaceRand(int size, bool isHisto) {

  MersenneTwister randomGen(DateAndTime::getCurrentTime().nanoseconds(), 0,
                            std::numeric_limits<int>::max());

  auto randFunc = [&randomGen] { return randomGen.nextValue(); };
  Counts counts(size, randFunc);
  CountStandardDeviations errorVals(size, randFunc);

  auto retVal = boost::make_shared<Workspace2D>();
  retVal->initialize(1, createHisto(isHisto, counts, errorVals));
  return retVal;
}

Workspace2D_sptr create1DWorkspaceConstant(int size, double value, double error,
                                           bool isHisto) {
  Counts yVals(size, value);
  CountStandardDeviations errVals(size, error);

  auto retVal = boost::make_shared<Workspace2D>();
  retVal->initialize(1, createHisto(isHisto, yVals, errVals));
  return retVal;
}

Workspace2D_sptr create1DWorkspaceConstantWithXerror(int size, double value,
                                                     double error,
                                                     double xError,
                                                     bool isHisto) {
  auto ws = create1DWorkspaceConstant(size, value, error, isHisto);
  auto dx1 = Kernel::make_cow<HistogramData::HistogramDx>(size, xError);
  ws->setSharedDx(0, dx1);
  return ws;
}

Workspace2D_sptr create1DWorkspaceFib(int size, bool isHisto) {
  BinEdges xVals(size + 1, LinearGenerator(1, 1));
  Counts yVals(size, FibSeries<double>());
  CountStandardDeviations errVals(size);

  auto retVal = boost::make_shared<Workspace2D>();
  retVal->initialize(1, createHisto(isHisto, yVals, errVals));
  return retVal;
}

Workspace2D_sptr create2DWorkspace(size_t nhist, size_t numBoundaries) {
  return create2DWorkspaceBinned(nhist, numBoundaries);
}

/** Create a Workspace2D where the Y value at each bin is
 * == to the workspace index
 * @param nhist :: # histograms
 * @param numBoundaries :: # of bins
 * @return Workspace2D
 */
Workspace2D_sptr create2DWorkspaceWhereYIsWorkspaceIndex(int nhist,
                                                         int numBoundaries) {
  Workspace2D_sptr out = create2DWorkspaceBinned(nhist, numBoundaries);
  for (int workspaceIndex = 0; workspaceIndex < nhist; workspaceIndex++) {
    std::vector<double> yValues(numBoundaries,
                                static_cast<double>(workspaceIndex));
    out->mutableY(workspaceIndex) = std::move(yValues);
  }

  return out;
}

Workspace2D_sptr create2DWorkspaceThetaVsTOF(int nHist, int nBins) {

  Workspace2D_sptr outputWS = create2DWorkspaceBinned(nHist, nBins);
  auto newAxis = std::make_unique<NumericAxis>(nHist);
  auto newAxisRaw = newAxis.get();
  outputWS->replaceAxis(1, std::move(newAxis));
  newAxisRaw->unit() = boost::make_shared<Units::Degrees>();
  for (int i = 0; i < nHist; ++i) {
    newAxisRaw->setValue(i, i + 1);
  }

  return outputWS;
}

/**
 * @brief create2DWorkspaceWithValues
 * @param nHist :: Number of spectra
 * @param nBins :: Number of points (not bin edges!)
 * @param isHist :: Flag if it is a histogram or point data
 * @param maskedWorkspaceIndices :: Mask workspace indices
 * @param xVal :: bin edge or point
 * @param yVal :: y value
 * @param eVal :: error values
 * @param hasDx :: wether workspace has dx values defined (default is false)
 * @return A workspace filled with nBins bins or points and nHist spectra of the
 * values yVal and the error eVal as well as Dx values which are copies of the y
 * values
 */
Workspace2D_sptr
create2DWorkspaceWithValues(int64_t nHist, int64_t nBins, bool isHist,
                            const std::set<int64_t> &maskedWorkspaceIndices,
                            double xVal, double yVal, double eVal,
                            bool hasDx = false) {
  auto x1 = Kernel::make_cow<HistogramData::HistogramX>(
      isHist ? nBins + 1 : nBins, LinearGenerator(xVal, 1.0));
  Counts y1(nBins, yVal);
  CountStandardDeviations e1(nBins, eVal);
  auto dx = Kernel::make_cow<HistogramData::HistogramDx>(nBins, yVal);
  auto retVal = boost::make_shared<Workspace2D>();
  retVal->initialize(nHist, createHisto(isHist, y1, e1));
  for (int i = 0; i < nHist; i++) {
    retVal->setSharedX(i, x1);
    if (hasDx)
      retVal->setSharedDx(i, dx);
    retVal->getSpectrum(i).setDetectorID(i);
    retVal->getSpectrum(i).setSpectrumNo(i);
  }
  retVal = maskSpectra(retVal, maskedWorkspaceIndices);
  return retVal;
}

Workspace2D_sptr create2DWorkspaceWithValuesAndXerror(
    int64_t nHist, int64_t nBins, bool isHist, double xVal, double yVal,
    double eVal, double dxVal,
    const std::set<int64_t> &maskedWorkspaceIndices) {
  auto ws = create2DWorkspaceWithValues(
      nHist, nBins, isHist, maskedWorkspaceIndices, xVal, yVal, eVal);
  PointStandardDeviations dx1(nBins, dxVal);
  for (int i = 0; i < nHist; i++) {
    ws->setPointStandardDeviations(i, dx1);
  }
  return ws;
}

Workspace2D_sptr
create2DWorkspace123(int64_t nHist, int64_t nBins, bool isHist,
                     const std::set<int64_t> &maskedWorkspaceIndices,
                     bool hasDx) {
  return create2DWorkspaceWithValues(
      nHist, nBins, isHist, maskedWorkspaceIndices, 1.0, 2.0, 3.0, hasDx);
}

Workspace2D_sptr
create2DWorkspace154(int64_t nHist, int64_t nBins, bool isHist,
                     const std::set<int64_t> &maskedWorkspaceIndices,
                     bool hasDx) {
  return create2DWorkspaceWithValues(
      nHist, nBins, isHist, maskedWorkspaceIndices, 1.0, 5.0, 4.0, hasDx);
}

Workspace2D_sptr maskSpectra(Workspace2D_sptr workspace,
                             const std::set<int64_t> &maskedWorkspaceIndices) {
  const int nhist = static_cast<int>(workspace->getNumberHistograms());
  if (workspace->getInstrument()->nelements() == 0) {
    // We need detectors to be able to mask them.
    auto instrument = boost::make_shared<Instrument>();
    workspace->setInstrument(instrument);

    std::string xmlShape = "<sphere id=\"shape\"> ";
    xmlShape += R"(<centre x="0.0"  y="0.0" z="0.0" /> )";
    xmlShape += "<radius val=\"0.05\" /> ";
    xmlShape += "</sphere>";
    xmlShape += "<algebra val=\"shape\" /> ";

    ShapeFactory sFactory;
    auto shape = sFactory.createShape(xmlShape);
    for (int i = 0; i < nhist; ++i) {
      Detector *det = new Detector("det", detid_t(i + 1), shape, nullptr);
      det->setPos(i, i + 1, 1);
      instrument->add(det);
      instrument->markAsDetector(det);
    }
    workspace->setInstrument(instrument);
    // Set IndexInfo without explicit spectrum definitions to trigger building
    // default mapping of spectra to detectors in new instrument.
    workspace->setIndexInfo(Indexing::IndexInfo(nhist));
  }

  auto &spectrumInfo = workspace->mutableSpectrumInfo();
  for (const auto index : maskedWorkspaceIndices)
    spectrumInfo.setMasked(index, true);
  return workspace;
}

/**
 * Create a group with nEntries. It is added to the ADS with the given stem
 */
WorkspaceGroup_sptr createWorkspaceGroup(int nEntries, int nHist, int nBins,
                                         const std::string &stem) {
  auto group = boost::make_shared<WorkspaceGroup>();
  AnalysisDataService::Instance().add(stem, group);
  for (int i = 0; i < nEntries; ++i) {
    Workspace2D_sptr ws = create2DWorkspace(nHist, nBins);
    std::ostringstream os;
    os << stem << "_" << i;
    AnalysisDataService::Instance().add(os.str(), ws);
    group->add(os.str());
  }
  return group;
}

/** Create a 2D workspace with this many histograms and bins.
 * Filled with Y = 2.0 and E = M_SQRT2w
 */
Workspace2D_sptr create2DWorkspaceBinned(size_t nhist, size_t numVals,
                                         double x0, double deltax) {
  BinEdges x(numVals + 1, LinearGenerator(x0, deltax));
  Counts y(numVals, 2);
  CountStandardDeviations e(numVals, M_SQRT2);
  return create<Workspace2D>(nhist, Histogram(x, y, e));
}

/** Create a 2D workspace with this many histograms and bins. The bins are
 * assumed to be non-uniform and given by the input array
 * Filled with Y = 2.0 and E = M_SQRT2w
 * If hasDx is true, all spectra will have dx values, starting from 0.1 and
 * increased by 0.1 for each bin.
 */
Workspace2D_sptr create2DWorkspaceNonUniformlyBinned(int nhist,
                                                     const int numBoundaries,
                                                     const double xBoundaries[],
                                                     bool hasDx) {
  BinEdges x(xBoundaries, xBoundaries + numBoundaries);
  const int numBins = numBoundaries - 1;
  Counts y(numBins, 2);
  CountStandardDeviations e(numBins, M_SQRT2);
  auto dx = Kernel::make_cow<HistogramData::HistogramDx>(
      numBins, LinearGenerator(0.1, .1));
  auto retVal = boost::make_shared<Workspace2D>();
  retVal->initialize(nhist, createHisto(true, y, e));
  for (int i = 0; i < nhist; i++) {
    retVal->setBinEdges(i, x);
    if (hasDx)
      retVal->setSharedDx(i, dx);
  }
  return retVal;
}

/**
 * Add random noise to the signal
 * @param ws :: The workspace to add the noise to
 * @param noise :: The mean noise level
 * @param lower :: The lower bound of the flucation (default=-0.5)
 * @param upper:: The upper bound of the flucation (default=-0.5)
 */
void addNoise(Mantid::API::MatrixWorkspace_sptr ws, double noise,
              const double lower, const double upper) {
  const size_t seed(12345);
  MersenneTwister randGen(seed, lower, upper);
  for (size_t iSpec = 0; iSpec < ws->getNumberHistograms(); iSpec++) {
    auto &mutableY = ws->mutableY(iSpec);
    auto &mutableE = ws->mutableE(iSpec);
    for (size_t i = 0; i < mutableY.size(); i++) {
      mutableY[i] += noise * randGen.nextValue();
      mutableE[i] += noise;
    }
  }
}

/**
 * Each spectra will have a cylindrical detector defined 2*cylinder_radius away
 * from the centre of the previous.
 * Data filled with: Y: 2.0, E: M_SQRT2, X: nbins of width 1 starting at 0
 * The flag hasDx is responsible for creating dx values or not
 */
Workspace2D_sptr create2DWorkspaceWithFullInstrument(
    int nhist, int nbins, bool includeMonitors, bool startYNegative,
    bool isHistogram, const std::string &instrumentName, bool hasDx) {
  if (includeMonitors && nhist < 2) {
    throw std::invalid_argument("Attempting to 2 include monitors for a "
                                "workspace with fewer than 2 histograms");
  }

  Workspace2D_sptr space;
  // A 1:1 spectra is created by default
  if (isHistogram)
    space = create2DWorkspaceBinned(nhist, nbins, hasDx);
  else
    space =
        create2DWorkspace123(nhist, nbins, false, std::set<int64_t>(), hasDx);
  // actually adds a property called run_title to the logs
  space->setTitle("Test histogram");
  space->getAxis(0)->setUnit("TOF");
  space->setYUnit("Counts");

  InstrumentCreationHelper::addFullInstrumentToWorkspace(
      *space, includeMonitors, startYNegative, instrumentName);

  return space;
}

//================================================================================================================
/*
 * startTime is in seconds
 */
MatrixWorkspace_sptr create2DDetectorScanWorkspaceWithFullInstrument(
    int nhist, int nbins, size_t nTimeIndexes, size_t startTime,
    size_t firstInterval, bool includeMonitors, bool startYNegative,
    bool isHistogram, const std::string &instrumentName) {

  auto baseWS = create2DWorkspaceWithFullInstrument(
      nhist, nbins, includeMonitors, startYNegative, isHistogram,
      instrumentName);

  auto builder =
      ScanningWorkspaceBuilder(baseWS->getInstrument(), nTimeIndexes, nbins);

  std::vector<double> timeRanges;
  for (size_t i = 0; i < nTimeIndexes; ++i) {
    timeRanges.push_back(double(i + firstInterval));
  }

  builder.setTimeRanges(DateAndTime(int(startTime), 0), timeRanges);

  return builder.buildWorkspace();
}

//================================================================================================================
/** Create an Workspace2D with an instrument that contains
 *RectangularDetector's.
 * Bins will be 0.0, 1.0, to numBins, filled with signal=2.0, M_SQRT2
 *
 * @param numBanks :: number of rectangular banks
 * @param numPixels :: each bank will be numPixels*numPixels
 * @param numBins :: each spectrum will have this # of bins
 * @return The Workspace2D
 */
Mantid::DataObjects::Workspace2D_sptr
create2DWorkspaceWithRectangularInstrument(int numBanks, int numPixels,
                                           int numBins) {
  Instrument_sptr inst =
      ComponentCreationHelper::createTestInstrumentRectangular(numBanks,
                                                               numPixels);
  Workspace2D_sptr ws =
      create2DWorkspaceBinned(numBanks * numPixels * numPixels, numBins);
  ws->setInstrument(inst);
  ws->getAxis(0)->setUnit("dSpacing");
  for (size_t wi = 0; wi < ws->getNumberHistograms(); wi++) {
    ws->getSpectrum(wi).setDetectorID(detid_t(numPixels * numPixels + wi));
    ws->getSpectrum(wi).setSpectrumNo(specnum_t(wi));
  }

  return ws;
}

//================================================================================================================
/** Create an Eventworkspace with an instrument that contains
 *RectangularDetector's.
 * X axis = 100 histogrammed bins from 0.0 in steps of 1.0.
 * 200 events per pixel.
 *
 * @param numBanks :: number of rectangular banks
 * @param numPixels :: each bank will be numPixels*numPixels
 * @param clearEvents :: if true, erase the events from list
 * @return The EventWorkspace
 */
Mantid::DataObjects::EventWorkspace_sptr
createEventWorkspaceWithFullInstrument(int numBanks, int numPixels,
                                       bool clearEvents) {
  Instrument_sptr inst =
      ComponentCreationHelper::createTestInstrumentRectangular(numBanks,
                                                               numPixels);
  EventWorkspace_sptr ws =
      createEventWorkspace2(numBanks * numPixels * numPixels, 100);
  ws->setInstrument(inst);

  // Set the X axes
  const auto &xVals = ws->x(0);
  const size_t xSize = xVals.size();
  auto ax0 = std::make_unique<NumericAxis>(xSize);
  ax0->setUnit("dSpacing");
  for (size_t i = 0; i < xSize; i++) {
    ax0->setValue(i, xVals[i]);
  }
  ws->replaceAxis(0, std::move(ax0));

  // re-assign detector IDs to the rectangular detector
  const auto detIds = inst->getDetectorIDs();
  for (int wi = 0; wi < static_cast<int>(ws->getNumberHistograms()); ++wi) {
    ws->getSpectrum(wi).clearDetectorIDs();
    if (clearEvents)
      ws->getSpectrum(wi).clear(true);
    ws->getSpectrum(wi).setDetectorID(detIds[wi]);
  }
  return ws;
}

Mantid::DataObjects::EventWorkspace_sptr
createEventWorkspaceWithNonUniformInstrument(int numBanks, bool clearEvents) {
  // Number of detectors in a bank as created by createTestInstrumentCylindrical
  const int DETECTORS_PER_BANK(9);

  V3D srcPos(0., 0., -10.), samplePos;
  Instrument_sptr inst =
      ComponentCreationHelper::createTestInstrumentCylindrical(
          numBanks, srcPos, samplePos, 0.0025, 0.005);
  EventWorkspace_sptr ws =
      createEventWorkspace2(numBanks * DETECTORS_PER_BANK, 100);
  ws->setInstrument(inst);

  std::vector<detid_t> detectorIds = inst->getDetectorIDs();

  // Should be equal if DETECTORS_PER_BANK is correct
  assert(detectorIds.size() == ws->getNumberHistograms());

  // Re-assign detector IDs
  for (size_t wi = 0; wi < ws->getNumberHistograms(); wi++) {
    ws->getSpectrum(wi).clearDetectorIDs();
    if (clearEvents)
      ws->getSpectrum(wi).clear(true);
    ws->getSpectrum(wi).setDetectorID(detectorIds[wi]);
  }

  return ws;
}

/** Creates a binned 2DWorkspace with title and TOF x-axis and counts y-axis
 *
 * @param startX :: start TOF x-value
 * @param nSpectra :: number of spectra
 * @param nBins :: number of bins
 * @param deltaX :: TOF delta x-value
 * @return a Workspace2D
 */
DataObjects::Workspace2D_sptr reflectometryWorkspace(const double startX,
                                                     const int nSpectra,
                                                     const int nBins,
                                                     const double deltaX) {

  auto workspace = create2DWorkspaceBinned(nSpectra, nBins, startX, deltaX);

  workspace->setTitle(
      "Test histogram"); // actually adds a property call run_title to the logs
  workspace->getAxis(0)->setUnit("TOF");
  workspace->setYUnit("Counts");
  return workspace;
}

/**
 * Create a very small 2D workspace for a virtual reflectometry instrument.
 * @return workspace with instrument attached.
 * @param startX : X Tof start value for the workspace.
 * @param slit1Pos :: slit 1 position
 * @param slit2Pos :: slit 2 position
 * @param vg1 :: vertical gap slit 1
 * @param vg2 :: vertical gap slit 2
 * @param sourcePos :: source position
 * @param monitorPos :: monitor position
 * @param samplePos :: sample position
 * @param detectorPos :: detector position
 * @param nBins :: number of bins
 * @param deltaX :: TOF delta x-value
 */
MatrixWorkspace_sptr create2DWorkspaceWithReflectometryInstrument(
    const double startX, const V3D &slit1Pos, const V3D &slit2Pos,
    const double vg1, const double vg2, const V3D &sourcePos,
    const V3D &monitorPos, const V3D &samplePos, const V3D &detectorPos,
    const int nBins, const double deltaX) {
  Instrument_sptr instrument = boost::make_shared<Instrument>();
  instrument->setReferenceFrame(boost::make_shared<ReferenceFrame>(
      PointingAlong::Y, PointingAlong::X, Handedness::Left, "0,0,0"));

  InstrumentCreationHelper::addSource(instrument, sourcePos, "source");
  InstrumentCreationHelper::addMonitor(instrument, monitorPos, 1, "Monitor");
  InstrumentCreationHelper::addSample(instrument, samplePos,
                                      "some-surface-holder");
  InstrumentCreationHelper::addDetector(instrument, detectorPos, 2,
                                        "point-detector");
  auto slit1 =
      InstrumentCreationHelper::addComponent(instrument, slit1Pos, "slit1");
  auto slit2 =
      InstrumentCreationHelper::addComponent(instrument, slit2Pos, "slit2");

  auto workspace = reflectometryWorkspace(startX, 2, nBins, deltaX);
  workspace->setInstrument(instrument);

  ParameterMap &pmap = workspace->instrumentParameters();
  pmap.addDouble(slit1, "vertical gap", vg1);
  pmap.addDouble(slit2, "vertical gap", vg2);

  workspace->getSpectrum(0).setDetectorID(2);
  workspace->getSpectrum(1).setDetectorID(1);

  return workspace;
}

/**
 * Create a very small 2D workspace for a virtual reflectometry instrument with
 * multiple detectors
 * @return workspace with instrument attached.
 * @param startX :: X Tof start value for the workspace.
 * @param detSize :: detector height
 * @param slit1Pos :: position of the first slit (counting from source)
 * @param slit2Pos :: position of the second slit (counting from source)
 * @param vg1 :: slit 1 vertical gap
 * @param vg2 :: slit 2 vertical gap
 * @param sourcePos :: source position
 * @param monitorPos :: monitor position
 * @param samplePos :: sample position
 * @param detectorCenterPos :: position of the detector center
 * @param nSpectra :: number of spectra (detectors + monitor)
 * @param nBins :: number of TOF channels
 * @param deltaX :: TOF channel width
 */
MatrixWorkspace_sptr create2DWorkspaceWithReflectometryInstrumentMultiDetector(
    const double startX, const double detSize, const V3D &slit1Pos,
    const V3D &slit2Pos, const double vg1, const double vg2,
    const V3D &sourcePos, const V3D &monitorPos, const V3D &samplePos,
    const V3D &detectorCenterPos, const int nSpectra, const int nBins,
    const double deltaX) {
  Instrument_sptr instrument = boost::make_shared<Instrument>();
  instrument->setReferenceFrame(boost::make_shared<ReferenceFrame>(
      PointingAlong::Y /*up*/, PointingAlong::X /*along*/, Handedness::Left,
      "0,0,0"));

  InstrumentCreationHelper::addSource(instrument, sourcePos, "source");
  InstrumentCreationHelper::addSample(instrument, samplePos,
                                      "some-surface-holder");
  InstrumentCreationHelper::addMonitor(instrument, monitorPos, 1, "Monitor");

  const int nDet = nSpectra - 1;
  const double minY = detectorCenterPos.Y() - detSize * (nDet - 1) / 2.;
  for (int i = 0; i < nDet; ++i) {
    const double y = minY + i * detSize;
    const V3D pos{detectorCenterPos.X(), y, detectorCenterPos.Z()};
    InstrumentCreationHelper::addDetector(instrument, pos, i + 2,
                                          "point-detector");
  }
  auto slit1 =
      InstrumentCreationHelper::addComponent(instrument, slit1Pos, "slit1");
  auto slit2 =
      InstrumentCreationHelper::addComponent(instrument, slit2Pos, "slit2");

  auto workspace = reflectometryWorkspace(startX, nSpectra, nBins, deltaX);
  workspace->setInstrument(instrument);
  ParameterMap &pmap = workspace->instrumentParameters();
  pmap.addDouble(slit1, "vertical gap", vg1);
  pmap.addDouble(slit2, "vertical gap", vg2);
  for (int i = 0; i < nSpectra; ++i) {
    workspace->getSpectrum(i).setDetectorID(i + 1);
  }
  return workspace;
}

void createInstrumentForWorkspaceWithDistances(
    MatrixWorkspace_sptr workspace, const V3D &samplePosition,
    const V3D &sourcePosition, const std::vector<V3D> &detectorPositions) {
  Instrument_sptr instrument = boost::make_shared<Instrument>();
  instrument->setReferenceFrame(
      boost::make_shared<ReferenceFrame>(Y, X, Left, "0,0,0"));

  InstrumentCreationHelper::addSource(instrument, sourcePosition, "source");
  InstrumentCreationHelper::addSample(instrument, samplePosition, "sample");

  for (int i = 0; i < static_cast<int>(detectorPositions.size()); ++i) {
    std::stringstream buffer;
    buffer << "detector_" << i;
    InstrumentCreationHelper::addDetector(instrument, detectorPositions[i], i,
                                          buffer.str());

    // Link it to the workspace
    workspace->getSpectrum(i).addDetectorID(i);
  }
  workspace->setInstrument(instrument);
}

//================================================================================================================
WorkspaceSingleValue_sptr createWorkspaceSingleValue(double value) {
  return boost::make_shared<WorkspaceSingleValue>(value, sqrt(value));
}

WorkspaceSingleValue_sptr createWorkspaceSingleValueWithError(double value,
                                                              double error) {
  return boost::make_shared<WorkspaceSingleValue>(value, error);
}

/** Perform some finalization on event workspace stuff */
void eventWorkspace_Finalize(EventWorkspace_sptr ew) {
  // get a proton charge
  ew->mutableRun().integrateProtonCharge();
}

/** Create event workspace with:
 * 500 pixels
 * 1000 histogrammed bins.
 */
EventWorkspace_sptr createEventWorkspace() {
  return createEventWorkspace(500, 1001, 100, 1000);
}

/** Create event workspace with:
 * numPixels pixels
 * numBins histogrammed bins from 0.0 in steps of 1.0
 * 200 events; two in each bin, at time 0.5, 1.5, etc.
 * PulseTime = 0 second x2, 1 second x2, 2 seconds x2, etc. after 2010-01-01
 */
EventWorkspace_sptr createEventWorkspace2(int numPixels, int numBins) {
  return createEventWorkspace(numPixels, numBins, 100, 0.0, 1.0, 2);
}

/** Create event workspace
 */
EventWorkspace_sptr createEventWorkspace(int numPixels, int numBins,
                                         int numEvents, double x0,
                                         double binDelta, int eventPattern,
                                         int start_at_pixelID) {
  return createEventWorkspaceWithStartTime(
      numPixels, numBins, numEvents, x0, binDelta, eventPattern,
      start_at_pixelID, DateAndTime("2010-01-01T00:00:00"));
}

/**
 * Create event workspace with defined start date time
 */
EventWorkspace_sptr
createEventWorkspaceWithStartTime(int numPixels, int numBins, int numEvents,
                                  double x0, double binDelta, int eventPattern,
                                  int start_at_pixelID, DateAndTime run_start) {

  // add one to the number of bins as this is histogram
  numBins++;

  auto retVal = boost::make_shared<EventWorkspace>();
  retVal->initialize(numPixels, 1, 1);

  // Make fake events
  if (eventPattern) // 0 == no events
  {
    size_t workspaceIndex = 0;
    for (int pix = start_at_pixelID + 0; pix < start_at_pixelID + numPixels;
         pix++) {
      EventList &el = retVal->getSpectrum(workspaceIndex);
      el.setSpectrumNo(pix);
      el.setDetectorID(pix);

      for (int i = 0; i < numEvents; i++) {
        if (eventPattern == 1) // 0, 1 diagonal pattern
          el += TofEvent((pix + i + 0.5) * binDelta, run_start + double(i));
        else if (eventPattern == 2) // solid 2
        {
          el += TofEvent((i + 0.5) * binDelta, run_start + double(i));
          el += TofEvent((i + 0.5) * binDelta, run_start + double(i));
        } else if (eventPattern == 3) // solid 1
        {
          el += TofEvent((i + 0.5) * binDelta, run_start + double(i));
        } else if (eventPattern == 4) // Number of events per bin = pixelId (aka
                                      // workspace index in most cases)
        {
          for (int q = 0; q < pix; q++)
            el += TofEvent((i + 0.5) * binDelta, run_start + double(i));
        }
      }
      workspaceIndex++;
    }
  }

  retVal->setAllX(BinEdges(numBins, LinearGenerator(x0, binDelta)));

  return retVal;
}

// =====================================================================================
/** Create event workspace, with several detector IDs in one event list.
 */
EventWorkspace_sptr
createGroupedEventWorkspace(std::vector<std::vector<int>> groups, int numBins,
                            double binDelta, double xOffset) {

  auto retVal = boost::make_shared<EventWorkspace>();
  retVal->initialize(groups.size(), 2, 1);

  for (size_t g = 0; g < groups.size(); g++) {
    retVal->getSpectrum(g).clearDetectorIDs();
    std::vector<int> dets = groups[g];
    for (auto det : dets) {
      for (int i = 0; i < numBins; i++)
        retVal->getSpectrum(g) += TofEvent((i + 0.5) * binDelta, 1);
      retVal->getSpectrum(g).addDetectorID(det);
    }
  }

  if (xOffset == 0.) {
    retVal->setAllX(BinEdges(numBins, LinearGenerator(0.0, binDelta)));
  } else {
    for (size_t g = 0; g < groups.size(); g++) {
      // Create the x-axis for histogramming.
      const double x0 = xOffset * static_cast<double>(g);
      retVal->setX(
          g, make_cow<HistogramX>(numBins, LinearGenerator(x0, binDelta)));
    }
  }

  return retVal;
}

// =====================================================================================
/** Create an event workspace with randomized TOF and pulsetimes
 *
 * @param numbins :: # of bins to set. This is also = # of events per EventList
 * @param numpixels :: number of pixels
 * @param bin_delta :: a constant offset to shift the bin bounds by
 * @return EventWorkspace
 */
EventWorkspace_sptr createRandomEventWorkspace(size_t numbins, size_t numpixels,
                                               double bin_delta) {
  auto retVal = boost::make_shared<EventWorkspace>();
  retVal->initialize(numpixels, numbins, numbins - 1);

  // and X-axis for references:
  auto pAxis0 = std::make_unique<NumericAxis>(numbins);
  // Create the original X axis to histogram on.
  // Create the x-axis for histogramming.
  HistogramData::BinEdges axis(numbins, LinearGenerator(0.0, bin_delta));
  for (int i = 0; i < static_cast<int>(numbins); ++i) {
    pAxis0->setValue(i, axis[i]);
  }
  pAxis0->setUnit("TOF");

  MersenneTwister randomGen(DateAndTime::getCurrentTime().nanoseconds(), 0,
                            std::numeric_limits<int>::max());
  // Make up some data for each pixels
  for (size_t i = 0; i < numpixels; i++) {
    // Create one event for each bin
    EventList &events = retVal->getSpectrum(static_cast<detid_t>(i));
    for (std::size_t ie = 0; ie < numbins; ie++) {
      // Create a list of events, randomize
      events += TofEvent(static_cast<double>(randomGen.nextValue()),
                         static_cast<int64_t>(randomGen.nextValue()));
    }
    events.addDetectorID(detid_t(i));
  }
  retVal->setAllX(axis);
  retVal->replaceAxis(0, std::move(pAxis0));

  return retVal;
}

// =====================================================================================
/** Create Workspace2d, with numHist spectra, each with 9 detectors,
 * with IDs 1-9, 10-18, 19-27
 */
MatrixWorkspace_sptr createGroupedWorkspace2D(size_t numHist, int numBins,
                                              double binDelta) {
  Workspace2D_sptr retVal = create2DWorkspaceBinned(static_cast<int>(numHist),
                                                    numBins, 0.0, binDelta);
  retVal->setInstrument(
      ComponentCreationHelper::createTestInstrumentCylindrical(
          static_cast<int>(numHist)));

  for (int g = 0; g < static_cast<int>(numHist); g++) {
    auto &spec = retVal->getSpectrum(g);
    for (int i = 1; i <= 9; i++)
      spec.addDetectorID(g * 9 + i);
    spec.setSpectrumNo(g + 1); // Match detector ID and spec NO
  }
  return boost::dynamic_pointer_cast<MatrixWorkspace>(retVal);
}

// =====================================================================================
// RootOfNumHist == square root of hystohram number;
MatrixWorkspace_sptr
createGroupedWorkspace2DWithRingsAndBoxes(size_t RootOfNumHist, int numBins,
                                          double binDelta) {
  size_t numHist = RootOfNumHist * RootOfNumHist;
  Workspace2D_sptr retVal = create2DWorkspaceBinned(static_cast<int>(numHist),
                                                    numBins, 0.0, binDelta);
  retVal->setInstrument(
      ComponentCreationHelper::createTestInstrumentCylindrical(
          static_cast<int>(numHist)));
  for (int g = 0; g < static_cast<int>(numHist); g++) {
    auto &spec = retVal->getSpectrum(g);
    spec.addDetectorID(
        g + 1); // Legacy comptibilty: Used to be default IDs in Workspace2D.
    for (int i = 1; i <= 9; i++)
      spec.addDetectorID(g * 9 + i);
    spec.setSpectrumNo(g + 1); // Match detector ID and spec NO
  }
  return boost::dynamic_pointer_cast<MatrixWorkspace>(retVal);
}

// not strictly creating a workspace, but really helpful to see what one
// contains
void displayDataY(MatrixWorkspace_const_sptr ws) {
  const size_t numHists = ws->getNumberHistograms();
  for (size_t i = 0; i < numHists; ++i) {
    std::cout << "Histogram " << i << " = ";
    const auto &y = ws->y(i);
    for (size_t j = 0; j < y.size(); ++j) {
      std::cout << y[j] << " ";
    }
    std::cout << '\n';
  }
}
void displayData(MatrixWorkspace_const_sptr ws) { displayDataX(ws); }

// not strictly creating a workspace, but really helpful to see what one
// contains
void displayDataX(MatrixWorkspace_const_sptr ws) {
  const size_t numHists = ws->getNumberHistograms();
  for (size_t i = 0; i < numHists; ++i) {
    std::cout << "Histogram " << i << " = ";
    const auto &x = ws->x(i);
    for (size_t j = 0; j < x.size(); ++j) {
      std::cout << x[j] << " ";
    }
    std::cout << '\n';
  }
}

// not strictly creating a workspace, but really helpful to see what one
// contains
void displayDataE(MatrixWorkspace_const_sptr ws) {
  const size_t numHists = ws->getNumberHistograms();
  for (size_t i = 0; i < numHists; ++i) {
    std::cout << "Histogram " << i << " = ";
    const auto &e = ws->e(i);
    for (size_t j = 0; j < e.size(); ++j) {
      std::cout << e[j] << " ";
    }
    std::cout << '\n';
  }
}

// =====================================================================================
/** Utility function to add a TimeSeriesProperty with a name and value
 *
 * @param runInfo :: Run to add to
 * @param name :: property name
 * @param val :: value
 */
void addTSPEntry(Run &runInfo, std::string name, double val) {
  TimeSeriesProperty<double> *tsp;
  tsp = new TimeSeriesProperty<double>(name);
  tsp->addValue("2011-05-24T00:00:00", val);
  runInfo.addProperty(tsp);
}

// =====================================================================================
/** Sets the OrientedLattice in the crystal as an crystal with given lattice
 *lengths, angles of 90 deg
 *
 * @param ws :: workspace to set
 * @param a :: lattice length
 * @param b :: lattice length
 * @param c :: lattice length
 */
void setOrientedLattice(Mantid::API::MatrixWorkspace_sptr ws, double a,
                        double b, double c) {
  auto latt = std::make_unique<OrientedLattice>(a, b, c, 90., 90., 90.);
  ws->mutableSample().setOrientedLattice(latt.release());
}

// =====================================================================================
/** Create a default universal goniometer and set its angles
 *
 * @param ws :: workspace to set
 * @param phi :: +Y rotation angle (deg)
 * @param chi :: +X rotation angle (deg)
 * @param omega :: +Y rotation angle (deg)
 */
void setGoniometer(Mantid::API::MatrixWorkspace_sptr ws, double phi, double chi,
                   double omega) {
  addTSPEntry(ws->mutableRun(), "phi", phi);
  addTSPEntry(ws->mutableRun(), "chi", chi);
  addTSPEntry(ws->mutableRun(), "omega", omega);
  Mantid::Geometry::Goniometer gm;
  gm.makeUniversalGoniometer();
  ws->mutableRun().setGoniometer(gm, true);
}

//
Mantid::API::MatrixWorkspace_sptr
createProcessedWorkspaceWithCylComplexInstrument(size_t numPixels,
                                                 size_t numBins,
                                                 bool has_oriented_lattice) {
  size_t rHist = static_cast<size_t>(std::sqrt(static_cast<double>(numPixels)));
  while (rHist * rHist < numPixels)
    rHist++;

  Mantid::API::MatrixWorkspace_sptr ws =
      createGroupedWorkspace2DWithRingsAndBoxes(rHist, 10, 0.1);
  auto pAxis0 = std::make_unique<NumericAxis>(numBins);
  for (size_t i = 0; i < numBins; i++) {
    double dE = -1.0 + static_cast<double>(i) * 0.8;
    pAxis0->setValue(i, dE);
  }
  pAxis0->setUnit("DeltaE");
  ws->replaceAxis(0, std::move(pAxis0));
  if (has_oriented_lattice) {
    auto latt = std::make_unique<OrientedLattice>(1, 1, 1, 90., 90., 90.);
    ws->mutableSample().setOrientedLattice(latt.release());

    addTSPEntry(ws->mutableRun(), "phi", 0);
    addTSPEntry(ws->mutableRun(), "chi", 0);
    addTSPEntry(ws->mutableRun(), "omega", 0);
    Mantid::Geometry::Goniometer gm;
    gm.makeUniversalGoniometer();
    ws->mutableRun().setGoniometer(gm, true);
  }

  return ws;
}

/// Create a workspace with all components needed for inelastic analysis and 3
/// detectors in specific places
/// @param L2        -- the sample to detector flight path
/// @param polar     -- the detector polar angle
/// @param azimutal  -- the detector azimuthal
/// @param numBins   -- the number of histogram bins for the workspace
/// @param Emin      -- minimal energy transfer
/// @param Emax      -- maxinal energy transfer
/// @param Ei        -- input beam energy
Mantid::API::MatrixWorkspace_sptr
createProcessedInelasticWS(const std::vector<double> &L2,
                           const std::vector<double> &polar,
                           const std::vector<double> &azimutal, size_t numBins,
                           double Emin, double Emax, double Ei) {
  // not used but interface needs it
  std::set<int64_t> maskedWorkspaceIndices;
  size_t numPixels = L2.size();

  Mantid::API::MatrixWorkspace_sptr ws =
      create2DWorkspaceWithValues(uint64_t(numPixels), uint64_t(numBins), true,
                                  maskedWorkspaceIndices, 0, 1, 0.1);

  // detectors at L2, sample at 0 and source at -L2_min
  ws->setInstrument(
      ComponentCreationHelper::createCylInstrumentWithDetInGivenPositions(
          L2, polar, azimutal));

  for (int g = 0; g < static_cast<int>(numPixels); g++) {
    auto &spec = ws->getSpectrum(g);
    // we just made (in createCylInstrumentWithDetInGivenPosisions) det ID-s to
    // start from 1
    spec.setDetectorID(g + 1);
    // and this is absolutely different nummer, corresponding to det ID just by
    // chance ? -- some uncertainties remain
    spec.setSpectrumNo(g + 1);
    // spec->setSpectrumNo(g+1);
    //   spec->addDetectorID(g*9);
    //   spec->setSpectrumNo(g+1); // Match detector ID and spec NO
  }

  const double dE = (Emax - Emin) / static_cast<double>(numBins);
  for (size_t j = 0; j < numPixels; j++) {
    std::vector<double> E_transfer;
    E_transfer.reserve(numBins);
    for (size_t i = 0; i <= numBins; i++) {
      E_transfer.push_back(Emin + static_cast<double>(i) * dE);
    }
    ws->mutableX(j) = std::move(E_transfer);
  }

  // set axis, correspondent to the X-values
  auto pAxis0 = std::make_unique<NumericAxis>(numBins);
  const auto &E_transfer = ws->x(0);
  for (size_t i = 0; i < numBins; i++) {
    double E = 0.5 * (E_transfer[i] + E_transfer[i + 1]);
    pAxis0->setValue(i, E);
  }

  pAxis0->setUnit("DeltaE");

  ws->replaceAxis(0, std::move(pAxis0));

  // define oriented lattice which requested for processed ws
  auto latt = std::make_unique<OrientedLattice>(1, 1, 1, 90., 90., 90.);
  ws->mutableSample().setOrientedLattice(latt.release());

  ws->mutableRun().addProperty(
      new PropertyWithValue<std::string>("deltaE-mode", "Direct"), true);
  ws->mutableRun().addProperty(new PropertyWithValue<double>("Ei", Ei), true);
  // these properties have to be different -> specific for processed ws, as time
  // now should be reconciled
  addTSPEntry(ws->mutableRun(), "phi", 0);
  addTSPEntry(ws->mutableRun(), "chi", 0);
  addTSPEntry(ws->mutableRun(), "omega", 0);
  Mantid::Geometry::Goniometer gm;
  gm.makeUniversalGoniometer();
  ws->mutableRun().setGoniometer(gm, true);

  return ws;
}

/*
 * Create an EventWorkspace from a source EventWorkspace.
 * The new workspace should be exactly the same as the source workspace but
 * without any events
 */
Mantid::DataObjects::EventWorkspace_sptr
createEventWorkspace3(Mantid::DataObjects::EventWorkspace_const_sptr sourceWS,
                      std::string wsname, API::Algorithm *alg) {
  UNUSED_ARG(wsname);
  // 1. Initialize:use dummy numbers for arguments, for event workspace it
  // doesn't matter
  Mantid::DataObjects::EventWorkspace_sptr outputWS =
      Mantid::DataObjects::EventWorkspace_sptr(
          new DataObjects::EventWorkspace());
  outputWS->initialize(sourceWS->getInstrument()->getDetectorIDs(true).size(),
                       1, 1);

  // 2. Set the units
  outputWS->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
  outputWS->setYUnit("Counts");
  outputWS->setTitle("Empty_Title");

  // 3. Add the run_start property:
  int runnumber = sourceWS->getRunNumber();
  outputWS->mutableRun().addProperty("run_number", runnumber);

  std::string runstartstr = sourceWS->run().getProperty("run_start")->value();
  outputWS->mutableRun().addProperty("run_start", runstartstr);

  // 4. Instrument
  Mantid::API::Algorithm_sptr loadInst =
      alg->createChildAlgorithm("LoadInstrument");
  // Now execute the Child Algorithm. Catch and log any error, but don't stop.
  loadInst->setPropertyValue("InstrumentName",
                             sourceWS->getInstrument()->getName());
  loadInst->setProperty<MatrixWorkspace_sptr>("Workspace", outputWS);
  loadInst->setProperty("RewriteSpectraMap",
                        Mantid::Kernel::OptionalBool(true));
  loadInst->executeAsChildAlg();
  // Populate the instrument parameters in this workspace - this works around a
  // bug
  outputWS->populateInstrumentParameters();

  // 6. Build spectrum and event list
  // a) We want to pad out empty pixels.
  detid2det_map detector_map;
  outputWS->getInstrument()->getDetectors(detector_map);

  // b) determine maximum pixel id
  detid2det_map::iterator it;
  detid_t detid_max = 0; // seems like a safe lower bound
  for (it = detector_map.begin(); it != detector_map.end(); ++it)
    if (it->first > detid_max)
      detid_max = it->first;

  // c) Pad all the pixels and Set to zero
  size_t workspaceIndex = 0;
  const auto &detectorInfo = outputWS->detectorInfo();
  for (it = detector_map.begin(); it != detector_map.end(); ++it) {
    if (!detectorInfo.isMonitor(detectorInfo.indexOf(it->first))) {
      auto &spec = outputWS->getSpectrum(workspaceIndex);
      spec.addDetectorID(it->first);
      // Start the spectrum number at 1
      spec.setSpectrumNo(specnum_t(workspaceIndex + 1));
      workspaceIndex += 1;
    }
  }

  return outputWS;
}

RebinnedOutput_sptr createRebinnedOutputWorkspace() {
  RebinnedOutput_sptr outputWS =
      Mantid::DataObjects::RebinnedOutput_sptr(new RebinnedOutput());
  // outputWS->setName("rebinTest");
  Mantid::API::AnalysisDataService::Instance().add("rebinTest", outputWS);

  // Set Q ('y') axis binning
  std::vector<double> qbins{0.0, 1.0, 4.0};
  std::vector<double> qaxis;
  const int numY =
      static_cast<int>(VectorHelper::createAxisFromRebinParams(qbins, qaxis));

  // Initialize the workspace
  const int numHist = numY - 1;
  const int numX = 7;
  outputWS->initialize(numHist, numX, numX - 1);

  // Set the normal units
  outputWS->getAxis(0)->unit() = UnitFactory::Instance().create("DeltaE");
  outputWS->setYUnit("Counts");
  outputWS->setTitle("Empty_Title");

  // Create the i-axis for histogramming.
  HistogramData::BinEdges x1{-3.0, -2.0, -1.0, 0.0, 1.0, 2.0, 3.0};

  // Create a numeric axis to replace the default vertical one
  auto verticalAxis = std::make_unique<NumericAxis>(numY);


  // Now set the axis values
  for (int i = 0; i < numHist; ++i) {
    outputWS->setBinEdges(i, x1);
    verticalAxis->setValue(i, qaxis[i]);
  }
  // One more to set on the 'y' axis
  verticalAxis->setValue(numHist, qaxis[numHist]);

  // Set the 'y' axis units
  verticalAxis->unit() = UnitFactory::Instance().create("MomentumTransfer");
  verticalAxis->title() = "|Q|";
    outputWS->replaceAxis(1, std::move(verticalAxis));

  // Set the X axis title (for conversion to MD)
  outputWS->getAxis(0)->title() = "Energy transfer";

  // Now, setup the data
  // Q bin #1

  // Populates destination starting at index 1 with the following data
  // e.g. y(0)[1] = 2.0, y(0)[2] = 3.0 ..etc. as the starting index is 1
  // if you change the values in the line below please update this comment!
  populateWsWithInitList(outputWS->mutableY(0), 1, {2.0, 3.0, 3.0, 2.0});
  populateWsWithInitList(outputWS->mutableE(0), 1, {2.0, 3.0, 3.0, 2.0});
  populateWsWithInitList(outputWS->dataF(0), 1, {2.0, 3.0, 3.0, 1.0});

  // Q bin #2
  populateWsWithInitList(outputWS->mutableY(1), 1, {1.0, 3.0, 3.0, 2.0, 2.0});
  populateWsWithInitList(outputWS->mutableE(1), 1, {1.0, 3.0, 3.0, 2.0, 2.0});
  populateWsWithInitList(outputWS->dataF(1), 1, {1.0, 3.0, 3.0, 1.0, 2.0});

  // Q bin #3
  populateWsWithInitList(outputWS->mutableY(2), 1, {1.0, 2.0, 3.0, 1.0});
  populateWsWithInitList(outputWS->mutableE(2), 1, {1.0, 2.0, 3.0, 1.0});
  populateWsWithInitList(outputWS->dataF(2), 1, {1.0, 2.0, 2.0, 1.0});

  // Q bin #4
  populateWsWithInitList(outputWS->mutableY(3), 0, {1.0, 2.0, 3.0, 2.0, 1.0});
  populateWsWithInitList(outputWS->mutableE(3), 0, {1.0, 2.0, 3.0, 2.0, 1.0});
  populateWsWithInitList(outputWS->dataF(3), 0, {1.0, 2.0, 3.0, 2.0, 1.0, 1.0});

  // Set representation
  outputWS->finalize();

  // Make errors squared rooted
  for (int i = 0; i < numHist; ++i) {
    auto &mutableE = outputWS->mutableE(i);
    for (int j = 0; j < numX - 1; ++j) {
      mutableE[j] = std::sqrt(mutableE[j]);
    }
  }

  return outputWS;
}

/**
 * Populates the destination array (usually a mutable histogram)
 * starting at the index specified with the doubles provided in an
 * initializer list. Note the caller is responsible for ensuring
 * the destination has capacity for startingIndex + size(initializer list)
 * number of values
 *
 * @param destination :: The array to populate with data
 * @param startingIndex :: The index to start populating data at
 * @param values :: The initializer list to populate the array with
 * starting at the index specified
 */
template <typename T>
void populateWsWithInitList(T &destination, size_t startingIndex,
                            const std::initializer_list<double> &values) {
  size_t index = 0;
  for (const double val : values) {
    destination[startingIndex + index] = val;
    index++;
  }
}

Mantid::DataObjects::PeaksWorkspace_sptr
createPeaksWorkspace(const int numPeaks, const bool createOrientedLattice) {
  auto peaksWS = boost::make_shared<PeaksWorkspace>();
  Instrument_sptr inst =
      ComponentCreationHelper::createTestInstrumentRectangular2(1, 10);
  peaksWS->setInstrument(inst);

  for (int i = 0; i < numPeaks; ++i) {
    Peak peak(inst, i, i + 0.5);
    peaksWS->addPeak(peak);
  }

  if (createOrientedLattice) {
    Mantid::Geometry::OrientedLattice lattice;
    peaksWS->mutableSample().setOrientedLattice(&lattice);
  }
  return peaksWS;
}

/** helper method to create preprocessed detector's table workspace */
boost::shared_ptr<DataObjects::TableWorkspace>
createTableWorkspace(const API::MatrixWorkspace_const_sptr &inputWS) {
  const size_t nHist = inputWS->getNumberHistograms();

  // set the target workspace
  auto targWS = boost::make_shared<TableWorkspace>(nHist);
  // detectors positions
  targWS->addColumn("V3D", "DetDirections");
  // sample-detector distance;
  targWS->addColumn("double", "L2");
  // Diffraction angle
  targWS->addColumn("double", "TwoTheta");
  targWS->addColumn("double", "Azimuthal");
  // the detector ID;
  targWS->addColumn("int", "DetectorID");
  // stores spectra index which corresponds to a valid detector index;
  targWS->addColumn("size_t", "detIDMap");
  // stores detector index which corresponds to the workspace index;
  targWS->addColumn("size_t", "spec2detMap");

  // will see about that
  // sin^2(Theta)
  //    std::vector<double>      SinThetaSq;

  //,"If the detectors were actually processed from real instrument or generated
  // for some fake one ");
  return targWS;
}

/** method does preliminary calculations of the detectors positions to convert
 results into k-dE space ;
 and places the results into static cash to be used in subsequent calls to this
 algorithm */
void processDetectorsPositions(const API::MatrixWorkspace_const_sptr &inputWS,
                               DataObjects::TableWorkspace_sptr &targWS,
                               double Ei) {
  Geometry::Instrument_const_sptr instrument = inputWS->getInstrument();
  //
  Geometry::IComponent_const_sptr source = instrument->getSource();
  Geometry::IComponent_const_sptr sample = instrument->getSample();
  if ((!source) || (!sample)) {

    throw Kernel::Exception::InstrumentDefinitionError(
        "Instrument not sufficiently defined: failed to get source and/or "
        "sample");
  }

  // L1
  try {
    double L1 = source->getDistance(*sample);
    targWS->logs()->addProperty<double>("L1", L1, true);
  } catch (Kernel::Exception::NotFoundError &) {
    throw Kernel::Exception::InstrumentDefinitionError(
        "Unable to calculate source-sample distance for workspace",
        inputWS->getTitle());
  }
  // Instrument name
  std::string InstrName = instrument->getName();
  targWS->logs()->addProperty<std::string>("InstrumentName", InstrName, true);
  targWS->logs()->addProperty<bool>("FakeDetectors", false, true);
  targWS->logs()->addProperty<double>("Ei", Ei, true); //"Incident energy for
  // Direct or Analysis
  // energy for indirect
  // instrument");

  // get access to the workspace memory
  auto &sp2detMap = targWS->getColVector<size_t>("spec2detMap");
  auto &detId = targWS->getColVector<int32_t>("DetectorID");
  auto &detIDMap = targWS->getColVector<size_t>("detIDMap");
  auto &L2 = targWS->getColVector<double>("L2");
  auto &TwoTheta = targWS->getColVector<double>("TwoTheta");
  auto &Azimuthal = targWS->getColVector<double>("Azimuthal");
  auto &detDir = targWS->getColVector<Kernel::V3D>("DetDirections");

  //// progress messave appearence
  size_t nHist = targWS->rowCount();
  //// Loop over the spectra
  uint32_t liveDetectorsCount(0);
  const auto &spectrumInfo = inputWS->spectrumInfo();
  for (size_t i = 0; i < nHist; i++) {
    sp2detMap[i] = std::numeric_limits<size_t>::quiet_NaN();
    detId[i] = std::numeric_limits<int32_t>::quiet_NaN();
    detIDMap[i] = std::numeric_limits<size_t>::quiet_NaN();
    L2[i] = std::numeric_limits<double>::quiet_NaN();
    TwoTheta[i] = std::numeric_limits<double>::quiet_NaN();
    Azimuthal[i] = std::numeric_limits<double>::quiet_NaN();

    if (!spectrumInfo.hasDetectors(i) || spectrumInfo.isMonitor(i))
      continue;

    // calculate the requested values;
    sp2detMap[i] = liveDetectorsCount;
    detId[liveDetectorsCount] = int32_t(spectrumInfo.detector(i).getID());
    detIDMap[liveDetectorsCount] = i;
    L2[liveDetectorsCount] = spectrumInfo.l2(i);

    double polar = spectrumInfo.twoTheta(i);
    double azim = spectrumInfo.detector(i).getPhi();
    TwoTheta[liveDetectorsCount] = polar;
    Azimuthal[liveDetectorsCount] = azim;

    double sPhi = sin(polar);
    double ez = cos(polar);
    double ex = sPhi * cos(azim);
    double ey = sPhi * sin(azim);

    detDir[liveDetectorsCount].setX(ex);
    detDir[liveDetectorsCount].setY(ey);
    detDir[liveDetectorsCount].setZ(ez);

    // double sinTheta=sin(0.5*polar);
    // this->SinThetaSq[liveDetectorsCount]  = sinTheta*sinTheta;

    liveDetectorsCount++;
  }
  targWS->logs()->addProperty<uint32_t>(
      "ActualDetectorsNum", liveDetectorsCount,
      true); //,"The actual number of detectors receivinv signal");
}

boost::shared_ptr<Mantid::DataObjects::TableWorkspace>
buildPreprocessedDetectorsWorkspace(Mantid::API::MatrixWorkspace_sptr ws) {
  Mantid::DataObjects::TableWorkspace_sptr DetPos = createTableWorkspace(ws);
  double Ei = ws->run().getPropertyValueAsType<double>("Ei");
  processDetectorsPositions(ws, DetPos, Ei);

  return DetPos;
}
void create2DAngles(std::vector<double> &L2, std::vector<double> &polar,
                    std::vector<double> &azim, size_t nPolar, size_t nAzim,
                    double polStart, double polEnd, double azimStart,
                    double azimEnd) {
  size_t nDet = nPolar * nAzim;
  L2.resize(nDet, 10);
  polar.resize(nDet);
  azim.resize(nDet);

  double dPolar = (polEnd - polStart) / static_cast<double>(nDet - 1);
  double dAzim = (azimEnd - azimEnd) / static_cast<double>(nDet - 1);
  for (size_t i = 0; i < nPolar; i++) {
    for (size_t j = 0; j < nAzim; j++) {
      polar[i * nPolar + j] = polStart + dPolar * static_cast<double>(i);
      azim[i * nPolar + j] = azimStart + dAzim * static_cast<double>(j);
    }
  }
}

ITableWorkspace_sptr
createEPPTableWorkspace(const std::vector<EPPTableRow> &rows) {
  ITableWorkspace_sptr ws = boost::make_shared<TableWorkspace>(rows.size());
  auto wsIndexColumn = ws->addColumn("int", "WorkspaceIndex");
  auto centreColumn = ws->addColumn("double", "PeakCentre");
  auto centreErrorColumn = ws->addColumn("double", "PeakCentreError");
  auto sigmaColumn = ws->addColumn("double", "Sigma");
  auto sigmaErrorColumn = ws->addColumn("double", "SigmaError");
  auto heightColumn = ws->addColumn("double", "Height");
  auto heightErrorColumn = ws->addColumn("double", "HeightError");
  auto chiSqColumn = ws->addColumn("double", "chiSq");
  auto statusColumn = ws->addColumn("str", "FitStatus");
  for (size_t i = 0; i != rows.size(); ++i) {
    const auto &row = rows[i];
    if (row.workspaceIndex < 0) {
      wsIndexColumn->cell<int>(i) = static_cast<int>(i);
    } else {
      wsIndexColumn->cell<int>(i) = row.workspaceIndex;
    }
    centreColumn->cell<double>(i) = row.peakCentre;
    centreErrorColumn->cell<double>(i) = row.peakCentreError;
    sigmaColumn->cell<double>(i) = row.sigma;
    sigmaErrorColumn->cell<double>(i) = row.sigmaError;
    heightColumn->cell<double>(i) = row.height;
    heightErrorColumn->cell<double>(i) = row.heightError;
    chiSqColumn->cell<double>(i) = row.chiSq;
    statusColumn->cell<std::string>(i) =
        row.fitStatus == EPPTableRow::FitStatus::SUCCESS ? "success" : "failed";
  }
  return ws;
}
Mantid::DataObjects::Workspace2D_sptr create2DWorkspace123WithMaskedBin(
    int numHist, int numBins, int maskedWorkspaceIndex, int maskedBinIndex) {
  auto ws = create2DWorkspace123(numHist, numBins);
  ws->flagMasked(maskedWorkspaceIndex, maskedBinIndex);
  return ws;
}
} // namespace WorkspaceCreationHelper
