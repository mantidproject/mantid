#ifndef H_CONVERT_TO_MDEVENTS_ChildAlg_FACTORY
#define H_CONVERT_TO_MDEVENTS_ChildAlg_FACTORY
/**TODO: FOR DEPRICATION */ 

#include "MantidMDAlgorithms/ConvertToMDEventsParams.h"
#include "MantidMDAlgorithms/ConvertToMDEventsWSInterface.h"
#include "MantidMDAlgorithms/ConvertToMDEventsHistoWS.h"
#include "MantidMDAlgorithms/ConvertToMDEventsEventWS.h"


namespace Mantid
{
namespace MDAlgorithms
{

/** The helper class for ConvertToMDEvents, which instantiates various ChildAlgorithms, availible to convert a workspace into MDEvent workspace
   * and accessed on request:
   * It contains the map alg_id->ChildAlgorithm, where alg_id is provided by ConvertToMDEventsParams class
   *
   * @date 16-03-2012
   *
   * Intended to cover wide range of cases;    

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

        File change history is stored at: <https://github.com/mantidproject/mantid>
        Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport ConvertToMDEventsChildAlgFactory
{
  /// map to select an algorithm as function of the key, which describes it
    std::map<std::string, ConvertToMDEventsWSBase *> alg_selector;
public:
    /// constructor
    ConvertToMDEventsChildAlgFactory();
    ~ConvertToMDEventsChildAlgFactory();
    /// access to a ChildAlgorithm 
    ConvertToMDEventsWSBase * getAlg(const std::string &AlgName);
    /// initiate the ChildAlgorithms and made them availible for getAlg function
    void init(const ConvertToMD::ConvertToMDEventsParams &ChildAlgDescriptor);
private:
 //--------------------------------------------------------------------------------------------------   
     /// helper class to orginize metaloop instantiating various ChildAlgorithms 
     template<ConvertToMD::QMode Q,size_t N_ALGORITHMS >
     friend class LOOP_ALGS;  
};

//} // end namespace ConvertToMD
}
}



#endif