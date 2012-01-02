#ifndef H_CONVERT_TO_MDEVENTS_METHODS
#define H_CONVERT_TO_MDEVENTS_METHODS
//
#include "MantidMDAlgorithms/ConvertToMDEvents.h"
#include "MantidMDAlgorithms/ConvertToMDEventsCoordTransf.h"

namespace Mantid
{
namespace MDAlgorithms
{
/** The macrodefinitions for ConvertToMDEvents function, making the conversion into the MD events and dynamic factories, dealing with 
   * creation and managment of new MD Event workspaces as function of number of dimesnions.
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

        File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
        Code Documentation is available at: <http://doxygen.mantidproject.org>
*/




//-----------------------------------------------
template<Q_state Q, AnalMode MODE, CnvrtUnits CONV>
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
  
 
    // initiate the class which is does the conversion of workspace data into MD WS coordinates;
    COORD_TRANSFORMER<Q,MODE,CONV> trn(this); 
    // one of the dimensions has to be X-ws dimension -> need to add check for that;

    // copy experiment info into target workspace
    API::ExperimentInfo_sptr ExperimentInfo(inWS2D->cloneExperimentInfo());
    // run index;
    uint16_t runIndex   = this->pWSWrapper->pWorkspace()->addExperimentInfo(ExperimentInfo);
    // number of dimesnions
    size_t n_dims       = this->pWSWrapper->nDimensions();

    
    const size_t specSize = this->inWS2D->blocksize();    
    size_t nValidSpectra  = det_loc.det_id.size();

    // take at least bufSize for efficiency
    size_t buf_size     = ((specSize>SPLIT_LEVEL)?specSize:SPLIT_LEVEL);
    std::vector<coord_t>  allCoord(n_dims*buf_size);
    std::vector<coord_t>  Coord(n_dims);
    std::vector<float>    sig_err(2*buf_size);
    std::vector<uint16_t> run_index(buf_size);
    std::vector<uint32_t> det_ids(buf_size);


    if(!trn.calcGenericVariables(Coord,n_dims))return; // if any property dimension is outside of the data range requested
    //External loop over the spectra:
    for (int64_t i = 0; i < int64_t(nValidSpectra); ++i)
    {
        size_t ic                 = det_loc.detIDMap[i];
        int32_t det_id            = det_loc.det_id[i];

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
        } // end spectra loop
        if(n_added_events>SPLIT_LEVEL){
              pWSWrapper->addMDData(sig_err,run_index,det_ids,allCoord,n_added_events);
 
              n_added_events=0;
              pProg->report(i);
          }
       } // end detectors loop;

 
        pWSWrapper->refreshCache();
        pProg->report();          

}

    


} // endNamespace MDAlgorithms
} // endNamespace Mantid

#endif