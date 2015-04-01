#include "MantidMDAlgorithms/ConvToMDHistoWS.h"

namespace Mantid {
namespace MDAlgorithms {
// service variable used for efficient filling of the MD event WS  -> should be
// moved to configuration?
#define DATA_BUFFER_SIZE 8192
/// Template to check if a variable equal to NaN
template <class T> inline bool isNaN(T val) {
  volatile T buf = val;
  return (val != buf);
}

/** method sets up all internal variables necessary to convert from Matrix2D
workspace to MDEvent workspace
@param WSD         -- the class describing the target MD workspace, sorurce
matrtix workspace and the transformations, necessary to perform on these
workspaces
@param inWSWrapper -- the class wrapping the target MD workspace
@param ignoreZeros  -- if zero value signals should be rejected
*/
size_t ConvToMDHistoWS::initialize(
    const MDWSDescription &WSD,
    boost::shared_ptr<MDEventWSWrapper> inWSWrapper,
    bool ignoreZeros) {

  size_t numSpec = ConvToMDBase::initialize(WSD, inWSWrapper, ignoreZeros);

  // check if we indeed have matrix workspace as input.
  DataObjects::Workspace2D_const_sptr pWS2D =
      boost::dynamic_pointer_cast<const DataObjects::Workspace2D>(m_InWS2D);
  if (!pWS2D)
    throw(std::logic_error(
        "ConvToDataObjectsHisto should work with defined histrogram workspace"));

  return numSpec;
}

/** convert range of spectra starting from initial spectra  startSpectra into MD
*events
*@param startSpectra -- initial spectra number to begin conversion from
*
* @returns -- number of events added to the workspace.
*/
size_t ConvToMDHistoWS::conversionChunk(size_t startSpectra) {
  size_t nAddedEvents(0), nBufEvents(0);
  // cache global variable locally
  bool ignoreZeros(m_ignoreZeros);

  const size_t specSize = this->m_InWS2D->blocksize();
  // preprocessed detectors associate each spectra with a detector (position)
  size_t nValidSpectra = m_NSpectra;

  // create local unit conversion class
  UnitsConversionHelper localUnitConv(m_UnitConversion);
  // local coordinatres initiated by the global coordinates which do not depend
  // on detector
  std::vector<coord_t> locCoord(m_Coord);

  // allocate temporary buffer for MD Events data
  std::vector<float> sig_err(2 * m_bufferSize); // array for signal and error.
  std::vector<uint16_t> run_index(
      m_bufferSize); // Buffer run index for each event
  std::vector<uint32_t> det_ids(
      m_bufferSize); // Buffer of det Id-s for each event

  std::vector<coord_t> allCoord(m_NDims *
                                m_bufferSize); // MD events coordinates buffer
  size_t n_coordinates = 0;

  size_t nSpectraToProcess = startSpectra + m_spectraChunk;
  if (nSpectraToProcess > nValidSpectra)
    nSpectraToProcess = nValidSpectra;

  // External loop over the spectra:
  for (size_t i = startSpectra; i < nSpectraToProcess; ++i) {
    size_t iSpctr = m_detIDMap[i];
    int32_t det_id = m_detID[i];

    const MantidVec &X = m_InWS2D->readX(iSpctr);
    const MantidVec &Signal = m_InWS2D->readY(iSpctr);
    const MantidVec &Error = m_InWS2D->readE(iSpctr);

    // calculate the coordinates which depend on detector posision
    if (!m_QConverter->calcYDepCoordinates(locCoord, i))
      continue; // skip y outside of the range;

    bool histogram(true);
    if (X.size() == Signal.size())
      histogram = false;

    // convert units
    localUnitConv.updateConversion(i);
    std::vector<double> XtargetUnits;
    XtargetUnits.resize(X.size());

    if (histogram) {
      double xm1 = localUnitConv.convertUnits(X[0]);
      for (size_t j = 1; j < XtargetUnits.size(); j++) {
        double xm = localUnitConv.convertUnits(X[j]);
        XtargetUnits[j - 1] = 0.5 * (xm + xm1);
        xm1 = xm;
      }
      XtargetUnits[XtargetUnits.size() - 1] =
          xm1; // just in case, should not be used
    } else
      for (size_t j = 0; j < XtargetUnits.size(); j++)
        XtargetUnits[j] = localUnitConv.convertUnits(X[j]);

    //=> START INTERNAL LOOP OVER THE "TIME"
    for (size_t j = 0; j < specSize; ++j) {
      double signal = Signal[j];
      // drop NaN events
      if (isNaN(signal))
        continue;
      // drop 0 -value signals if necessary.
      if (ignoreZeros && (signal == 0.))
        continue;
      double errorSq = Error[j] * Error[j];

      if (!m_QConverter->calcMatrixCoord(XtargetUnits[j], locCoord, signal,
                                         errorSq))
        continue; // skip ND outside the range
      //  ADD RESULTING EVENTS TO THE BUFFER
      // coppy all data into data buffer for future transformation into events;
      sig_err[2 * nBufEvents + 0] = float(signal);
      sig_err[2 * nBufEvents + 1] = float(errorSq);
      run_index[nBufEvents] = m_RunIndex;
      det_ids[nBufEvents] = det_id;

      for (size_t ii = 0; ii < m_NDims; ii++)
        allCoord[n_coordinates++] = locCoord[ii];

      // calculate number of events
      nBufEvents++;
      if (nBufEvents >= m_bufferSize) {
        m_OutWSWrapper->addMDData(sig_err, run_index, det_ids, allCoord,
                                  nBufEvents);
        nAddedEvents += nBufEvents;
        // reset buffer counts
        n_coordinates = 0;
        nBufEvents = 0;
      }
    } // end spectra loop
  }   // end detectors loop;

  if (nBufEvents > 0) {
    m_OutWSWrapper->addMDData(sig_err, run_index, det_ids, allCoord,
                              nBufEvents);
    nAddedEvents += nBufEvents;
    nBufEvents = 0;
  }

  return nAddedEvents;
}

/** run conversion as multithread job*/
void ConvToMDHistoWS::runConversion(API::Progress *pProgress) {
  // counder for the number of events
  size_t nAddedEvents(0);
  //
  Mantid::API::BoxController_sptr bc =
      m_OutWSWrapper->pWorkspace()->getBoxController();
  size_t lastNumBoxes = bc->getTotalNumMDBoxes();
  size_t nEventsInWS = m_OutWSWrapper->pWorkspace()->getNPoints();
  //

  const size_t specSize = m_InWS2D->blocksize();
  // preprocessed detectors associate each spectra with a detector (position)
  size_t nValidSpectra = m_NSpectra;

  // if any property dimension is outside of the data range requested, the job
  // is done;
  if (!m_QConverter->calcGenericVariables(m_Coord, m_NDims))
    return;

  //--->>> Thread control stuff
  Kernel::ThreadSchedulerFIFO *ts(NULL);
  int nThreads(m_NumThreads);
  if (nThreads < 0)
    nThreads = 0; // negative m_NumThreads correspond to all cores used, 0 no
                  // threads and positive number -- nThreads requested;
  bool runMultithreaded = false;
  if (m_NumThreads != 0) {
    runMultithreaded = true;
    // Create the thread pool that will run all of these.  It will be deleted by
    // the threadpool
    ts = new Kernel::ThreadSchedulerFIFO();
    // it will initiate thread pool with number threads or machine's cores (0 in
    // tp constructor)
    pProgress->resetNumSteps(nValidSpectra, 0, 1);
  }
  Kernel::ThreadPool tp(ts, nThreads, new API::Progress(*pProgress));
  //<<<--  Thread control stuff

  if (runMultithreaded)
    nThreads = static_cast<int>(tp.getNumPhysicalCores());
  else
    nThreads = 1;

  // estimate the size of data conversion a single thread should perform
  // TO DO: this piece of code should be carefully rethinked
  size_t eventsChunkNum = bc->getSignificantEventsNumber();
  this->estimateThreadWork(nThreads, specSize, eventsChunkNum);

  // External loop over the spectra:
  for (size_t i = 0; i < nValidSpectra; i += m_spectraChunk) {
    size_t nThreadEv = this->conversionChunk(i);
    nAddedEvents += nThreadEv;
    nEventsInWS += nThreadEv;

    if (bc->shouldSplitBoxes(nEventsInWS, nAddedEvents, lastNumBoxes)) {
      if (runMultithreaded) {
        // Do all the adding tasks
        tp.joinAll();
        // Now do all the splitting tasks
        m_OutWSWrapper->pWorkspace()->splitAllIfNeeded(ts);
        if (ts->size() > 0)
          tp.joinAll();
      } else {
        m_OutWSWrapper->pWorkspace()->splitAllIfNeeded(
            NULL); // it is done this way as it is possible trying to do single
                   // threaded split more efficiently
      }
      // Count the new # of boxes.
      lastNumBoxes = bc->getTotalNumMDBoxes();
      nAddedEvents = 0;
      pProgress->report(i, "Adding Events");
    }
    // TODO::
    // if (m_OutWSWrapper->ifNeedsSplitting())
    //{
    //  // Do all the adding tasks
    //  //tp.joinAll();
    //  // Now do all the splitting tasks
    //  //m_OutWSWrapper->pWorkspace()->splitAllIfNeeded(ts);
    //  m_OutWSWrapper->splitList(ts);
    //  //if (ts->size() > 0)       tp.joinAll();
    //  // Count the new # of boxes.
    //  lastNumBoxes =
    //  m_OutWSWrapper->pWorkspace()->getBoxController()->getTotalNumMDBoxes();
    //}
    // pProgress->report(i);
  } // end detectors loop;
  // Do a final splitting of everything
  if (runMultithreaded) {
    tp.joinAll();
    m_OutWSWrapper->pWorkspace()->splitAllIfNeeded(ts);
    tp.joinAll();
  } else {
    m_OutWSWrapper->pWorkspace()->splitAllIfNeeded(NULL);
  }
  m_OutWSWrapper->pWorkspace()->refreshCache();
  // m_OutWSWrapper->refreshCentroid();
  pProgress->report();

  /// Set the special coordinate system flag on the output workspace.
  m_OutWSWrapper->pWorkspace()->setCoordinateSystem(m_coordinateSystem);
}
/**function calculates the size of temporary memory used to keep convertTo MD
* data before these data should be added to DataObjects
* @param nThreads        -- number of threads used to process data
* @param specSize        -- the size of single spectra in matrix workspace;
* @param nPointsToProcess -- total number of data points in the workspace
*/
void ConvToMDHistoWS::estimateThreadWork(size_t nThreads, size_t specSize,
                                         size_t nPointsToProcess) {
  if (nThreads == 0)
    nThreads = 1;

  // buffer size is at least a spectra size or more
  m_bufferSize = ((specSize > DATA_BUFFER_SIZE) ? specSize : DATA_BUFFER_SIZE);
  if (m_bufferSize % specSize != 0) {
    m_bufferSize = ((m_bufferSize / specSize) + 1) * specSize;
  }

  size_t nSpectras = nPointsToProcess / specSize + 1;
  m_spectraChunk = std::max(nSpectras / nThreads, static_cast<size_t>(1));
}

} // endNamespace DataObjects
} // endNamespace Mantid
