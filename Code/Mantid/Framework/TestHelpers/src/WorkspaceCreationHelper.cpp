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
//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"

#include "MantidAPI/Run.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/MersenneTwister.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/VectorHelper.h"

#include <cmath>
#include <sstream>

namespace WorkspaceCreationHelper {
using namespace Mantid;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using Mantid::MantidVec;
using Mantid::MantidVecPtr;

MockAlgorithm::MockAlgorithm(size_t nSteps) {
  m_Progress =
      std::auto_ptr<API::Progress>(new API::Progress(this, 0, 1, nSteps));
}

/**
 * @param name :: The name of the workspace
 * @param ws :: The workspace object
 */
void storeWS(const std::string &name, Mantid::API::Workspace_sptr ws) {
  Mantid::API::AnalysisDataService::Instance().add(name, ws);
}

/**
 * Deletes a workspace
 * @param name :: The name of the workspace
 */
void removeWS(const std::string &name) {
  Mantid::API::AnalysisDataService::Instance().remove(name);
}

Workspace2D_sptr Create1DWorkspaceRand(int size) {
  MantidVecPtr x1, y1, e1;
  x1.access().resize(size, 1);
  y1.access().resize(size);
  std::generate(y1.access().begin(), y1.access().end(), rand);
  e1.access().resize(size);
  std::generate(e1.access().begin(), e1.access().end(), rand);
  Workspace2D_sptr retVal(new Workspace2D);
  retVal->initialize(1, size, size);
  retVal->setX(0, x1);
  retVal->setData(0, y1, e1);
  return retVal;
}

Workspace2D_sptr Create1DWorkspaceConstant(int size, double value,
                                           double error) {
  MantidVecPtr x1, y1, e1;
  x1.access().resize(size, 1);
  y1.access().resize(size);
  std::fill(y1.access().begin(), y1.access().end(), value);
  e1.access().resize(size);
  std::fill(y1.access().begin(), y1.access().end(), error);
  Workspace2D_sptr retVal(new Workspace2D);
  retVal->initialize(1, size, size);
  retVal->setX(0, x1);
  retVal->setData(0, y1, e1);
  return retVal;
}

Workspace2D_sptr Create1DWorkspaceFib(int size) {
  MantidVecPtr x1, y1, e1;
  x1.access().resize(size, 1);
  y1.access().resize(size);
  std::generate(y1.access().begin(), y1.access().end(), FibSeries<double>());
  e1.access().resize(size);
  Workspace2D_sptr retVal(new Workspace2D);
  retVal->initialize(1, size, size);
  retVal->setX(0, x1);
  retVal->setData(0, y1, e1);
  return retVal;
}

Workspace2D_sptr Create2DWorkspace(int nhist, int numBoundaries) {
  return Create2DWorkspaceBinned(nhist, numBoundaries);
}

/** Create a Workspace2D where the Y value at each bin is
 * == to the workspace index
 * @param nhist :: # histograms
 * @param numBoundaries :: # of bins
 * @return Workspace2D
 */
Workspace2D_sptr Create2DWorkspaceWhereYIsWorkspaceIndex(int nhist,
                                                         int numBoundaries) {
  Workspace2D_sptr out = Create2DWorkspaceBinned(nhist, numBoundaries);
  for (int wi = 0; wi < nhist; wi++)
    for (int x = 0; x < numBoundaries; x++)
      out->dataY(wi)[x] = wi * 1.0;
  return out;
}

Workspace2D_sptr create2DWorkspaceThetaVsTOF(int nHist, int nBins) {

  Workspace2D_sptr outputWS = Create2DWorkspaceBinned(nHist, nBins);
  NumericAxis *const newAxis = new NumericAxis(nHist);
  outputWS->replaceAxis(1, newAxis);
  newAxis->unit() = boost::shared_ptr<Unit>(new Units::Degrees);
  for (int i = 0; i < nHist; ++i) {
    newAxis->setValue(i, i + 1);
  }

  return outputWS;
}

Workspace2D_sptr
Create2DWorkspaceWithValues(int64_t nHist, int64_t nBins, bool isHist,
                            const std::set<int64_t> &maskedWorkspaceIndices,
                            double xVal, double yVal, double eVal) {
  MantidVecPtr x1, y1, e1;
  x1.access().resize(isHist ? nBins + 1 : nBins, xVal);
  y1.access().resize(nBins, yVal);
  e1.access().resize(nBins, eVal);
  Workspace2D_sptr retVal(new Workspace2D);
  retVal->initialize(nHist, isHist ? nBins + 1 : nBins, nBins);
  for (int i = 0; i < nHist; i++) {
    retVal->setX(i, x1);
    retVal->setData(i, y1, e1);
    retVal->getSpectrum(i)->setDetectorID(i);
    retVal->getSpectrum(i)->setSpectrumNo(i);
  }
  retVal = maskSpectra(retVal, maskedWorkspaceIndices);
  return retVal;
}

Workspace2D_sptr
Create2DWorkspace123(int64_t nHist, int64_t nBins, bool isHist,
                     const std::set<int64_t> &maskedWorkspaceIndices) {
  return Create2DWorkspaceWithValues(nHist, nBins, isHist,
                                     maskedWorkspaceIndices, 1.0, 2.0, 3.0);
}

Workspace2D_sptr
Create2DWorkspace154(int64_t nHist, int64_t nBins, bool isHist,
                     const std::set<int64_t> &maskedWorkspaceIndices) {
  return Create2DWorkspaceWithValues(nHist, nBins, isHist,
                                     maskedWorkspaceIndices, 1.0, 5.0, 4.0);
}

Workspace2D_sptr maskSpectra(Workspace2D_sptr workspace,
                             const std::set<int64_t> &maskedWorkspaceIndices) {
  const int nhist = static_cast<int>(workspace->getNumberHistograms());
  if (workspace->getInstrument()->nelements() == 0) {
    // We need detectors to be able to mask them.
    boost::shared_ptr<Instrument> instrument(new Instrument);
    workspace->setInstrument(instrument);

    std::string xmlShape = "<sphere id=\"shape\"> ";
    xmlShape += "<centre x=\"0.0\"  y=\"0.0\" z=\"0.0\" /> ";
    xmlShape += "<radius val=\"0.05\" /> ";
    xmlShape += "</sphere>";
    xmlShape += "<algebra val=\"shape\" /> ";

    ShapeFactory sFactory;
    boost::shared_ptr<Object> shape = sFactory.createShape(xmlShape);
    for (int i = 0; i < nhist; ++i) {
      Detector *det = new Detector("det", detid_t(i), shape, NULL);
      det->setPos(i, i + 1, 1);
      instrument->add(det);
      instrument->markAsDetector(det);
    }
    workspace->setInstrument(instrument);
  }

  ParameterMap &pmap = workspace->instrumentParameters();
  for (int i = 0; i < nhist; ++i) {
    if (maskedWorkspaceIndices.find(i) != maskedWorkspaceIndices.end()) {
      IDetector_const_sptr det = workspace->getDetector(i);
      pmap.addBool(det.get(), "masked", true);
    }
  }
  return workspace;
}

/**
 * Create a group with nEntries. It is added to the ADS with the given stem
 */
WorkspaceGroup_sptr CreateWorkspaceGroup(int nEntries, int nHist, int nBins,
                                         const std::string &stem) {
  WorkspaceGroup_sptr group(new WorkspaceGroup);
  AnalysisDataService::Instance().add(stem, group);
  for (int i = 0; i < nEntries; ++i) {
    Workspace2D_sptr ws = Create2DWorkspace(nHist, nBins);
    std::ostringstream os;
    os << stem << "_" << i;
    AnalysisDataService::Instance().add(os.str(), ws);
    group->add(os.str());
  }
  return group;
}

/** Create a 2D workspace with this many histograms and bins.
 * Filled with Y = 2.0 and E = sqrt(2.0)w
 */
Workspace2D_sptr Create2DWorkspaceBinned(int nhist, int nbins, double x0,
                                         double deltax) {
  MantidVecPtr x, y, e;
  x.access().resize(nbins + 1);
  y.access().resize(nbins, 2);
  e.access().resize(nbins, sqrt(2.0));
  for (int i = 0; i < nbins + 1; ++i) {
    x.access()[i] = x0 + i * deltax;
  }
  Workspace2D_sptr retVal(new Workspace2D);
  retVal->initialize(nhist, nbins + 1, nbins);
  for (int i = 0; i < nhist; i++) {
    retVal->setX(i, x);
    retVal->setData(i, y, e);
  }
  return retVal;
}

/** Create a 2D workspace with this many histograms and bins. The bins are
 * assumed to be non-uniform and given by the input array
 * Filled with Y = 2.0 and E = sqrt(2.0)w
 */
Workspace2D_sptr Create2DWorkspaceBinned(int nhist, const int numBoundaries,
                                         const double xBoundaries[]) {
  MantidVecPtr x, y, e;
  const int numBins = numBoundaries - 1;
  x.access().resize(numBoundaries);
  y.access().resize(numBins, 2);
  e.access().resize(numBins, sqrt(2.0));
  for (int i = 0; i < numBoundaries; ++i) {
    x.access()[i] = xBoundaries[i];
  }
  Workspace2D_sptr retVal(new Workspace2D);
  retVal->initialize(nhist, numBins + 1, numBins);
  for (int i = 0; i < nhist; i++) {
    retVal->setX(i, x);
    retVal->setData(i, y, e);
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
    Mantid::MantidVec &Y = ws->dataY(iSpec);
    Mantid::MantidVec &E = ws->dataE(iSpec);
    for (size_t i = 0; i < Y.size(); i++) {
      Y[i] += noise * randGen.nextValue();
      E[i] += noise;
    }
  }
}

//================================================================================================================
/**
 * Create a test workspace with a fully defined instrument
 * Each spectra will have a cylindrical detector defined 2*cylinder_radius away
 * from the centre of the
 * previous.
 * Data filled with: Y: 2.0, E: sqrt(2.0), X: nbins of width 1 starting at 0
 */
Workspace2D_sptr
create2DWorkspaceWithFullInstrument(int nhist, int nbins, bool includeMonitors,
                                    bool startYNegative, bool isHistogram,
                                    const std::string &instrumentName) {
  if (includeMonitors && nhist < 2) {
    throw std::invalid_argument("Attempting to 2 include monitors for a "
                                "workspace with fewer than 2 histograms");
  }

  Workspace2D_sptr space;
  if (isHistogram)
    space = Create2DWorkspaceBinned(
        nhist, nbins); // A 1:1 spectra is created by default
  else
    space = Create2DWorkspace123(nhist, nbins, false);
  space->setTitle(
      "Test histogram"); // actually adds a property call run_title to the logs
  space->getAxis(0)->setUnit("TOF");
  space->setYUnit("Counts");

  boost::shared_ptr<Instrument> testInst(new Instrument(instrumentName));
  testInst->setReferenceFrame(
      boost::shared_ptr<ReferenceFrame>(new ReferenceFrame(Y, X, Left, "")));
  space->setInstrument(testInst);

  const double pixelRadius(0.05);
  Object_sptr pixelShape = ComponentCreationHelper::createCappedCylinder(
      pixelRadius, 0.02, V3D(0.0, 0.0, 0.0), V3D(0., 1.0, 0.), "tube");

  const double detXPos(5.0);
  int ndets = nhist;
  if (includeMonitors)
    ndets -= 2;
  for (int i = 0; i < ndets; ++i) {
    std::ostringstream lexer;
    lexer << "pixel-" << i << ")";
    Detector *physicalPixel =
        new Detector(lexer.str(), space->getAxis(1)->spectraNo(i), pixelShape,
                     testInst.get());
    int ycount(i);
    if (startYNegative)
      ycount -= 1;
    const double ypos = ycount * 2.0 * pixelRadius;
    physicalPixel->setPos(detXPos, ypos, 0.0);
    testInst->add(physicalPixel);
    testInst->markAsDetector(physicalPixel);
    space->getSpectrum(i)->addDetectorID(physicalPixel->getID());
  }

  // Monitors last
  if (includeMonitors) // These occupy the last 2 spectra
  {
    Detector *monitor1 =
        new Detector("mon1", space->getAxis(1)->spectraNo(ndets), Object_sptr(),
                     testInst.get());
    monitor1->setPos(-9.0, 0.0, 0.0);
    testInst->add(monitor1);
    testInst->markAsMonitor(monitor1);

    Detector *monitor2 =
        new Detector("mon2", space->getAxis(1)->spectraNo(ndets) + 1,
                     Object_sptr(), testInst.get());
    monitor2->setPos(-2.0, 0.0, 0.0);
    testInst->add(monitor2);
    testInst->markAsMonitor(monitor2);
  }

  // Define a source and sample position
  // Define a source component
  ObjComponent *source =
      new ObjComponent("moderator", Object_sptr(), testInst.get());
  source->setPos(V3D(-20, 0.0, 0.0));
  testInst->add(source);
  testInst->markAsSource(source);

  // Define a sample as a simple sphere
  ObjComponent *sample =
      new ObjComponent("samplePos", Object_sptr(), testInst.get());
  testInst->setPos(0.0, 0.0, 0.0);
  testInst->add(sample);
  testInst->markAsSamplePos(sample);

  return space;
}

//================================================================================================================
/** Create an Workspace2D with an instrument that contains
 *RectangularDetector's.
 * Bins will be 0.0, 1.0, to numBins, filled with signal=2.0, sqrt(2.0)
 *
 * @param numBanks :: number of rectangular banks
 * @param numPixels :: each bank will be numPixels*numPixels
 * @param numBins :: each spectrum will have this # of bins
 * @return The EventWorkspace
 */
Mantid::DataObjects::Workspace2D_sptr
create2DWorkspaceWithRectangularInstrument(int numBanks, int numPixels,
                                           int numBins) {
  Instrument_sptr inst =
      ComponentCreationHelper::createTestInstrumentRectangular(numBanks,
                                                               numPixels);
  Workspace2D_sptr ws =
      Create2DWorkspaceBinned(numBanks * numPixels * numPixels, numBins);
  ws->setInstrument(inst);
  ws->getAxis(0)->setUnit("dSpacing");
  for (size_t wi = 0; wi < ws->getNumberHistograms(); wi++) {
    ws->getSpectrum(wi)->setDetectorID(detid_t(numPixels * numPixels + wi));
    ws->getSpectrum(wi)->setSpectrumNo(specid_t(wi));
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
      CreateEventWorkspace2(numBanks * numPixels * numPixels, 100);
  ws->setInstrument(inst);

  // Set the X axes
  MantidVec x = ws->readX(0);
  NumericAxis *ax0 = new NumericAxis(x.size());
  ax0->setUnit("dSpacing");
  for (size_t i = 0; i < x.size(); i++)
    ax0->setValue(i, x[i]);
  ws->replaceAxis(0, ax0);

  // re-assign detector IDs to the rectangular detector
  int detID = numPixels * numPixels;
  for (int wi = 0; wi < static_cast<int>(ws->getNumberHistograms()); wi++) {
    ws->getEventList(wi).clearDetectorIDs();
    if (clearEvents)
      ws->getEventList(wi).clear(true);
    ws->getEventList(wi).setDetectorID(detID);
    detID++;
  }
  return ws;
}

Mantid::DataObjects::EventWorkspace_sptr
createEventWorkspaceWithNonUniformInstrument(int numBanks, bool clearEvents) {
  // Number of detectors in a bank as created by createTestInstrumentCylindrical
  const int DETECTORS_PER_BANK(9);

  Instrument_sptr inst =
      ComponentCreationHelper::createTestInstrumentCylindrical(numBanks, false,
                                                               0.0025, 0.005);
  EventWorkspace_sptr ws =
      CreateEventWorkspace2(numBanks * DETECTORS_PER_BANK, 100);
  ws->setInstrument(inst);

  std::vector<detid_t> detectorIds = inst->getDetectorIDs();

  // Should be equal if DETECTORS_PER_BANK is correct
  assert(detectorIds.size() == ws->getNumberHistograms());

  // Re-assign detector IDs
  for (size_t wi = 0; wi < ws->getNumberHistograms(); wi++) {
    ws->getEventList(wi).clearDetectorIDs();
    if (clearEvents)
      ws->getEventList(wi).clear(true);
    ws->getEventList(wi).setDetectorID(detectorIds[wi]);
  }

  return ws;
}

/**
 * Create a very small 2D workspace for a virtual reflectometry instrument.
 * @return workspace with instrument attached.
 * @param startX : X Tof start value for the workspace.
 */
MatrixWorkspace_sptr
create2DWorkspaceWithReflectometryInstrument(double startX) {
  Instrument_sptr instrument = boost::make_shared<Instrument>();
  instrument->setReferenceFrame(
      boost::make_shared<ReferenceFrame>(Y, X, Left, "0,0,0"));

  ObjComponent *source = new ObjComponent("source");
  source->setPos(V3D(0, 0, 0));
  instrument->add(source);
  instrument->markAsSource(source);

  Detector *monitor = new Detector("Monitor", 1, NULL);
  monitor->setPos(14, 0, 0);
  instrument->add(monitor);
  instrument->markAsMonitor(monitor);

  ObjComponent *sample = new ObjComponent("some-surface-holder");
  source->setPos(V3D(15, 0, 0));
  instrument->add(sample);
  instrument->markAsSamplePos(sample);

  Detector *det = new Detector("point-detector", 2, NULL);
  det->setPos(20, (20 - sample->getPos().X()), 0);
  instrument->add(det);
  instrument->markAsDetector(det);

  const int nSpectra = 2;
  const int nBins = 100;
  const double deltaX = 2000; // TOF
  auto workspace = Create2DWorkspaceBinned(nSpectra, nBins, startX, deltaX);

  workspace->setTitle(
      "Test histogram"); // actually adds a property call run_title to the logs
  workspace->getAxis(0)->setUnit("TOF");
  workspace->setYUnit("Counts");

  workspace->setInstrument(instrument);

  workspace->getSpectrum(0)->addDetectorID(det->getID());
  workspace->getSpectrum(1)->addDetectorID(monitor->getID());
  return workspace;
}

void createInstrumentForWorkspaceWithDistances(
    MatrixWorkspace_sptr workspace, const V3D &samplePosition,
    const V3D &sourcePosition, const std::vector<V3D> &detectorPositions) {
  Instrument_sptr instrument = boost::make_shared<Instrument>();
  instrument->setReferenceFrame(
      boost::make_shared<ReferenceFrame>(Y, X, Left, "0,0,0"));

  ObjComponent *source = new ObjComponent("source");
  source->setPos(sourcePosition);
  instrument->add(source);
  instrument->markAsSource(source);

  ObjComponent *sample = new ObjComponent("sample");
  source->setPos(samplePosition);
  instrument->add(sample);
  instrument->markAsSamplePos(sample);

  workspace->setInstrument(instrument);

  for (int i = 0; i < static_cast<int>(detectorPositions.size()); ++i) {
    std::stringstream buffer;
    buffer << "detector_" << i;
    Detector *det = new Detector(buffer.str(), i, NULL);
    det->setPos(detectorPositions[i]);
    instrument->add(det);
    instrument->markAsDetector(det);

    // Link it to the workspace
    workspace->getSpectrum(i)->addDetectorID(det->getID());
  }
}

//================================================================================================================
WorkspaceSingleValue_sptr CreateWorkspaceSingleValue(double value) {
  WorkspaceSingleValue_sptr retVal(
      new WorkspaceSingleValue(value, sqrt(value)));
  return retVal;
}

WorkspaceSingleValue_sptr CreateWorkspaceSingleValueWithError(double value,
                                                              double error) {
  WorkspaceSingleValue_sptr retVal(new WorkspaceSingleValue(value, error));
  return retVal;
}

/** Perform some finalization on event workspace stuff */
void EventWorkspace_Finalize(EventWorkspace_sptr ew) {
  // get a proton charge
  ew->mutableRun().integrateProtonCharge();
}

/** Create event workspace with:
 * 500 pixels
 * 1000 histogrammed bins.
 */
EventWorkspace_sptr CreateEventWorkspace() {
  return CreateEventWorkspace(500, 1001, 100, 1000);
}

/** Create event workspace with:
 * numPixels pixels
 * numBins histogrammed bins from 0.0 in steps of 1.0
 * 200 events; two in each bin, at time 0.5, 1.5, etc.
 * PulseTime = 0 second x2, 1 second x2, 2 seconds x2, etc. after 2010-01-01
 */
EventWorkspace_sptr CreateEventWorkspace2(int numPixels, int numBins) {
  return CreateEventWorkspace(numPixels, numBins, 100, 0.0, 1.0, 2);
}

/** Create event workspace
 */
EventWorkspace_sptr CreateEventWorkspace(int numPixels, int numBins,
                                         int numEvents, double x0,
                                         double binDelta, int eventPattern,
                                         int start_at_pixelID) {
  DateAndTime run_start("2010-01-01T00:00:00");

  // add one to the number of bins as this is histogram
  numBins++;

  EventWorkspace_sptr retVal(new EventWorkspace);
  retVal->initialize(numPixels, 1, 1);

  // Make fake events
  if (eventPattern) // 0 == no events
  {
    size_t workspaceIndex = 0;
    for (int pix = start_at_pixelID + 0; pix < start_at_pixelID + numPixels;
         pix++) {
      EventList &el = retVal->getEventList(workspaceIndex);
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

  // Create the x-axis for histogramming.
  MantidVecPtr x1;
  MantidVec &xRef = x1.access();
  xRef.resize(numBins);
  for (int i = 0; i < numBins; ++i) {
    xRef[i] = x0 + i * binDelta;
  }

  // Set all the histograms at once.
  retVal->setAllX(x1);

  return retVal;
}

// =====================================================================================
/** Create event workspace, with several detector IDs in one event list.
 */
EventWorkspace_sptr
CreateGroupedEventWorkspace(std::vector<std::vector<int>> groups, int numBins,
                            double binDelta) {

  EventWorkspace_sptr retVal(new EventWorkspace);
  retVal->initialize(1, 2, 1);

  for (size_t g = 0; g < groups.size(); g++) {
    retVal->getOrAddEventList(g).clearDetectorIDs();
    std::vector<int> dets = groups[g];
    for (std::vector<int>::iterator it = dets.begin(); it != dets.end(); ++it) {
      for (int i = 0; i < numBins; i++)
        retVal->getOrAddEventList(g) += TofEvent((i + 0.5) * binDelta, 1);
      retVal->getOrAddEventList(g).addDetectorID(*it);
    }
  }

  // Create the x-axis for histogramming.
  MantidVecPtr x1;
  MantidVec &xRef = x1.access();
  double x0 = 0;
  xRef.resize(numBins);
  for (int i = 0; i < numBins; ++i) {
    xRef[i] = x0 + i * binDelta;
  }

  // Set all the histograms at once.
  retVal->setAllX(x1);

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
EventWorkspace_sptr CreateRandomEventWorkspace(size_t numbins, size_t numpixels,
                                               double bin_delta) {
  EventWorkspace_sptr retVal(new EventWorkspace);
  retVal->initialize(numpixels, numbins, numbins - 1);

  // and X-axis for references:
  NumericAxis *pAxis0 = new NumericAxis(numbins);

  // Create the original X axis to histogram on.
  // Create the x-axis for histogramming.
  Kernel::cow_ptr<MantidVec> axis;
  MantidVec &xRef = axis.access();
  xRef.resize(numbins);
  for (int i = 0; i < static_cast<int>(numbins); ++i) {
    xRef[i] = i * bin_delta;
    pAxis0->setValue(i, xRef[i]);
  }
  pAxis0->setUnit("TOF");

  // Make up some data for each pixels
  for (size_t i = 0; i < numpixels; i++) {
    // Create one event for each bin
    EventList &events = retVal->getEventList(static_cast<detid_t>(i));
    for (double ie = 0; ie < numbins; ie++) {
      // Create a list of events, randomize
      events += TofEvent(std::rand(), std::rand());
    }
    events.addDetectorID(detid_t(i));
  }
  retVal->setAllX(axis);
  retVal->replaceAxis(0, pAxis0);

  return retVal;
}

// =====================================================================================
/** Create Workspace2d, with numHist spectra, each with 9 detectors,
 * with IDs 1-9, 10-18, 19-27
 */
MatrixWorkspace_sptr CreateGroupedWorkspace2D(size_t numHist, int numBins,
                                              double binDelta) {
  Workspace2D_sptr retVal = Create2DWorkspaceBinned(static_cast<int>(numHist),
                                                    numBins, 0.0, binDelta);
  retVal->setInstrument(
      ComponentCreationHelper::createTestInstrumentCylindrical(
          static_cast<int>(numHist)));

  for (int g = 0; g < static_cast<int>(numHist); g++) {
    ISpectrum *spec = retVal->getSpectrum(g);
    for (int i = 1; i <= 9; i++)
      spec->addDetectorID(g * 9 + i);
    spec->setSpectrumNo(g + 1); // Match detector ID and spec NO
  }
  return boost::dynamic_pointer_cast<MatrixWorkspace>(retVal);
}

// =====================================================================================
// RootOfNumHist == square root of hystohram number;
MatrixWorkspace_sptr
CreateGroupedWorkspace2DWithRingsAndBoxes(size_t RootOfNumHist, int numBins,
                                          double binDelta) {
  size_t numHist = RootOfNumHist * RootOfNumHist;
  Workspace2D_sptr retVal = Create2DWorkspaceBinned(static_cast<int>(numHist),
                                                    numBins, 0.0, binDelta);
  retVal->setInstrument(
      ComponentCreationHelper::createTestInstrumentCylindrical(
          static_cast<int>(numHist)));
  for (int g = 0; g < static_cast<int>(numHist); g++) {
    ISpectrum *spec = retVal->getSpectrum(g);
    for (int i = 1; i <= 9; i++)
      spec->addDetectorID(g * 9 + i);
    spec->setSpectrumNo(g + 1); // Match detector ID and spec NO
  }
  return boost::dynamic_pointer_cast<MatrixWorkspace>(retVal);
}

// not strictly creating a workspace, but really helpfull to see what one
// contains
void DisplayDataY(const MatrixWorkspace_sptr ws) {
  const size_t numHists = ws->getNumberHistograms();
  for (size_t i = 0; i < numHists; ++i) {
    std::cout << "Histogram " << i << " = ";
    for (size_t j = 0; j < ws->blocksize(); ++j) {
      std::cout << ws->readY(i)[j] << " ";
    }
    std::cout << std::endl;
  }
}
void DisplayData(const MatrixWorkspace_sptr ws) { DisplayDataX(ws); }

// not strictly creating a workspace, but really helpfull to see what one
// contains
void DisplayDataX(const MatrixWorkspace_sptr ws) {
  const size_t numHists = ws->getNumberHistograms();
  for (size_t i = 0; i < numHists; ++i) {
    std::cout << "Histogram " << i << " = ";
    for (size_t j = 0; j < ws->blocksize(); ++j) {
      std::cout << ws->readX(i)[j] << " ";
    }
    std::cout << std::endl;
  }
}

// not strictly creating a workspace, but really helpfull to see what one
// contains
void DisplayDataE(const MatrixWorkspace_sptr ws) {
  const size_t numHists = ws->getNumberHistograms();
  for (size_t i = 0; i < numHists; ++i) {
    std::cout << "Histogram " << i << " = ";
    for (size_t j = 0; j < ws->blocksize(); ++j) {
      std::cout << ws->readE(i)[j] << " ";
    }
    std::cout << std::endl;
  }
}

// =====================================================================================
/** Utility function to add a TimeSeriesProperty with a name and value
 *
 * @param runInfo :: Run to add to
 * @param name :: property name
 * @param val :: value
 */
void AddTSPEntry(Run &runInfo, std::string name, double val) {
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
void SetOrientedLattice(Mantid::API::MatrixWorkspace_sptr ws, double a,
                        double b, double c) {
  OrientedLattice *latt = new OrientedLattice(a, b, c, 90., 90., 90.);
  ws->mutableSample().setOrientedLattice(latt);
  delete latt;
}

// =====================================================================================
/** Create a default universal goniometer and set its angles
 *
 * @param ws :: workspace to set
 * @param phi :: +Y rotation angle (deg)
 * @param chi :: +X rotation angle (deg)
 * @param omega :: +Y rotation angle (deg)
 */
void SetGoniometer(Mantid::API::MatrixWorkspace_sptr ws, double phi, double chi,
                   double omega) {
  AddTSPEntry(ws->mutableRun(), "phi", phi);
  AddTSPEntry(ws->mutableRun(), "chi", chi);
  AddTSPEntry(ws->mutableRun(), "omega", omega);
  Mantid::Geometry::Goniometer gm;
  gm.makeUniversalGoniometer();
  ws->mutableRun().setGoniometer(gm, true);
}

//
Mantid::API::MatrixWorkspace_sptr
createProcessedWorkspaceWithCylComplexInstrument(size_t numPixels,
                                                 size_t numBins,
                                                 bool has_oriented_lattice) {
  size_t rHist = (size_t)sqrt(double(numPixels));
  while (rHist * rHist < numPixels)
    rHist++;

  Mantid::API::MatrixWorkspace_sptr ws =
      CreateGroupedWorkspace2DWithRingsAndBoxes(rHist, 10, 0.1);
  NumericAxis *pAxis0 = new NumericAxis(numBins);

  for (size_t i = 0; i < numBins; i++) {
    double dE = -1.0 + static_cast<double>(i) * 0.8;
    pAxis0->setValue(i, dE);
  }
  pAxis0->setUnit("DeltaE");
  ws->replaceAxis(0, pAxis0);
  if (has_oriented_lattice) {
    OrientedLattice *latt = new OrientedLattice(1, 1, 1, 90., 90., 90.);
    ws->mutableSample().setOrientedLattice(latt);
    delete latt;

    AddTSPEntry(ws->mutableRun(), "phi", 0);
    AddTSPEntry(ws->mutableRun(), "chi", 0);
    AddTSPEntry(ws->mutableRun(), "omega", 0);
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
      Create2DWorkspaceWithValues(uint64_t(numPixels), uint64_t(numBins), true,
                                  maskedWorkspaceIndices, 0, 1, 0.1);

  // detectors at L2, sample at 0 and source at -L2_min
  ws->setInstrument(
      ComponentCreationHelper::createCylInstrumentWithDetInGivenPosisions(
          L2, polar, azimutal));

  for (int g = 0; g < static_cast<int>(numPixels); g++) {
    ISpectrum *spec = ws->getSpectrum(g);
    // we just made (in createCylInstrumentWithDetInGivenPosisions) det ID-s to
    // start from 1
    spec->setDetectorID(g + 1);
    // and this is absolutely different nummer, corresponding to det ID just by
    // chance ? -- some uncertainties remain
    spec->setSpectrumNo(g + 1);
    // spec->setSpectrumNo(g+1);
    //   spec->addDetectorID(g*9);
    //   spec->setSpectrumNo(g+1); // Match detector ID and spec NO
  }

  double dE = (Emax - Emin) / double(numBins);
  for (size_t j = 0; j < numPixels; j++) {

    MantidVec &E_transfer = ws->dataX(j);
    for (size_t i = 0; i <= numBins; i++) {
      double E = Emin + static_cast<double>(i) * dE;
      E_transfer[i] = E;
    }
  }
  // set axis, correspondent to the X-values
  NumericAxis *pAxis0 = new NumericAxis(numBins);
  MantidVec &E_transfer = ws->dataX(0);
  for (size_t i = 0; i < numBins; i++) {
    double E = 0.5 * (E_transfer[i] + E_transfer[i + 1]);
    pAxis0->setValue(i, E);
  }

  pAxis0->setUnit("DeltaE");

  ws->replaceAxis(0, pAxis0);

  // define oriented lattice which requested for processed ws
  OrientedLattice *latt = new OrientedLattice(1, 1, 1, 90., 90., 90.);
  ws->mutableSample().setOrientedLattice(latt);
  delete latt;

  // TODO: clarify if this property indeed goes there;
  ws->mutableRun().addProperty(new PropertyWithValue<double>("Ei", Ei), true);
  // these properties have to be different -> specific for processed ws, as time
  // now should be reconciled
  AddTSPEntry(ws->mutableRun(), "phi", 0);
  AddTSPEntry(ws->mutableRun(), "chi", 0);
  AddTSPEntry(ws->mutableRun(), "omega", 0);
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
  // outputWS->setName(wsname);
  outputWS->initialize(1, 1, 1);

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
  loadInst->setProperty("RewriteSpectraMap", true);
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
  std::vector<std::size_t> pixel_to_wkspindex;
  pixel_to_wkspindex.reserve(
      detid_max + 1); // starting at zero up to and including detid_max
  pixel_to_wkspindex.assign(detid_max + 1, 0);
  size_t workspaceIndex = 0;
  for (it = detector_map.begin(); it != detector_map.end(); ++it) {
    if (!it->second->isMonitor()) {
      pixel_to_wkspindex[it->first] = workspaceIndex;
      DataObjects::EventList &spec =
          outputWS->getOrAddEventList(workspaceIndex);
      spec.addDetectorID(it->first);
      // Start the spectrum number at 1
      spec.setSpectrumNo(specid_t(workspaceIndex + 1));
      workspaceIndex += 1;
    }
  }

  // Clear
  pixel_to_wkspindex.clear();

  return outputWS;
}

RebinnedOutput_sptr CreateRebinnedOutputWorkspace() {
  RebinnedOutput_sptr outputWS =
      Mantid::DataObjects::RebinnedOutput_sptr(new RebinnedOutput());
  // outputWS->setName("rebinTest");
  Mantid::API::AnalysisDataService::Instance().add("rebinTest", outputWS);

  // Set Q ('y') axis binning
  MantidVec qbins;
  qbins.push_back(0.0);
  qbins.push_back(1.0);
  qbins.push_back(4.0);
  MantidVec qaxis;
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

  // Create the x-axis for histogramming.
  MantidVecPtr x1;
  MantidVec &xRef = x1.access();
  double x0 = -3;
  xRef.resize(numX);
  for (int i = 0; i < numX; ++i) {
    xRef[i] = x0 + i;
  }

  // Create a numeric axis to replace the default vertical one
  Axis *const verticalAxis = new NumericAxis(numY);
  outputWS->replaceAxis(1, verticalAxis);

  // Now set the axis values
  for (int i = 0; i < numHist; ++i) {
    outputWS->setX(i, x1);
    verticalAxis->setValue(i, qaxis[i]);
  }
  // One more to set on the 'y' axis
  verticalAxis->setValue(numHist, qaxis[numHist]);

  // Set the 'y' axis units
  verticalAxis->unit() = UnitFactory::Instance().create("MomentumTransfer");
  verticalAxis->title() = "|Q|";

  // Set the X axis title (for conversion to MD)
  outputWS->getAxis(0)->title() = "Energy transfer";

  // Now, setup the data
  // Q bin #1
  outputWS->dataY(0)[1] = 2.0;
  outputWS->dataY(0)[2] = 3.0;
  outputWS->dataY(0)[3] = 3.0;
  outputWS->dataY(0)[4] = 2.0;
  outputWS->dataE(0)[1] = 2.0;
  outputWS->dataE(0)[2] = 3.0;
  outputWS->dataE(0)[3] = 3.0;
  outputWS->dataE(0)[4] = 2.0;
  outputWS->dataF(0)[1] = 2.0;
  outputWS->dataF(0)[2] = 3.0;
  outputWS->dataF(0)[3] = 3.0;
  outputWS->dataF(0)[4] = 1.0;
  // Q bin #2
  outputWS->dataY(1)[1] = 1.0;
  outputWS->dataY(1)[2] = 3.0;
  outputWS->dataY(1)[3] = 3.0;
  outputWS->dataY(1)[4] = 2.0;
  outputWS->dataY(1)[5] = 2.0;
  outputWS->dataE(1)[1] = 1.0;
  outputWS->dataE(1)[2] = 3.0;
  outputWS->dataE(1)[3] = 3.0;
  outputWS->dataE(1)[4] = 2.0;
  outputWS->dataE(1)[5] = 2.0;
  outputWS->dataF(1)[1] = 1.0;
  outputWS->dataF(1)[2] = 3.0;
  outputWS->dataF(1)[3] = 3.0;
  outputWS->dataF(1)[4] = 1.0;
  outputWS->dataF(1)[5] = 2.0;
  // Q bin #3
  outputWS->dataY(2)[1] = 1.0;
  outputWS->dataY(2)[2] = 2.0;
  outputWS->dataY(2)[3] = 3.0;
  outputWS->dataY(2)[4] = 1.0;
  outputWS->dataE(2)[1] = 1.0;
  outputWS->dataE(2)[2] = 2.0;
  outputWS->dataE(2)[3] = 3.0;
  outputWS->dataE(2)[4] = 1.0;
  outputWS->dataF(2)[1] = 1.0;
  outputWS->dataF(2)[2] = 2.0;
  outputWS->dataF(2)[3] = 2.0;
  outputWS->dataF(2)[4] = 1.0;
  // Q bin #4
  outputWS->dataY(3)[0] = 1.0;
  outputWS->dataY(3)[1] = 2.0;
  outputWS->dataY(3)[2] = 3.0;
  outputWS->dataY(3)[3] = 2.0;
  outputWS->dataY(3)[4] = 1.0;
  outputWS->dataE(3)[0] = 1.0;
  outputWS->dataE(3)[1] = 2.0;
  outputWS->dataE(3)[2] = 3.0;
  outputWS->dataE(3)[3] = 2.0;
  outputWS->dataE(3)[4] = 1.0;
  outputWS->dataF(3)[0] = 1.0;
  outputWS->dataF(3)[1] = 2.0;
  outputWS->dataF(3)[2] = 3.0;
  outputWS->dataF(3)[3] = 2.0;
  outputWS->dataF(3)[4] = 1.0;
  outputWS->dataF(3)[5] = 1.0;

  // Set representation
  outputWS->finalize();

  // Make errors squared rooted
  for (int i = 0; i < numHist; ++i) {
    for (int j = 0; j < numX - 1; ++j) {
      outputWS->dataE(i)[j] = std::sqrt(outputWS->dataE(i)[j]);
    }
  }

  return outputWS;
}

Mantid::DataObjects::PeaksWorkspace_sptr
createPeaksWorkspace(const int numPeaks) {
  PeaksWorkspace_sptr peaksWS(new PeaksWorkspace());
  Instrument_sptr inst =
      ComponentCreationHelper::createTestInstrumentRectangular2(1, 10);
  peaksWS->setInstrument(inst);

  for (int i = 0; i < numPeaks; ++i) {
    Peak peak(inst, i, i + 0.5);
    peaksWS->addPeak(peak);
  }

  Mantid::Geometry::OrientedLattice lattice;
  peaksWS->mutableSample().setOrientedLattice(&lattice);
  return peaksWS;
}

/** helper method to create preprocessed detector's table workspace */
boost::shared_ptr<DataObjects::TableWorkspace>
createTableWorkspace(const API::MatrixWorkspace_const_sptr &inputWS) {
  const size_t nHist = inputWS->getNumberHistograms();

  // set the target workspace
  auto targWS = boost::shared_ptr<TableWorkspace>(new TableWorkspace(nHist));
  // detectors positions
  if (!targWS->addColumn("V3D", "DetDirections"))
    throw(std::runtime_error("Can not add column DetDirectrions"));
  // sample-detector distance;
  if (!targWS->addColumn("double", "L2"))
    throw(std::runtime_error("Can not add column L2"));
  // Diffraction angle
  if (!targWS->addColumn("double", "TwoTheta"))
    throw(std::runtime_error("Can not add column TwoTheta"));
  if (!targWS->addColumn("double", "Azimuthal"))
    throw(std::runtime_error("Can not add column Azimuthal"));
  // the detector ID;
  if (!targWS->addColumn("int", "DetectorID"))
    throw(std::runtime_error("Can not add column DetectorID"));
  // stores spectra index which corresponds to a valid detector index;
  if (!targWS->addColumn("size_t", "detIDMap"))
    throw(std::runtime_error("Can not add column detIDMap"));
  // stores detector index which corresponds to the workspace index;
  if (!targWS->addColumn("size_t", "spec2detMap"))
    throw(std::runtime_error("Can not add column spec2detMap"));

  // will see about that
  // sin^2(Theta)
  //    std::vector<double>      SinThetaSq;

  //,"If the detectors were actually processed from real instrument or generated
  // for some fake one ");
  return targWS;
}

/** method does preliminary calculations of the detectors positions to convert
 results into k-dE space ;
 and places the resutls into static cash to be used in subsequent calls to this
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
        "Instrubment not sufficiently defined: failed to get source and/or "
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
  for (size_t i = 0; i < nHist; i++) {
    sp2detMap[i] = std::numeric_limits<size_t>::quiet_NaN();
    detId[i] = std::numeric_limits<int32_t>::quiet_NaN();
    detIDMap[i] = std::numeric_limits<size_t>::quiet_NaN();
    L2[i] = std::numeric_limits<double>::quiet_NaN();
    TwoTheta[i] = std::numeric_limits<double>::quiet_NaN();
    Azimuthal[i] = std::numeric_limits<double>::quiet_NaN();

    // get detector or detector group which corresponds to the spectra i
    Geometry::IDetector_const_sptr spDet;
    try {
      spDet = inputWS->getDetector(i);
    } catch (Kernel::Exception::NotFoundError &) {
      continue;
    }

    // Check that we aren't dealing with monitor...
    if (spDet->isMonitor())
      continue;

    // calculate the requested values;
    sp2detMap[i] = liveDetectorsCount;
    detId[liveDetectorsCount] = int32_t(spDet->getID());
    detIDMap[liveDetectorsCount] = i;
    L2[liveDetectorsCount] = spDet->getDistance(*sample);

    double polar = inputWS->detectorTwoTheta(spDet);
    double azim = spDet->getPhi();
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
}
