#ifndef H_CONVERT_TO_MDEVENTS_HISTO_WS
#define H_CONVERT_TO_MDEVENTS_HISTO_WS
//
#include "MantidKernel/System.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/Algorithm.h" 

#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"

#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/Progress.h"
#include "MantidKernel/PhysicalConstants.h"


#include "MantidMDEvents/MDEventWSWrapper.h"
#include "MantidMDEvents/MDEvent.h"

#include "MantidMDAlgorithms/IConvertToMDEventsMethods.h"
#include "MantidMDAlgorithms/ConvertToMDEventsDetInfo.h"
#include "MantidMDAlgorithms/ConvertToMDEventsCoordTransf.h"

namespace Mantid
{
namespace MDAlgorithms
{
/** The templated class to transform matrix workspace into MDEvent workspace
   *
   * @date 11-10-2011

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

        This file is part of Mantid.

        Mantid is free software; you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation; either version 3 of the License, or
        (at your option) any later version.

        Mantid is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
        GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
        along with this program.  If not, see <http://www.gnu.org/licenses/>.

        File/ change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
        Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

// service variable used for efficient filling of the MD event WS  -> should be moved to configuration;
#define SPLIT_LEVEL  2048

//-----------------------------------------------
// Method to process histohram workspace
template<Q_state Q, AnalMode MODE, CnvrtUnits CONV>
class ConvertToMDEvensHistoWS: public IConvertToMDEventsMethods 
{
    /// shalow class which is invoked from processQND procedure and describes the transformation from workspace coordinates to target coordinates
    /// presumably will be completely inlined
     template<Q, MODE, CONV, Histohram> 
     friend struct COORD_TRANSFORMER;
     // the instanciation of the class which does the transformation itself
     COORD_TRANSFORMER<Q,MODE,CONV,Histohram> trn; 
     // 
    virtual void conversionChunk(){};
public:
    size_t  setUPConversion(Mantid::API::MatrixWorkspace_sptr pWS2D, const PreprocessedDetectors &detLoc,
                          const MDEvents::MDWSDescription &WSD, boost::shared_ptr<MDEvents::MDEventWSWrapper> inWSWrapper)
    {
        size_t numSpec=IConvertToMDEventsMethods::setUPConversion(pWS2D,detLoc,WSD,inWSWrapper);

        // initiate the templated class which does the conversion of workspace data into MD WS coordinates;
        trn.setUP(this); 

        return numSpec;
    }

    void runConversion(API::Progress *pProg)
    {
       // counder for the number of events
        size_t n_added_events(0);
       // amount of work
        const size_t numSpec  = inWS2D->getNumberHistograms();
      

        const size_t specSize = this->inWS2D->blocksize();    
        size_t nValidSpectra  = pDetLoc->det_id.size();
        // copy experiment info into target workspace
        API::ExperimentInfo_sptr ExperimentInfo(inWS2D->cloneExperimentInfo());
        // run index;
        uint16_t runIndex   = this->pWSWrapper->pWorkspace()->addExperimentInfo(ExperimentInfo);
       // number of dimesnions
        std::vector<coord_t>  Coord(n_dims);             // coordinates for single event
     // if any property dimension is outside of the data range requested, the job is done;
        if(!trn.calcGenericVariables(Coord,n_dims))return; 

        // take at least bufSize amout of data in one run for efficiency
        size_t buf_size     = ((specSize>SPLIT_LEVEL)?specSize:SPLIT_LEVEL);
        // allocate temporary buffer for MD Events data
        std::vector<coord_t>  allCoord(0); // MD events coordinates buffer
        allCoord.reserve(n_dims*buf_size);
 
        std::vector<float>    sig_err(2*buf_size);       // array for signal and error. 
        std::vector<uint16_t> run_index(buf_size);       // Buffer run index for each event 
        std::vector<uint32_t> det_ids(buf_size);         // Buffer of det Id-s for each event


  
        //External loop over the spectra:
        for (int64_t i = 0; i < int64_t(nValidSpectra); ++i)
        {
            size_t ic                 = pDetLoc->detIDMap[i];
            int32_t det_id            = pDetLoc->det_id[i];

            const MantidVec& X        = inWS2D->readX(ic);
            const MantidVec& Signal   = inWS2D->readY(ic);
            const MantidVec& Error    = inWS2D->readE(ic);


            if(!trn.calcYDepCoordinates(Coord,ic))continue;   // skip y outsize of the range;

         //=> START INTERNAL LOOP OVER THE "TIME"
            for (size_t j = 0; j < specSize; ++j)
            {
                // drop emtpy events
                if(Signal[j]<FLT_EPSILON)continue;

                if(!trn.calcMatrixCoord(X,i,j,Coord))continue; // skip ND outside the range
                //  ADD RESULTING EVENTS TO THE WORKSPACE
                float ErrSq = float(Error[j]*Error[j]);

                // coppy all data into data buffer for future transformation into events;
                sig_err[2*n_added_events+0]=float(Signal[j]);
                sig_err[2*n_added_events+1]=ErrSq;
                run_index[n_added_events]  = runIndex;
                det_ids[n_added_events]    = det_id;
                allCoord.insert(allCoord.end(),Coord.begin(),Coord.end());

                n_added_events++;
                if(n_added_events>=buf_size){
                pWSWrapper->addMDData(sig_err,run_index,det_ids,allCoord,n_added_events);
 
                 n_added_events=0;
                 pProg->report(i);
                }
       
            } // end spectra loop
      
        } // end detectors loop;

       if(n_added_events>0){
              pWSWrapper->addMDData(sig_err,run_index,det_ids,allCoord,n_added_events);
 
              n_added_events=0;
        }
 
        pWSWrapper->pWorkspace()->refreshCache();
        pProg->report();          
    }
};

 
} // endNamespace MDAlgorithms
} // endNamespace Mantid

#endif
