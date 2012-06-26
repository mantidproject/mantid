#ifndef H_CONVERT_TO_MDEVENTS_INTERFACE
#define H_CONVERT_TO_MDEVENTS_INTERFACE

#include "MantidKernel/Logger.h"

#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/ExperimentInfo.h"

#include "MantidAPI/MatrixWorkspace.h"

#include "MantidMDEvents/MDWSDescription.h"
#include "MantidMDEvents/MDEventWSWrapper.h"


#include "MantidMDEvents/ConvToMDPreprocDet.h"
#include "MantidMDEvents/MDTransfInterface.h"
#include "MantidMDEvents/MDTransfFactory.h"

// units conversion
#include "MantidMDEvents/UnitsConversionHelper.h"


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

 class DLLExport ConvToMDBase
 {
 public:
     // constructor;
     ConvToMDBase();
 
    ///method which initates all main class variables 
    virtual size_t initialize(const MDWSDescription &WSD, boost::shared_ptr<MDEventWSWrapper> inWSWrapper);
    /// method which starts the conversion procedure
    virtual void runConversion(API::Progress *)=0;
    /// virtual destructor
    virtual ~ConvToMDBase(){};

 
  protected:
   // pointer to input matrix workspace;
   API::MatrixWorkspace_const_sptr inWS2D;
   // common variables used by all workspace-related methods are specified here
   // pointer to class, which describes preprocessed detectors location
   ConvToMDPreprocDet const * pDetLoc;
   // pointer to the class, which keeps target workspace and provides functions adding additional MD events to it. 
   boost::shared_ptr<MDEvents::MDEventWSWrapper> pWSWrapper;
    // shared pointer to the converter, which convertd WS coordinates to MD coordinates
   MDTransf_sptr      pQConverter;
   /// number of target ws dimesnions
   size_t n_dims;
   // index of current run(workspace). Used for MD WS combining
   uint16_t runIndex;
   // logger -> to provide logging, for MD dataset file operations
   static Mantid::Kernel::Logger& convert_log;
   // vector to keep MD coordinates of single event 
   std::vector<coord_t> Coord;
   // class responsible for converting units if necessary;
   UnitsConversionHelper UnitConversion;
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