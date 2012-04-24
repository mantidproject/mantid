#ifndef H_CONVERT_TO_MDEVENTS_SUBALG_FACTORY
#define H_CONVERT_TO_MDEVENTS_SUBALG_FACTORY


#include "MantidMDAlgorithms/ConvertToMDEventsParams.h"
#include "MantidMDAlgorithms/IConvertToMDEventsMethods.h"
#include "MantidMDAlgorithms/ConvertToMDEventsHistoWS.h"
#include "MantidMDAlgorithms/ConvertToMDEventsEventWS.h"

namespace Mantid
{
namespace MDAlgorithms
{

/** The helper class for ConvertToMDEvents, which instantiates various subalgorithms, availible to convert a workspace into MDEvent workspace
   * and accessed on request:
   * It contains the map alg_id->subAlgorithm, where alg_id is provided by ConvertToMDEventsParams class
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

        File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
        Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport ConvertToMDEventsSubalgFactory
{
  /// map to select an algorithm as function of the key, which describes it
    std::map<std::string, IConvertToMDEventsMethods *> alg_selector;
public:
    /// constructor
    ConvertToMDEventsSubalgFactory();
    ~ConvertToMDEventsSubalgFactory();
    /// access to a subalgorithm 
    IConvertToMDEventsMethods * getAlg(const std::string &AlgName);
    /// initiate the subalgorithms and made them availible for getAlg function
    void init(const  ConvertToMDEventsParams &SubAlgDescriptor);
private:
 //--------------------------------------------------------------------------------------------------   
     /// helper class to orginize metaloop instantiating various subalgorithms 
     template< QMode Q,size_t N_ALGORITHMS >
     friend class LOOP_ALGS;  
};

//} // end namespace ConvertToMD
}
}



#endif