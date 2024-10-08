// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
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
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidFrameworkTestHelpers/InstrumentCreationHelper.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataObjects/GroupingWorkspace.h"
#include "MantidDataObjects/LeanElasticPeaksWorkspace.h"
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

StubAlgorithm::StubAlgorithm(size_t nSteps) : m_Progress(std::make_unique<API::Progress>(this, 0.0, 1.0, nSteps)) {}

EPPTableRow::EPPTableRow(const double peakCentre_, const double sigma_, const double height_,
                         const FitStatus fitStatus_)
    : peakCentre(peakCentre_), sigma(sigma_), height(height_), fitStatus(fitStatus_) {}

EPPTableRow::EPPTableRow(const int index, const double peakCentre_, const double sigma_, const double height_,
                         const FitStatus fitStatus_)
    : workspaceIndex(index), peakCentre(peakCentre_), sigma(sigma_), height(height_), fitStatus(fitStatus_) {}

/**
 * Deletes a workspace
 * @param name :: The name of the workspace
 */
void removeWS(const std::string &name) { Mantid::API::AnalysisDataService::Instance().remove(name); }

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
template <typename YType, typename EType> Histogram createHisto(bool isHistogram, YType &&yAxis, EType &&eAxis) {
  // We don't need to check if y.size() == e.size() as the histogram
  // type does this at construction
  const size_t yValsSize = yAxis.size();
  if (isHistogram) {
    BinEdges xAxis(yValsSize + 1, LinearGenerator(1, 1));
    Histogram histo{std::move(xAxis), std::forward<YType>(yAxis), std::forward<EType>(eAxis)};
    return histo;
  } else {
    Points xAxis(yValsSize, LinearGenerator(1, 1));
    Histogram pointsHisto{std::move(xAxis), std::forward<YType>(yAxis), std::forward<EType>(eAxis)};
    return pointsHisto;
  }
}

Workspace2D_sptr create1DWorkspaceRand(int size, bool isHisto) {

  MersenneTwister randomGen(DateAndTime::getCurrentTime().nanoseconds(), 0, std::numeric_limits<int>::max());

  auto randFunc = [&randomGen] { return randomGen.nextValue(); };
  Counts counts(size, randFunc);
  CountStandardDeviations errorVals(size, randFunc);

  auto retVal = std::make_shared<Workspace2D>();
  retVal->initialize(1, createHisto(isHisto, counts, errorVals));
  return retVal;
}

Workspace2D_sptr create1DWorkspaceConstant(int size, double value, double error, bool isHisto) {
  Counts yVals(size, value);
  CountStandardDeviations errVals(size, error);

  auto retVal = std::make_shared<Workspace2D>();
  retVal->initialize(1, createHisto(isHisto, yVals, errVals));
  return retVal;
}

