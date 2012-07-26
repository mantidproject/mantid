#include "MantidMDEvents/ConvToMDHistoWS.h"


namespace Mantid
{
namespace MDEvents
{
    // service variable used for efficient filling of the MD event WS  -> should be moved to configuration?
#define DATA_BUFFER_SIZE  8192
    /// Template to check if a variable equal to NaN
template <class T>
inline bool isNaN(T val){
  volatile T buf=val;
  return (val!=buf);
}

/** method sets up all internal variables necessary to convert from Matrix2D workspace to MDEvent workspace 
@parameter WSD         -- the class describing the target MD workspace, sorurce matrtix workspace and the transformations, necessary to perform on these workspaces
@parameter inWSWrapper -- the class wrapping the target MD workspace
*/
size_t  ConvToMDHistoWS::initialize(const MDEvents::MDWSDescription &WSD, boost::shared_ptr<MDEvents::MDEventWSWrapper> inWSWrapper)
{

  size_t numSpec=ConvToMDBase::initialize(WSD,inWSWrapper);

  // check if we indeed have matrix workspace as input.
  DataObjects::Workspace2D_const_sptr pWS2D  = boost::dynamic_pointer_cast<const DataObjects::Workspace2D>(m_InWS2D);
  if(!pWS2D.get())throw(std::logic_error("ConvToMDEventsHisto should work with defined histrohram workspace"));

  return numSpec;
}

/** convert range of spectra starting from initial spectra  startSpectra into MD events
 *@param startSpectra -- initial spectra number to begin conversion from 
 *
 * @returns -- number of events added to the workspace. 
*/
size_t ConvToMDHistoWS::conversionChunk(size_t startSpectra)
{
  size_t n_added_events(0),n_buf_events(0);

  const size_t specSize = this->m_InWS2D->blocksize();    
  // preprocessed detectors associate each spectra with a detector (position)
  size_t nValidSpectra  = m_DetLoc->nDetectors();

  // create local unit conversion class
  UnitsConversionHelper localUnitConv(m_UnitConversion);
  // local coordinatres initiated by the global coordinates which do not depend on detector
  std::vector<coord_t>locCoord(m_Coord);

  // allocate temporary buffer for MD Events data 
  std::vector<float>    sig_err(2*m_bufferSize);       // array for signal and error. 
  std::vector<uint16_t> run_index(m_bufferSize);       // Buffer run index for each event 
  std::vector<uint32_t> det_ids(m_bufferSize);         // Buffer of det Id-s for each event

  std::vector<coord_t>  allCoord(m_NDims*m_bufferSize); // MD events coordinates buffer
  size_t n_coordinates = 0;

  size_t nSpectraToProcess = startSpectra+m_spectraChunk;
  if(nSpectraToProcess >nValidSpectra)nSpectraToProcess=nValidSpectra;

  //External loop over the spectra:
  for (size_t i = startSpectra; i < nSpectraToProcess; ++i)
  {
    size_t iSpctr             = m_DetLoc->getDetSpectra(i);
    int32_t det_id            = m_DetLoc->getDetID(i);

    const MantidVec& X        = m_InWS2D->readX(iSpctr);
    const MantidVec& Signal   = m_InWS2D->readY(iSpctr);
    const MantidVec& Error    = m_InWS2D->readE(iSpctr);

    // calculate the coordinates which depend on detector posision 
    if(!m_QConverter->calcYDepCoordinates(locCoord,i))continue;   // skip y outside of the range;

    // convert units 
    localUnitConv.updateConversion(i);
    std::vector<double> XtargetUnits;

    XtargetUnits.resize(X.size());
    for(size_t j=0;j<XtargetUnits.size();j++) XtargetUnits[j]=localUnitConv.convertUnits(X[j]);


    //=> START INTERNAL LOOP OVER THE "TIME"
    for (size_t j = 0; j < specSize; ++j)
    {
      // drop NaN events
      if(isNaN(Signal[j]))continue;

      if(!m_QConverter->calcMatrixCoordinates(XtargetUnits,i,j,locCoord))continue; // skip ND outside the range
      //  ADD RESULTING EVENTS TO THE BUFFER
      float ErrSq = float(Error[j]*Error[j]);

      // coppy all data into data buffer for future transformation into events;
      sig_err[2*n_buf_events+0]= float(Signal[j]);
      sig_err[2*n_buf_events+1]= ErrSq;
      run_index[n_buf_events]  = m_RunIndex;
      det_ids[n_buf_events]    = det_id;

      for(size_t ii=0;ii<m_NDims;ii++) allCoord[n_coordinates++]=locCoord[ii];
     

      // calculate number of events
      n_buf_events++;
      if(n_buf_events>=m_bufferSize)
      {
        m_OutWSWrapper->addMDData(sig_err,run_index,det_ids,allCoord,n_buf_events);
        n_added_events+=n_buf_events;
        // reset buffer counts
        n_coordinates= 0;
        n_buf_events = 0;
      }
    } // end spectra loop     
  } // end detectors loop;

  if(n_buf_events>0)
  {
    m_OutWSWrapper->addMDData(sig_err,run_index,det_ids,allCoord,n_buf_events);
    n_added_events+=n_buf_events;
    n_buf_events=0;
  }

  return n_added_events;
}

/** run conversion as multithread job*/
void ConvToMDHistoWS::runConversion(API::Progress *pProgress)
{
  // counder for the number of events
  size_t n_added_events(0);
  //
  Mantid::API::BoxController_sptr bc = m_OutWSWrapper->pWorkspace()->getBoxController();
  size_t lastNumBoxes                = bc->getTotalNumMDBoxes();
  size_t nEventsInWS                 = m_OutWSWrapper->pWorkspace()->getNPoints();
  //

  const size_t specSize = m_InWS2D->blocksize();    
  // preprocessed detectors associate each spectra with a detector (position)
  size_t nValidSpectra  = m_DetLoc->nDetectors();

  // if any property dimension is outside of the data range requested, the job is done;
  if(!m_QConverter->calcGenericVariables(m_Coord,m_NDims))return; 


  // Create the thread pool that will run all of these.
  //Kernel::ThreadScheduler * ts = new Kernel::ThreadSchedulerFIFO();
  // initiate thread pool with number of machine's cores (0 in tp constructor)
  Kernel::ThreadScheduler * ts = NULL;       
  pProgress->resetNumSteps(nValidSpectra,0,1);
  //Kernel::ThreadPool tp(ts, 0, new API::Progress(*pProgress));
  // estimate the size of data conversion a single thread should perform

  //size_t nThreads = tp.getNumPhysicalCores();
  size_t nThreads = 1;
  this->estimateThreadWork(nThreads,specSize);

  //External loop over the spectra:
  for (size_t i = 0; i < nValidSpectra; i+=m_spectraChunk)
  {
    size_t n_thread_ev = this->conversionChunk(i);
    n_added_events+=n_thread_ev;
    nEventsInWS   +=n_thread_ev;

    if (m_OutWSWrapper->ifNeedsSplitting())
    {
      // Do all the adding tasks
      //tp.joinAll();    
      // Now do all the splitting tasks
      //m_OutWSWrapper->pWorkspace()->splitAllIfNeeded(ts);
      m_OutWSWrapper->splitList(ts);
      //if (ts->size() > 0)       tp.joinAll();
      // Count the new # of boxes.
      lastNumBoxes = m_OutWSWrapper->pWorkspace()->getBoxController()->getTotalNumMDBoxes();
    }
    pProgress->report(i);
  } // end detectors loop;

   m_OutWSWrapper->splitList(ts);
  // m_OutWSWrapper->pWorkspace()->splitAllIfNeeded(ts); 
  m_OutWSWrapper->pWorkspace()->refreshCache();
  m_OutWSWrapper->refreshCentroid();
  pProgress->report();          
}
/**function calculates the size of temporary memory used to keep convertTo MD data before these data should be added to MDEvents
* @param nThreads  -- number of threads used to process data
* @param specSize  -- the size of single spectra in matrix workspace;
*/
void ConvToMDHistoWS::estimateThreadWork(size_t nThreads,size_t specSize)
{
  m_bufferSize     = ((specSize>DATA_BUFFER_SIZE)?specSize:DATA_BUFFER_SIZE);
  if(m_bufferSize%specSize!=0)
  {
    m_bufferSize = ((m_bufferSize/specSize)+1)*specSize;
  }
  size_t nSpectras = this->m_InWS2D->getNPoints()/(specSize);
  m_spectraChunk =  nSpectras/nThreads;
  // estimate number of points, produced by single thread;
  size_t nPoints = m_spectraChunk*nThreads;
  // experimental parameter, which defines the number of points, which can be added to ws efficiently;
  if(nPoints > 10000000)
  {
    nPoints = 10000000;
    m_spectraChunk = nPoints/nThreads+1;
  }
  // the usfullness of this criteria is questionable;
  //if(m_spectraChunk*specSize>10*m_bufferSize)m_spectraChunk = 10*m_bufferSize;
  //if(nSpectras/m_spectraChunk<nThreads)m_spectraChunk=nSpectras/nThreads;

  if(m_spectraChunk<1)m_spectraChunk=1;


}

} // endNamespace MDEvents
} // endNamespace Mantid


