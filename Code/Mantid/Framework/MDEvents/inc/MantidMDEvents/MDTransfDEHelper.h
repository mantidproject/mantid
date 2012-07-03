#ifndef H_MDTRANSF_DE_HELPER
#define H_MDTRANSF_DE_HELPER

#include "MantidKernel/DllConfig.h"
#include <vector>
#include <string>

namespace Mantid
{
namespace MDEvents
{
/** The class is here do define common operations/interfaces involved in dE (energy transfer) analysis 
  * for the MD transformations, which actually do energy transfer analyzis 
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

namespace CnvrtToMD
{
    /* enum describes known eneergy conversion/analysis modes
    *  It is important to assign enums proper numbers, as direct correspondence between enums and their emodes 
    *  used by the external units conversion algorithms within the Mantid, so the agreement should be the stame     
    */
    enum EModes
    {
          Elastic = 0,  //< int emode = 0; Elastic analysis
          Direct  = 1,  //< emode=1; Direct inelastic analysis mode
          Indir   = 2,  //< emode=2; InDirect inelastic analysis mode
          No_DE,         //< couples with NoNonentum analysis, means just copying existing data (may be doing units conversion), 
                       // it is also the counter for the number of availible modes, used to initiate the mode names
          Undef        // non-existing (undefined mode). Used to check if emode was defined
    };
}


class DLLExport MDTransfDEHelper
{
 public:
    // energy conversion modes supported by this class
    std::vector<std::string> getEmodes()const{return EmodesList;}
    /// string presentation of emode
    std::string getEmode(CnvrtToMD::EModes Mode)const;
    /// convert string presentation of emode into nimerical one 
    CnvrtToMD::EModes getEmode(const std::string &Mode)const;
    // constructor
    MDTransfDEHelper();
 private:
    std::vector<std::string> EmodesList;
 };

} // endnamespace MDEvents
} // endnamespace Mantid



#endif