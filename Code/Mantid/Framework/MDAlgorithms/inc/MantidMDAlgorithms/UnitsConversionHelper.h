#ifndef MANTID_MDALGORITHMS_UNIT_CONVERSION_HELPER_H
#define MANTID_MDALGORITHMS_UNIT_CONVERSION_HELPER_H

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/Unit.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidMDAlgorithms/MDWSDescription.h"

namespace Mantid {
namespace MDAlgorithms {
/**  The class helps to organize unit conversion when running transformation
   from a matrix(event) workspace into
     MD event workspace


    @date 23/05/2012

    Copyright &copy; 2008-2012 ISIS Rutherford Appleton Laboratory, NScD Oak
   Ridge National Laboratory & European Spallation Source

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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
namespace CnvrtToMD {
// possible situations with unit conversion
enum ConvertUnits {
  ConvertNo, //< no, input workspace has the same units as output workspace or
  // in units used by Q-dE algorithms naturally
  ConvertFast, //< the input workspace has different units from the requested
  // and fast conversion is possible
  ConvertByTOF,  //< conversion possible via TOF
  ConvertFromTOF //< Input workspace units are the TOF
};
}
class DLLExport UnitsConversionHelper {
  // variables for units conversion:
  // pointer to input workspace units
  Kernel::Unit_sptr m_SourceWSUnit;
  // pointer to target workspace units
  Kernel::Unit_sptr m_TargetUnit;

  // the ID, which specifies what kind of unit conversion should be used.
  CnvrtToMD::ConvertUnits m_UnitCnvrsn;

  //  these variables needed and used in the case of fast units conversion
  double m_Factor, m_Power;
  //  these variables needed and used for conversion through TOF
  int m_Emode;

  double m_L1, m_Efix, m_TwoTheta, m_L2;
  std::vector<double> const *m_pTwoThetas;
  std::vector<double> const *m_pL2s;
  // pointer to detector specific input energy (eFixed) defined for indirect
  // instruments;
  float *m_pEfixedArray;

public:
  UnitsConversionHelper();
  void initialize(const MDWSDescription &targetWSDescr,
                  const std::string &units_to, bool forceViaTOF = false);
  void initialize(const std::string &unitsFrom, const std::string &unitsTo,
                  const DataObjects::TableWorkspace_const_sptr &DetWS,
                  int Emode, bool forceViaTOF = false);
  void updateConversion(size_t i);
  double convertUnits(double val) const;

  bool isUnitConverted() const;
  std::pair<double, double> getConversionRange(double x1, double x2) const;
  // copy constructor
  UnitsConversionHelper(const UnitsConversionHelper &another);

protected: // for testing
  /// establish and initialize proper units conversion from input to output
  /// units;
  CnvrtToMD::ConvertUnits analyzeUnitsConversion(const std::string &UnitsFrom,
                                                 const std::string &UnitsTo,
                                                 bool forceViaTOF = false);
};

} // endNamespace DataObjects
} // endNamespace Mantid

#endif
