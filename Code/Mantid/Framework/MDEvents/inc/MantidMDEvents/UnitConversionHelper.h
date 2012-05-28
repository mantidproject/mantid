#ifndef H_MD_UNIT_CONVERSION_HELPER
#define H_MD_UNIT_CONVERSION_HELPER

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/Unit.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidMDEvents/ConvToMDPreprocDet.h"

namespace Mantid
{
namespace MDEvents
{
/**  The class helps to organize unit conversion when running transformation from a matrix(event) workspace into 
     MD event workspace

 
    @date 23/05/2012

    Copyright &copy; 2008-2012 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
namespace ConvertToMD
{
    // possible situations with unit conversion
    enum ConvertUnits
    {
          ConvertNo,   //< no, input workspace has the same units as output workspace or in units used by Q-dE algorithms naturally
          ConvertFast,    //< the input workspace has different units from the requested and fast conversion is possible
          ConvertByTOF,   //< conversion possible via TOF
          ConvertFromTOF  //< Input workspace units are the TOF 
    };
}
class DLLExport UnitsConversionHelper
{
    // variables for units conversion:
    // pointer to input workpsace units 
      Kernel::Unit_sptr pSourceWSUnit;
      // pointer to target workspace units
      Kernel::Unit_sptr pTargetUnit;

      // the ID, which specifies what kind of unit conversion should be used. 
      ConvertToMD::ConvertUnits UnitCnvrsn;

      //  these variables needed in the case of fast units conversion
      double factor, power;
      //
      int emode;
      double L1,efix,twoTheta,L2;
      std::vector<double>const *pTwoTheta;
      std::vector<double>const *pL2;

public:
    UnitsConversionHelper():pTwoTheta(NULL),pL2(NULL){};
    void initialize(const ConvToMDPreprocDet &det, API::MatrixWorkspace_const_sptr inWS2D,const std::string &units_to);
    void updateConversion(size_t i);
    void convertUnits(const std::vector<double> &data_toConvert, std::vector<double> &dataConvertTo);

protected: // for testing
    /// establish and initialize proper units conversion from input to output units;
    ConvertToMD::ConvertUnits analyzeUnitsConversion(const std::string &UnitsFrom,const std::string &UnitsTo);

};

} // endNamespace MDEvents
} // endNamespace Mantid

#endif