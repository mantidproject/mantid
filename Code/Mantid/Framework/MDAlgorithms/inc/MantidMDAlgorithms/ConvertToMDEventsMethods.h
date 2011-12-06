#include "MantidMDAlgorithms/ConvertToMDEvents.h"
#include "MantidMDAlgorithms/ConvertToMDEventsCoordTransf.h"
namespace Mantid
{
namespace MDAlgorithms
{

//-----------------------------------------------
template<size_t nd,Q_state Q, AnalMode MODE, CnvrtUnits CONV>
void 
ConvertToMDEvents::processQND(API::IMDEventWorkspace *const piWS)
{
    // service variable used for efficient filling of the MD event WS  -> should be moved to configuration;
    size_t SPLIT_LEVEL(1024);
    // counder for the number of events
    size_t n_added_events(0);
    // amount of work
    const size_t numSpec  = inWS2D->getNumberHistograms();
    // progress reporter
    pProg = std::auto_ptr<API::Progress>(new API::Progress(this,0.0,1.0,numSpec));


    MDEvents::MDEventWorkspace<MDEvents::MDEvent<nd>,nd> *const pWs = dynamic_cast<MDEvents::MDEventWorkspace<MDEvents::MDEvent<nd>,nd> *>(piWS);
    if(!pWs){
        convert_log.error()<<"ConvertToMDEvents: can not cast input worspace pointer into pointer to proper target workspace\n"; 
        throw(std::bad_cast());
    }
    COORD_TRANSFORMER<Q,MODE,CONV> trn(this); 
    // one of the dimensions has to be X-ws dimension -> need to add check for that;

    // copy experiment info into target workspace
    API::ExperimentInfo_sptr ExperimentInfo(inWS2D->cloneExperimentInfo());
    uint16_t runIndex = pWs->addExperimentInfo(ExperimentInfo);
    
    const size_t specSize = inWS2D->blocksize();    
    std::vector<coord_t> Coord(nd);


    if(!trn.calcGenericVariables(Coord,nd))return; // if any property dimension is outside of the data range requested
    //External loop over the spectra:
    for (int64_t i = 0; i < int64_t(numSpec); ++i)
    {
 
        const MantidVec& X        = inWS2D->readX(i);
        const MantidVec& Signal   = inWS2D->readY(i);
        const MantidVec& Error    = inWS2D->readE(i);
        int32_t det_id            = det_loc.det_id[i];

        if(!trn.calcYDepCoordinates(Coord,i))continue;   // skip y outsize of the range;

        //=> START INTERNAL LOOP OVER THE "TIME"
        for (size_t j = 0; j < specSize; ++j)
        {
            // drop emtpy events
           if(Signal[j]<FLT_EPSILON)continue;

           if(!trn.calculate_ND_coordinates(X,i,j,Coord))continue; // skip ND outside the range
            //  ADD RESULTING EVENTS TO THE WORKSPACE
            float ErrSq = float(Error[j]*Error[j]);
            pWs->addEvent(MDEvents::MDEvent<nd>(float(Signal[j]),ErrSq,runIndex,det_id,&Coord[0]));
            n_added_events++;
        } // end spectra loop

         // This splits up all the boxes according to split thresholds and sizes.
         //Kernel::ThreadScheduler * ts = new ThreadSchedulerFIFO();
         //ThreadPool tp(NULL);
          if(n_added_events>SPLIT_LEVEL){
                pWs->splitAllIfNeeded(NULL);
                n_added_events=0;
                pProg->report(i);
          }
          //tp.joinAll();        
       } // end detectors loop;

       // FINALIZE:
       if(n_added_events>0){
         pWs->splitAllIfNeeded(NULL);
         n_added_events=0;
        }
        pWs->refreshCache();
        pProg->report();          

}

/// helper function to create empty MDEventWorkspace with nd dimensions 
template<size_t nd>
API::IMDEventWorkspace_sptr
ConvertToMDEvents::createEmptyEventWS(size_t split_into,size_t split_threshold,size_t split_maxDepth)
{

       boost::shared_ptr<MDEvents::MDEventWorkspace<MDEvents::MDEvent<nd>,nd> > ws = 
       boost::shared_ptr<MDEvents::MDEventWorkspace<MDEvents::MDEvent<nd>, nd> >(new MDEvents::MDEventWorkspace<MDEvents::MDEvent<nd>, nd>());
    
      // Give all the dimensions
      for (size_t d=0; d<nd; d++)
      {
        Geometry::MDHistoDimension * dim = new Geometry::MDHistoDimension(this->targ_dim_names[d], this->targ_dim_names[d], this->targ_dim_units[d], dim_min[d], dim_min[d], 10);
        ws->addDimension(Geometry::MDHistoDimension_sptr(dim));
      }
      ws->initialize();

      // Build up the box controller
      Mantid::API::BoxController_sptr bc = ws->getBoxController();
      bc->setSplitInto(split_into);
//      bc->setSplitThreshold(1500);
      bc->setSplitThreshold(split_threshold);
      bc->setMaxDepth(split_maxDepth);
      // We always want the box to be split (it will reject bad ones)
      ws->splitBox();
      return ws;
}
     


} // endNamespace MDAlgorithms
} // endNamespace Mantid