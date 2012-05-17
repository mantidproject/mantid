#ifndef H_ICONVERT_TO_MDEVENTS_METHODS
#define H_ICONVERT_TO_MDEVENTS_METHODS

#include "MantidKernel/Logger.h"
#include "MantidKernel/TimeSeriesProperty.h"


#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/Progress.h"

#include "MantidAPI/MatrixWorkspace.h"

#include "MantidMDEvents/MDWSDescription.h"
#include "MantidMDEvents/MDEventWSWrapper.h"

#include "MantidMDEvents/ConvToMDPreprocDet.h"

namespace Mantid
{
namespace MDEvents
{
/** class describes the inteface to the methods, which perform conversion from usual workspaces to MDEventWorkspace 
   *
   * @date 07-01-2012

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



 class DLLExport ConvToMDEventsBase
 {
 public:
     // constructor;
     ConvToMDEventsBase();
 
    ///method which initates all main class variables 
    virtual size_t initialize(Mantid::API::MatrixWorkspace_sptr pWS2D, ConvToMDPreprocDet &detLoc,const MDEvents::MDWSDescription &WSD, boost::shared_ptr<MDEvents::MDEventWSWrapper> inWSWrapper);
    /// method which starts the conversion procedure
    virtual void runConversion(API::Progress *)=0;
    /// virtual destructor
    virtual ~ConvToMDEventsBase(){};
/**> helper functions: To assist with units conversion done by separate class and get access to some important internal states of the subalgorithm */
    Kernel::Unit_sptr    getAxisUnits()const;
    double               getEi()const{return TWS.Ei;}
    int                  getEMode()const{return TWS.emode;}
    ConvToMDPreprocDet   const * pPrepDetectors()const{return pDetLoc;}

    API::NumericAxis *getPAxis(int nAaxis)const{return dynamic_cast<API::NumericAxis *>(this->inWS2D->getAxis(nAaxis));}
    std::vector<double> getTransfMatrix()const{return TWS.rotMatrix;}
//<------------------

   /** function extracts the coordinates from additional workspace porperties and places them to proper position within the vector of MD coodinates */
    bool fillAddProperties(std::vector<coord_t> &Coord,size_t nd,size_t n_ws_properties)const;
    //
    void getMinMax(std::vector<double> &min,std::vector<double> &max)const
    {
        min.assign(dim_min.begin(),dim_min.end());
        max.assign(dim_max.begin(),dim_max.end());
    }
    ConvToMDPreprocDet const* getDetectors(){return pDetLoc;}
  protected:
   // common variables used by all workspace=related methods are deployed here
   /// pointer to the input workspace;
    Mantid::API::MatrixWorkspace_sptr inWS2D;
    /// the properties of the requested target MD workpsace:
    MDEvents::MDWSDescription TWS;
    //
   boost::shared_ptr<MDEvents::MDEventWSWrapper> pWSWrapper ;
   // pointer to the array of detector's directions in reciprocal space
    ConvToMDPreprocDet const * pDetLoc;
   /// number of target ws dimesnions
    size_t n_dims;
    /// array of variables which describe min limits for the target variables;
    std::vector<double> dim_min;
    /// the array of variables which describe max limits for the target variables;
    std::vector<double> dim_max;
    /// index of current run(workspace) for MD WS combining
    uint16_t runIndex;
   /// logger -> to provide logging, for MD dataset file operations
    static Mantid::Kernel::Logger& convert_log;
 private:
    /** internal function which do one peace of work, which should be performed by one thread 
      *
      *@param job_ID -- the identifier which specifies, what part of the work on the workspace this job has to do. 
                        Oftern it is a spectra number
      *
    */
   virtual size_t conversionChunk(size_t job_ID)=0;

};


} // end namespace MDAlgorithms
} // end namespace Mantid
#endif