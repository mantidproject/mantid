#include "MantidMDEvents/ConvToMDEventsHisto.h"


namespace Mantid
{
namespace MDEvents
{

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
size_t  ConvToMDEventsHisto::initialize(const MDEvents::MDWSDescription &WSD, boost::shared_ptr<MDEvents::MDEventWSWrapper> inWSWrapper)
{
                        
   size_t numSpec=ConvToMDEventsBase::initialize(WSD,inWSWrapper);
   // check if we indeed have matrix workspace as input.
   DataObjects::Workspace2D_const_sptr pWS2D  = boost::dynamic_pointer_cast<const DataObjects::Workspace2D>(inWS2D);
   if(!pWS2D.get()){
           throw(std::logic_error("ConvToMDEventsHisto should work with defined histrohram workspace"));
   }

   return numSpec;
}

void ConvToMDEventsHisto::runConversion(API::Progress *pProg)
{
       // counder for the number of events
        size_t n_added_events(0),n_buf_events(0);
        //
        Mantid::API::BoxController_sptr bc = pWSWrapper->pWorkspace()->getBoxController();
        size_t lastNumBoxes                = bc->getTotalNumMDBoxes();
        size_t nEventsInWS                 = pWSWrapper->pWorkspace()->getNPoints();
        //

        const size_t specSize = this->inWS2D->blocksize();    
        // preprocessed detectors associate each spectra with a detector (position)
        size_t nValidSpectra  = pDetLoc->nDetectors();

      // if any property dimension is outside of the data range requested, the job is done;
        if(!pQConverter->calcGenericVariables(Coord,n_dims))return; 

        // take at least bufSize amout of data in one run for efficiency
        size_t buf_size     = ((specSize>SPLIT_LEVEL)?specSize:SPLIT_LEVEL);
        // allocate temporary buffer for MD Events data 
        std::vector<float>    sig_err(2*buf_size);       // array for signal and error. 
        std::vector<uint16_t> run_index(buf_size);       // Buffer run index for each event 
        std::vector<uint32_t> det_ids(buf_size);         // Buffer of det Id-s for each event

        std::vector<coord_t>  allCoord(n_dims*buf_size); // MD events coordinates buffer
        size_t n_coordinates = 0;


  
        //External loop over the spectra:
        for (size_t i = 0; i < nValidSpectra; ++i)
        {
            size_t iSpctr             = pDetLoc->getDetSpectra(i);
            int32_t det_id            = pDetLoc->getDetID(i);

            const MantidVec& X        = inWS2D->readX(iSpctr);
            const MantidVec& Signal   = inWS2D->readY(iSpctr);
            const MantidVec& Error    = inWS2D->readE(iSpctr);

            // calculate the coordinates which depend on detector posision 
            if(!pQConverter->calcYDepCoordinates(Coord,i))continue;   // skip y outside of the range;

            // convert units 
            UnitConversion.updateConversion(i);
            std::vector<double> XtargetUnits;
            XtargetUnits.resize(X.size());
            for(size_t j=0;j<XtargetUnits.size();j++)
            {
                XtargetUnits[j]=UnitConversion.convertUnits(X[j]);
            }

          //=> START INTERNAL LOOP OVER THE "TIME"
            for (size_t j = 0; j < specSize; ++j)
            {
                // drop NaN events
                if(isNaN(Signal[j]))continue;

                if(!pQConverter->calcMatrixCoordinates(XtargetUnits,i,j,Coord))continue; // skip ND outside the range
                //  ADD RESULTING EVENTS TO THE BUFFER
                float ErrSq = float(Error[j]*Error[j]);

                // coppy all data into data buffer for future transformation into events;
                sig_err[2*n_buf_events+0]= float(Signal[j]);
                sig_err[2*n_buf_events+1]= ErrSq;
                run_index[n_buf_events]  = runIndex;
                det_ids[n_buf_events]    = det_id;
                for(size_t ii=0;ii<n_dims;ii++){
                    allCoord[n_coordinates++]=Coord[ii];
                }

                // calculate number of events
                n_buf_events++;
                if(n_buf_events>=buf_size){
                   pWSWrapper->addMDData(sig_err,run_index,det_ids,allCoord,n_buf_events);
                   n_added_events+=n_buf_events;
                   nEventsInWS   +=n_buf_events;

                   // reset buffer counts
                   n_coordinates= 0;
                   n_buf_events = 0;
                   if (bc->shouldSplitBoxes(nEventsInWS,n_added_events, lastNumBoxes)){
                        // Do all the adding tasks
                        //   tp.joinAll();    
                        // Now do all the splitting tasks
                        //ws->splitAllIfNeeded(ts);
                        pWSWrapper->pWorkspace()->splitAllIfNeeded(NULL);
                        //if (ts->size() > 0)       tp.joinAll();
                        // Count the new # of boxes.
                        lastNumBoxes = pWSWrapper->pWorkspace()->getBoxController()->getTotalNumMDBoxes();
                    }
                    pProg->report(i);
                }
            } // end spectra loop     
        } // end detectors loop;

       if(n_buf_events>0){
              pWSWrapper->addMDData(sig_err,run_index,det_ids,allCoord,n_buf_events);
              n_buf_events=0;
        }

        pWSWrapper->pWorkspace()->splitAllIfNeeded(NULL); 
        pWSWrapper->pWorkspace()->refreshCache();
        pWSWrapper->refreshCentroid();
        pProg->report();          
}


 
} // endNamespace MDEvents
} // endNamespace Mantid