Workspace2D_sptr create1DWorkspaceConstantWithXerror(int size, double value, double error, double xError,
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

  auto retVal = std::make_shared<Workspace2D>();
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
Workspace2D_sptr create2DWorkspaceWhereYIsWorkspaceIndex(int nhist, int numBoundaries) {
  Workspace2D_sptr out = create2DWorkspaceBinned(nhist, numBoundaries);
  for (int workspaceIndex = 0; workspaceIndex < nhist; workspaceIndex++) {
    std::vector<double> yValues(numBoundaries, static_cast<double>(workspaceIndex));
    out->mutableY(workspaceIndex) = yValues;
  }

  return out;
}

Workspace2D_sptr create2DWorkspaceThetaVsTOF(int nHist, int nBins) {

  Workspace2D_sptr outputWS = create2DWorkspaceBinned(nHist, nBins);
  auto newAxis = std::make_unique<NumericAxis>(nHist);
  auto newAxisRaw = newAxis.get();
  outputWS->replaceAxis(1, std::move(newAxis));
  newAxisRaw->unit() = std::make_shared<Units::Degrees>();
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
Workspace2D_sptr create2DWorkspaceWithValues(int64_t nHist, int64_t nBins, bool isHist,
                                             const std::set<int64_t> &maskedWorkspaceIndices, double xVal, double yVal,
                                             double eVal, bool hasDx = false) {
  auto x1 = Kernel::make_cow<HistogramData::HistogramX>(isHist ? nBins + 1 : nBins, LinearGenerator(xVal, 1.0));
  Counts y1(nBins, yVal);
  CountStandardDeviations e1(nBins, eVal);
  auto dx = Kernel::make_cow<HistogramData::HistogramDx>(nBins, yVal);
  auto retVal = std::make_shared<Workspace2D>();
  retVal->initialize(nHist, createHisto(isHist, y1, e1));
  for (int i = 0; i < nHist; i++) {
    retVal->setSharedX(i, x1);
    if (hasDx)
      retVal->setSharedDx(i, dx);
    retVal->getSpectrum(i).setDetectorID(i);
    retVal->getSpectrum(i).setSpectrumNo(i);
  }
  if (maskedWorkspaceIndices.size() > 0) {
    retVal = maskSpectra(retVal, maskedWorkspaceIndices);
  }
  return retVal;
}

Workspace2D_sptr create2DWorkspaceWithValuesAndXerror(int64_t nHist, int64_t nBins, bool isHist, double xVal,
                                                      double yVal, double eVal, double dxVal,
                                                      const std::set<int64_t> &maskedWorkspaceIndices) {
  auto ws = create2DWorkspaceWithValues(nHist, nBins, isHist, maskedWorkspaceIndices, xVal, yVal, eVal);
  PointStandardDeviations dx1(nBins, dxVal);
  for (int i = 0; i < nHist; i++) {
    ws->setPointStandardDeviations(i, dx1);
  }
  return ws;
}

Workspace2D_sptr create2DWorkspace123(int64_t nHist, int64_t nBins, bool isHist,
                                      const std::set<int64_t> &maskedWorkspaceIndices, bool hasDx) {
  return create2DWorkspaceWithValues(nHist, nBins, isHist, maskedWorkspaceIndices, 1.0, 2.0, 3.0, hasDx);
}

Workspace2D_sptr create2DWorkspace154(int64_t nHist, int64_t nBins, bool isHist,
                                      const std::set<int64_t> &maskedWorkspaceIndices, bool hasDx) {
  return create2DWorkspaceWithValues(nHist, nBins, isHist, maskedWorkspaceIndices, 1.0, 5.0, 4.0, hasDx);
}

Workspace2D_sptr maskSpectra(Workspace2D_sptr workspace, const std::set<int64_t> &maskedWorkspaceIndices) {
  const auto nhist = static_cast<int>(workspace->getNumberHistograms());
  if (workspace->getInstrument()->nelements() == 0) {
    // We need detectors to be able to mask them.
    auto instrument = std::make_shared<Instrument>();
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
WorkspaceGroup_sptr createWorkspaceGroup(int nEntries, int nHist, int nBins, const std::string &stem) {
  auto group = std::make_shared<WorkspaceGroup>();
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
 * Filled with Y = 2.0 and E = M_SQRT2
 */
Workspace2D_sptr create2DWorkspaceBinned(size_t nhist, size_t numVals, double x0, double deltax) {
  BinEdges x(numVals + 1, LinearGenerator(x0, deltax));
  Counts y(numVals, 2);
  CountStandardDeviations e(numVals, M_SQRT2);
  return create<Workspace2D>(nhist, Histogram(x, y, e));
}

/** Create a 2D workspace with this many point-histograms and bins.
 * Filled with Y = 2.0 and E = M_SQRT2
 */
Workspace2D_sptr create2DWorkspacePoints(size_t nhist, size_t numVals, double x0, double deltax) {
  Points x(numVals, LinearGenerator(x0, deltax));
  Counts y(numVals, 2);
  CountStandardDeviations e(numVals, M_SQRT2);
  return create<Workspace2D>(nhist, Histogram(x, y, e));
}

/** Create a 2D workspace with this many histograms and bins. The bins are
 * assumed to be non-uniform and given by the input array
 * Filled with Y = 2.0 and E = M_SQRT2
 * If hasDx is true, all spectra will have dx values, starting from 0.1 and
 * increased by 0.1 for each bin.
 */
Workspace2D_sptr create2DWorkspaceNonUniformlyBinned(int nhist, const int numBoundaries, const double xBoundaries[],
                                                     bool hasDx) {
  BinEdges x(xBoundaries, xBoundaries + numBoundaries);
  const int numBins = numBoundaries - 1;
  Counts y(numBins, 2);
  CountStandardDeviations e(numBins, M_SQRT2);
  auto dx = Kernel::make_cow<HistogramData::HistogramDx>(numBins, LinearGenerator(0.1, .1));
  auto retVal = std::make_shared<Workspace2D>();
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
void addNoise(const Mantid::API::MatrixWorkspace_sptr &ws, double noise, const double lower, const double upper) {
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
Workspace2D_sptr create2DWorkspaceWithFullInstrument(int nhist, int nbins, bool includeMonitors, bool startYNegative,
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
    space = create2DWorkspace123(nhist, nbins, false, std::set<int64_t>(), hasDx);
  // actually adds a property called run_title to the logs
  space->setTitle("Test histogram");
  space->getAxis(0)->setUnit("TOF");
  space->setYUnit("Counts");

  InstrumentCreationHelper::addFullInstrumentToWorkspace(*space, includeMonitors, startYNegative, instrumentName);

  return space;
}

//================================================================================================================
/*
 * startTime is in seconds
 */
MatrixWorkspace_sptr create2DDetectorScanWorkspaceWithFullInstrument(int nhist, int nbins, size_t nTimeIndexes,
                                                                     size_t startTime, size_t firstInterval,
                                                                     bool includeMonitors, bool startYNegative,
                                                                     bool isHistogram,
                                                                     const std::string &instrumentName) {

  auto baseWS =
      create2DWorkspaceWithFullInstrument(nhist, nbins, includeMonitors, startYNegative, isHistogram, instrumentName);

  auto builder = ScanningWorkspaceBuilder(baseWS->getInstrument(), nTimeIndexes, nbins);

  std::vector<double> timeRanges;
  for (size_t i = 0; i < nTimeIndexes; ++i) {
    timeRanges.emplace_back(double(i + firstInterval));
  }

  builder.setTimeRanges(DateAndTime(int(startTime), 0), timeRanges);

  return builder.buildWorkspace();
}

//================================================================================================================
/** Create an Workspace2D with an instrument that contains detectors arranged at even latitude/longitude
 * values. For use in testing absorption and multiple scattering corrections. The sparse instrument functionality
 * in these algorithms uses geographical angles (lat/long) to specify the detector positions
 * Latitude/longitude corresponds to two theta if longitude/latitude equals zero
 */
Workspace2D_sptr create2DWorkspaceWithGeographicalDetectors(const int nlat, const int nlong, const double anginc,
                                                            const int nbins, const double x0, const double deltax,
                                                            const std::string &instrumentName,
                                                            const std::string &xunit) {
  auto inputWorkspace = WorkspaceCreationHelper::create2DWorkspaceBinned(nlat * nlong, nbins, x0, deltax);
  inputWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create(xunit);

  InstrumentCreationHelper::addInstrumentWithGeographicalDetectorsToWorkspace(*inputWorkspace, nlat, nlong, anginc,
                                                                              instrumentName);

  return inputWorkspace;
}

//================================================================================================================
/** Create an Workspace2D with an instrument that contains
 *RectangularDetector's.
 * Bins will be 0.0, 1.0, to numBins, filled with signal=2.0, M_SQRT2
 *
 * @param numBanks :: number of rectangular banks
 * @param numPixels :: each bank will be numPixels*numPixels
 * @param numBins :: each spectrum will have this # of bins
 * @param instrumentName :: the name of the workspace's instrument
 * @return The Workspace2D
 */
Mantid::DataObjects::Workspace2D_sptr create2DWorkspaceWithRectangularInstrument(int numBanks, int numPixels,
                                                                                 int numBins,
                                                                                 const std::string &instrumentName) {
  Instrument_sptr inst =
      ComponentCreationHelper::createTestInstrumentRectangular(numBanks, numPixels, 0.008, 5.0, false, instrumentName);
  Workspace2D_sptr ws = create2DWorkspaceBinned(numBanks * numPixels * numPixels, numBins);
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
Mantid::DataObjects::EventWorkspace_sptr createEventWorkspaceWithFullInstrument(int numBanks, int numPixels,
                                                                                bool clearEvents) {
  Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular(numBanks, numPixels);
  EventWorkspace_sptr ws = createEventWorkspace2(numBanks * numPixels * numPixels, 100);
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

/** Create an Eventworkspace with  instrument 2.0 that contains
 *RectangularDetector's.
 * X axis = 100 histogrammed bins from 0.0 in steps of 1.0.
 * 200 events per pixel.
 *
 * @param numBanks :: number of rectangular banks
 * @param numPixels :: each bank will be numPixels*numPixels
 * @param clearEvents :: if true, erase the events from list
 * @return The EventWorkspace
 */
Mantid::DataObjects::EventWorkspace_sptr createEventWorkspaceWithFullInstrument2(int numBanks, int numPixels,
                                                                                 bool clearEvents) {
  Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular2(numBanks, numPixels);
  EventWorkspace_sptr ws = createEventWorkspace2(numBanks * numPixels * numPixels, 100);
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

Mantid::DataObjects::EventWorkspace_sptr createEventWorkspaceWithNonUniformInstrument(int numBanks, bool clearEvents) {
  // Number of detectors in a bank as created by createTestInstrumentCylindrical
  const int DETECTORS_PER_BANK(9);

  V3D srcPos(0., 0., -10.), samplePos;
  Instrument_sptr inst =
      ComponentCreationHelper::createTestInstrumentCylindrical(numBanks, srcPos, samplePos, 0.0025, 0.005);
  EventWorkspace_sptr ws = createEventWorkspace2(numBanks * DETECTORS_PER_BANK, 100);
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
DataObjects::Workspace2D_sptr reflectometryWorkspace(const double startX, const int nSpectra, const int nBins,
                                                     const double deltaX) {

  auto workspace = create2DWorkspaceBinned(nSpectra, nBins, startX, deltaX);

  workspace->setTitle("Test histogram"); // actually adds a property call run_title to the logs
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
MatrixWorkspace_sptr create2DWorkspaceWithReflectometryInstrument(const double startX, const V3D &slit1Pos,
                                                                  const V3D &slit2Pos, const double vg1,
                                                                  const double vg2, const V3D &sourcePos,
                                                                  const V3D &monitorPos, const V3D &samplePos,
                                                                  const V3D &detectorPos, const int nBins,
                                                                  const double deltaX) {
  Instrument_sptr instrument = std::make_shared<Instrument>();
  instrument->setReferenceFrame(
      std::make_shared<ReferenceFrame>(PointingAlong::Y, PointingAlong::X, Handedness::Left, "0,0,0"));

  InstrumentCreationHelper::addSource(instrument, sourcePos, "source");
  InstrumentCreationHelper::addMonitor(instrument, monitorPos, 1, "Monitor");
  InstrumentCreationHelper::addSample(instrument, samplePos, "some-surface-holder");
  InstrumentCreationHelper::addDetector(instrument, detectorPos, 2, "point-detector");
  auto slit1 = InstrumentCreationHelper::addComponent(instrument, slit1Pos, "slit1");
  auto slit2 = InstrumentCreationHelper::addComponent(instrument, slit2Pos, "slit2");

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
    const double startX, const double detSize, const V3D &slit1Pos, const V3D &slit2Pos, const double vg1,
    const double vg2, const V3D &sourcePos, const V3D &monitorPos, const V3D &samplePos, const V3D &detectorCenterPos,
    const int nSpectra, const int nBins, const double deltaX) {
  Instrument_sptr instrument = std::make_shared<Instrument>();
  instrument->setReferenceFrame(
      std::make_shared<ReferenceFrame>(PointingAlong::Y /*up*/, PointingAlong::X /*along*/, Handedness::Left, "0,0,0"));

  InstrumentCreationHelper::addSource(instrument, sourcePos, "source");
  InstrumentCreationHelper::addSample(instrument, samplePos, "some-surface-holder");
  InstrumentCreationHelper::addMonitor(instrument, monitorPos, 1, "Monitor");

  const int nDet = nSpectra - 1;
  const double minY = detectorCenterPos.Y() - detSize * (nDet - 1) / 2.;
  for (int i = 0; i < nDet; ++i) {
    const double y = minY + i * detSize;
    const V3D pos{detectorCenterPos.X(), y, detectorCenterPos.Z()};
    InstrumentCreationHelper::addDetector(instrument, pos, i + 2, "point-detector");
  }
  auto slit1 = InstrumentCreationHelper::addComponent(instrument, slit1Pos, "slit1");
  auto slit2 = InstrumentCreationHelper::addComponent(instrument, slit2Pos, "slit2");

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

void createInstrumentForWorkspaceWithDistances(const MatrixWorkspace_sptr &workspace, const V3D &samplePosition,
                                               const V3D &sourcePosition, const std::vector<V3D> &detectorPositions) {
  Instrument_sptr instrument = std::make_shared<Instrument>();
  instrument->setReferenceFrame(std::make_shared<ReferenceFrame>(Y, X, Left, "0,0,0"));

  InstrumentCreationHelper::addSource(instrument, sourcePosition, "source");
  InstrumentCreationHelper::addSample(instrument, samplePosition, "sample");

  for (int i = 0; i < static_cast<int>(detectorPositions.size()); ++i) {
    std::stringstream buffer;
    buffer << "detector_" << i;
    InstrumentCreationHelper::addDetector(instrument, detectorPositions[i], i, buffer.str());

    // Link it to the workspace
    workspace->getSpectrum(i).addDetectorID(i);
  }
  workspace->setInstrument(instrument);
}

//================================================================================================================
WorkspaceSingleValue_sptr createWorkspaceSingleValue(double value) {
  return std::make_shared<WorkspaceSingleValue>(value, sqrt(value));
}

WorkspaceSingleValue_sptr createWorkspaceSingleValueWithError(double value, double error) {
  return std::make_shared<WorkspaceSingleValue>(value, error);
}

/** Perform some finalization on event workspace stuff */
void eventWorkspace_Finalize(const EventWorkspace_sptr &ew) {
  // get a proton charge
  ew->mutableRun().integrateProtonCharge();
}

/** Create event workspace with:
 * 500 pixels
 * 1000 histogrammed bins.
 */
EventWorkspace_sptr createEventWorkspace() { return createEventWorkspace(500, 1001, 100, 1000); }

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
EventWorkspace_sptr createEventWorkspace(int numPixels, int numBins, int numEvents, double x0, double binDelta,
                                         int eventPattern, int start_at_pixelID) {
  return createEventWorkspaceWithStartTime(numPixels, numBins, numEvents, x0, binDelta, eventPattern, start_at_pixelID,
                                           DateAndTime("2010-01-01T00:00:00"));
}

/**
 * Create event workspace with defined start date time
 */
EventWorkspace_sptr createEventWorkspaceWithStartTime(int numPixels, int numBins, int numEvents, double x0,
                                                      double binDelta, int eventPattern, int start_at_pixelID,
                                                      DateAndTime run_start) {

  // add one to the number of bins as this is histogram
  numBins++;

  auto retVal = std::make_shared<EventWorkspace>();
  retVal->initialize(numPixels, 1, 1);

  // Make fake events
  if (eventPattern) // 0 == no events
  {
    size_t workspaceIndex = 0;
    for (int pix = start_at_pixelID + 0; pix < start_at_pixelID + numPixels; pix++) {
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
EventWorkspace_sptr createGroupedEventWorkspace(std::vector<std::vector<int>> const &groups, int numBins,
                                                double binDelta, double xOffset) {

  auto retVal = std::make_shared<EventWorkspace>();
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
      retVal->setX(g, make_cow<HistogramX>(numBins, LinearGenerator(x0, binDelta)));
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
EventWorkspace_sptr createRandomEventWorkspace(size_t numbins, size_t numpixels, double bin_delta) {
  auto retVal = std::make_shared<EventWorkspace>();
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

  MersenneTwister randomGen(DateAndTime::getCurrentTime().nanoseconds(), 0, std::numeric_limits<int>::max());
  // Make up some data for each pixels
  for (size_t i = 0; i < numpixels; i++) {
    // Create one event for each bin
    EventList &events = retVal->getSpectrum(static_cast<detid_t>(i));
    for (std::size_t ie = 0; ie < numbins; ie++) {
      // Create a list of events, randomize
      events += TofEvent(static_cast<double>(randomGen.nextValue()), static_cast<int64_t>(randomGen.nextValue()));
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
MatrixWorkspace_sptr createGroupedWorkspace2D(size_t numHist, int numBins, double binDelta) {
  Workspace2D_sptr retVal = create2DWorkspaceBinned(static_cast<int>(numHist), numBins, 0.0, binDelta);
  retVal->setInstrument(ComponentCreationHelper::createTestInstrumentCylindrical(static_cast<int>(numHist)));

  for (int g = 0; g < static_cast<int>(numHist); g++) {
    auto &spec = retVal->getSpectrum(g);
    for (int i = 1; i <= 9; i++)
      spec.addDetectorID(g * 9 + i);
    spec.setSpectrumNo(g + 1); // Match detector ID and spec NO
  }
  return std::dynamic_pointer_cast<MatrixWorkspace>(retVal);
}

// =====================================================================================
// RootOfNumHist == square root of hystohram number;
MatrixWorkspace_sptr createGroupedWorkspace2DWithRingsAndBoxes(size_t RootOfNumHist, int numBins, double binDelta) {
  size_t numHist = RootOfNumHist * RootOfNumHist;
  Workspace2D_sptr retVal = create2DWorkspaceBinned(static_cast<int>(numHist), numBins, 0.0, binDelta);
  retVal->setInstrument(ComponentCreationHelper::createTestInstrumentCylindrical(static_cast<int>(numHist)));
  for (int g = 0; g < static_cast<int>(numHist); g++) {
    auto &spec = retVal->getSpectrum(g);
    spec.addDetectorID(g + 1); // Legacy comptibilty: Used to be default IDs in Workspace2D.
    for (int i = 1; i <= 9; i++)
      spec.addDetectorID(g * 9 + i);
    spec.setSpectrumNo(g + 1); // Match detector ID and spec NO
  }
  return std::dynamic_pointer_cast<MatrixWorkspace>(retVal);
}

// not strictly creating a workspace, but really helpful to see what one
// contains
void displayDataY(const MatrixWorkspace_const_sptr &ws) {
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
void displayData(const MatrixWorkspace_const_sptr &ws) { displayDataX(ws); }

// not strictly creating a workspace, but really helpful to see what one
// contains
void displayDataX(const MatrixWorkspace_const_sptr &ws) {
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
void displayDataE(const MatrixWorkspace_const_sptr &ws) {
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
void addTSPEntry(Run &runInfo, const std::string &name, double val) {
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
void setOrientedLattice(const Mantid::API::MatrixWorkspace_sptr &ws, double a, double b, double c) {
  ws->mutableSample().setOrientedLattice(std::make_unique<OrientedLattice>(a, b, c, 90., 90., 90.));
}

// =====================================================================================
/** Create a default universal goniometer and set its angles
 *
 * @param ws :: workspace to set
 * @param phi :: +Y rotation angle (deg)
 * @param chi :: +X rotation angle (deg)
 * @param omega :: +Y rotation angle (deg)
 */
void setGoniometer(const Mantid::API::MatrixWorkspace_sptr &ws, double phi, double chi, double omega) {
  addTSPEntry(ws->mutableRun(), "phi", phi);
  addTSPEntry(ws->mutableRun(), "chi", chi);
  addTSPEntry(ws->mutableRun(), "omega", omega);
  Mantid::Geometry::Goniometer gm;
  gm.makeUniversalGoniometer();
  ws->mutableRun().setGoniometer(gm, true);
}

//
Mantid::API::MatrixWorkspace_sptr createProcessedWorkspaceWithCylComplexInstrument(size_t numPixels, size_t numBins,
                                                                                   bool has_oriented_lattice) {
  auto rHist = static_cast<size_t>(std::sqrt(static_cast<double>(numPixels)));
  while (rHist * rHist < numPixels)
    rHist++;

  Mantid::API::MatrixWorkspace_sptr ws = createGroupedWorkspace2DWithRingsAndBoxes(rHist, 10, 0.1);
  auto pAxis0 = std::make_unique<NumericAxis>(numBins);
  for (size_t i = 0; i < numBins; i++) {
    double dE = -1.0 + static_cast<double>(i) * 0.8;
    pAxis0->setValue(i, dE);
  }
  pAxis0->setUnit("DeltaE");
  ws->replaceAxis(0, std::move(pAxis0));
  if (has_oriented_lattice) {
    ws->mutableSample().setOrientedLattice(std::make_unique<OrientedLattice>(1, 1, 1, 90., 90., 90.));

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
Mantid::API::MatrixWorkspace_sptr createProcessedInelasticWS(const std::vector<double> &L2,
                                                             const std::vector<double> &polar,
                                                             const std::vector<double> &azimutal, size_t numBins,
                                                             double Emin, double Emax, double Ei) {
  // not used but interface needs it
  std::set<int64_t> maskedWorkspaceIndices;
  size_t numPixels = L2.size();

  Mantid::API::MatrixWorkspace_sptr ws =
      create2DWorkspaceWithValues(uint64_t(numPixels), uint64_t(numBins), true, maskedWorkspaceIndices, 0, 1, 0.1);

  // detectors at L2, sample at 0 and source at -L2_min
  ws->setInstrument(ComponentCreationHelper::createCylInstrumentWithDetInGivenPositions(L2, polar, azimutal));

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
      E_transfer.emplace_back(Emin + static_cast<double>(i) * dE);
    }
    ws->mutableX(j) = E_transfer;
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
  ws->mutableSample().setOrientedLattice(std::make_unique<OrientedLattice>(1, 1, 1, 90., 90., 90.));

  ws->mutableRun().addProperty(new PropertyWithValue<std::string>("deltaE-mode", "Direct"), true);
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
createEventWorkspace3(const Mantid::DataObjects::EventWorkspace_const_sptr &sourceWS, const std::string &wsname,
                      API::Algorithm *alg) {
  UNUSED_ARG(wsname);
  // 1. Initialize:use dummy numbers for arguments, for event workspace it
  // doesn't matter
  Mantid::DataObjects::EventWorkspace_sptr outputWS =
      Mantid::DataObjects::EventWorkspace_sptr(new DataObjects::EventWorkspace());
  outputWS->initialize(sourceWS->getInstrument()->getDetectorIDs(true).size(), 1, 1);

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
  Mantid::API::Algorithm_sptr loadInst = alg->createChildAlgorithm("LoadInstrument");
  // Now execute the Child Algorithm. Catch and log any error, but don't stop.
  loadInst->setPropertyValue("InstrumentName", sourceWS->getInstrument()->getName());
  loadInst->setProperty<MatrixWorkspace_sptr>("Workspace", outputWS);
  loadInst->setProperty("RewriteSpectraMap", Mantid::Kernel::OptionalBool(true));
  loadInst->executeAsChildAlg();
  // Populate the instrument parameters in this workspace - this works around a
  // bug
  outputWS->populateInstrumentParameters();

  // 6. Build spectrum and event list
  // a) We want to pad out empty pixels.
  detid2det_map detector_map;
  outputWS->getInstrument()->getDetectors(detector_map);

  // b) Pad all the pixels and Set to zero
  size_t workspaceIndex = 0;
  const auto &detectorInfo = outputWS->detectorInfo();
  for (auto it = detector_map.begin(); it != detector_map.end(); ++it) {
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
  RebinnedOutput_sptr outputWS = Mantid::DataObjects::RebinnedOutput_sptr(new RebinnedOutput());
  // outputWS->setName("rebinTest");
  Mantid::API::AnalysisDataService::Instance().add("rebinTest", outputWS);

  // Set Q ('y') axis binning
  std::vector<double> qbins{0.0, 1.0, 4.0};
  std::vector<double> qaxis;
  const auto numY = static_cast<int>(VectorHelper::createAxisFromRebinParams(qbins, qaxis));

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

  // Make errors squared rooted and set sqrd err flag
  for (int i = 0; i < numHist; ++i) {
    auto &mutableE = outputWS->mutableE(i);
    for (int j = 0; j < numX - 1; ++j) {
      mutableE[j] = std::sqrt(mutableE[j]);
    }
  }
  outputWS->setSqrdErrors(false);

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
void populateWsWithInitList(T &destination, size_t startingIndex, const std::initializer_list<double> &values) {
  size_t index = 0;
  for (const double val : values) {
    destination[startingIndex + index] = val;
    index++;
  }
}

Mantid::DataObjects::PeaksWorkspace_sptr createPeaksWorkspace(const int numPeaks, const bool createOrientedLattice) {
  auto peaksWS = std::make_shared<PeaksWorkspace>();
  Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular2(1, 10);
  peaksWS->setInstrument(inst);

  for (int i = 0; i < numPeaks; ++i) {
    Peak peak(inst, i, i + 0.5);
    peaksWS->addPeak(peak);
  }

  if (createOrientedLattice) {
    peaksWS->mutableSample().setOrientedLattice(std::make_unique<OrientedLattice>());
  }
  return peaksWS;
}

Mantid::DataObjects::PeaksWorkspace_sptr createPeaksWorkspace(const int numPeaks,
                                                              const Mantid::Kernel::DblMatrix &ubMat) {
  if (ubMat.numRows() != 3 || ubMat.numCols() != 3) {
    throw std::invalid_argument("UB matrix is not 3x3");
  }

  auto peaksWS = createPeaksWorkspace(numPeaks, true);
  peaksWS->mutableSample().getOrientedLattice().setUB(ubMat);
  return peaksWS;
}

Mantid::DataObjects::LeanElasticPeaksWorkspace_sptr createLeanPeaksWorkspace(const int numPeaks,
                                                                             const bool createOrientedLattice) {
  auto peaksWS = std::make_shared<LeanElasticPeaksWorkspace>();
  Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular2(1, 10);
  for (int i = 0; i < numPeaks; ++i) {
    Peak peak(inst, i, i + 0.5);
    LeanElasticPeak lpeak(peak);
    peaksWS->addPeak(lpeak);
  }

  if (createOrientedLattice) {
    peaksWS->mutableSample().setOrientedLattice(std::make_unique<OrientedLattice>());
  }
  return peaksWS;
}

Mantid::DataObjects::LeanElasticPeaksWorkspace_sptr createLeanPeaksWorkspace(const int numPeaks,
                                                                             const Mantid::Kernel::DblMatrix &ubMat) {
  if (ubMat.numRows() != 3 || ubMat.numCols() != 3) {
    throw std::invalid_argument("UB matrix is not 3x3");
  }

  auto peaksWS = createLeanPeaksWorkspace(numPeaks, true);
  peaksWS->mutableSample().getOrientedLattice().setUB(ubMat);
  return peaksWS;
}

void create2DAngles(std::vector<double> &L2, std::vector<double> &polar, std::vector<double> &azim, size_t nPolar,
                    size_t nAzim, double polStart, double polEnd, double azimStart, double azimEnd) {
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

ITableWorkspace_sptr createEPPTableWorkspace(const std::vector<EPPTableRow> &rows) {
  ITableWorkspace_sptr ws = std::make_shared<TableWorkspace>(rows.size());
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
    statusColumn->cell<std::string>(i) = row.fitStatus == EPPTableRow::FitStatus::SUCCESS ? "success" : "failed";
  }
  return ws;
}
Mantid::DataObjects::Workspace2D_sptr create2DWorkspace123WithMaskedBin(int numHist, int numBins,
                                                                        int maskedWorkspaceIndex, int maskedBinIndex) {
  auto ws = create2DWorkspace123(numHist, numBins);
  ws->flagMasked(maskedWorkspaceIndex, maskedBinIndex);
  return ws;
}

MatrixWorkspace_sptr createSNAPLiteInstrument(const std::string &wkspName, const double ang1, const double ang2) {
  // Use lite instrument based on one valid starting 2018-05-01
  const std::string IDF_FILE("SNAPLite_Definition.xml");

  // Create empty instrument
  auto loadEmptyInstr = AlgorithmManager::Instance().createUnmanaged("LoadEmptyInstrument");
  loadEmptyInstr->initialize();
  loadEmptyInstr->setProperty("Filename", IDF_FILE);
  loadEmptyInstr->setProperty("OutputWorkspace", wkspName);

  loadEmptyInstr->execute();
  if (!loadEmptyInstr->isExecuted())
    throw std::runtime_error("Failed to execute LoadEmptyInstrument");

  // add logs
  // for some reason, the units aren't in the logs
  TimeSeriesProperty<double> *ang1Prop = new TimeSeriesProperty<double>("det_arc1");
  ang1Prop->addValue(DateAndTime(0), ang1);
  TimeSeriesProperty<double> *ang2Prop = new TimeSeriesProperty<double>("det_arc2");
  ang2Prop->addValue(DateAndTime(0), ang2);
  TimeSeriesProperty<double> *len1Prop = new TimeSeriesProperty<double>("det_lin1");
  len1Prop->addValue(DateAndTime(0), 0.045);
  TimeSeriesProperty<double> *len2Prop = new TimeSeriesProperty<double>("det_lin2");
  len2Prop->addValue(DateAndTime(0), 0.043);

  MatrixWorkspace_sptr wsIn =
      std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wkspName));
  wsIn->mutableRun().addProperty(ang1Prop);
  wsIn->mutableRun().addProperty(ang2Prop);
  wsIn->mutableRun().addProperty(len1Prop);
  wsIn->mutableRun().addProperty(len2Prop);

  // reload instrument so the logs are used
  auto loadInstr = AlgorithmManager::Instance().createUnmanaged("LoadInstrument");
  loadInstr->initialize();
  loadInstr->setProperty("Workspace", wkspName);
  loadInstr->setProperty("Filename", IDF_FILE);
  loadInstr->setProperty("RewriteSpectraMap", "False");
  loadInstr->execute();
  if (!loadInstr->isExecuted())
    throw std::runtime_error("Failed to execute LoadInstrument");

  // set the units so DiffractionFocussing will do its job
  auto xAxis = wsIn->getAxis(0);
  xAxis->unit() = Mantid::Kernel::UnitFactory::Instance().create("dSpacing");
  return wsIn;
}

MatrixWorkspace_sptr createFocusedSNAPLiteInstrument(const std::string &wkspName, const std::string &groupingAlg,
                                                     const std::string &groupingDescr, const double ang1,
                                                     const double ang2) {
  const std::string GROUP_WS_NAME = "tmpGroupingWorkspace_" + groupingAlg + "_" + groupingDescr;

  // create the full instrument
  MatrixWorkspace_sptr ws = WorkspaceCreationHelper::createSNAPLiteInstrument(wkspName, ang1, ang2);

  // create the groupingworkspace
  auto createGroupingAlg = AlgorithmManager::Instance().createUnmanaged(groupingAlg);
  createGroupingAlg->initialize();
  createGroupingAlg->setProperty("OutputWorkspace", GROUP_WS_NAME);
  if (groupingAlg == "CreateGroupingWorkspace") {
    createGroupingAlg->setProperty("InputWorkspace", ws);
    createGroupingAlg->setProperty("GroupDetectorsBy", groupingDescr);
  } else if (groupingAlg == "LoadDetectorsGroupingFile") {
    createGroupingAlg->setProperty("InputWorkspace", ws);
    createGroupingAlg->setProperty("InputFile", groupingDescr);
  } else {
    throw std::runtime_error("Do not know how to create grouping using \"" + groupingAlg + "\" algorithm");
  }
  createGroupingAlg->execute();
  if (!createGroupingAlg->isExecuted())
    throw std::runtime_error("Failed to execute CreateGroupingWorkspace");

  // focus the data
  auto focusAlg = AlgorithmManager::Instance().createUnmanaged("DiffractionFocussing");
  focusAlg->initialize();
  focusAlg->setProperty("InputWorkspace", wkspName);
  focusAlg->setProperty("OutputWorkspace", wkspName);
  focusAlg->setProperty("GroupingWorkspace", GROUP_WS_NAME);
  focusAlg->execute();
  AnalysisDataService::Instance().remove(GROUP_WS_NAME); // delete the grouping workspace no matter what happened
  if (!focusAlg->isExecuted())
    throw std::runtime_error("Failed to execute DiffractionFocussing");

  return std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wkspName));
}

} // namespace WorkspaceCreationHelper
