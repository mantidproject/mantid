#ifndef MANTID_MDALGORITHMS_PREPROCESS_DETECTORS2MD_H
#define MANTID_MDALGORITHMS_PREPROCESS_DETECTORS2MD_H

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidMDAlgorithms/DllConfig.h"

namespace Mantid
{
namespace MDAlgorithms
{
    /**
    This is helper algorithm used to preprocess detector's positions namely 
    to perform generic part of the transformation from a matrix workspace of a real instrument to 
    physical MD workspace of an experimental results (e.g Q-space). 

    Copyright &copy; 13/09/2012 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    class DLLExport PreprocessDetectorsToMD  :  public API::Algorithm
    {
    public:
      PreprocessDetectorsToMD();

      /// Algorithm's name for identification 
      virtual const std::string name() const { return "PreprocessDetectorsToMD";};
      /// Algorithm's version for identification 
      virtual int version() const { return 1;};
      /// Algorithm's category for identification
      virtual const std::string category() const { return "MDAlgorithms";}  
    private:
      void init();
      void exec();
      /// Sets documentation strings for this algorithm
      virtual void initDocs();  

    protected: // for testing
      void processDetectorsPositions(const API::MatrixWorkspace_sptr inputWS,DataObjects::TableWorkspace_sptr &targWS);
    };

}  // MDEvents
} // Mantid
#endif