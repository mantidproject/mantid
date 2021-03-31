// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidKernel/Unit.h"
#include "MantidMDAlgorithms/MDWSDescription.h"

namespace Mantid {
namespace MDAlgorithms {
/**  The class helps to organize unit conversion when running transformation
   from a matrix(event) workspace into
     MD event workspace


    @date 23/05/2012
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
} // namespace CnvrtToMD
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
  void initialize(const MDWSDescription &targetWSDescr, const std::string &unitsTo, bool forceViaTOF = false);
  void initialize(const std::string &unitsFrom, const std::string &unitsTo,
                  const DataObjects::TableWorkspace_const_sptr &DetWS, int Emode, bool forceViaTOF = false);
  void updateConversion(size_t i);
  double convertUnits(double val) const;

  bool isUnitConverted() const;
  std::pair<double, double> getConversionRange(double x1, double x2) const;
  // copy constructor
  UnitsConversionHelper(const UnitsConversionHelper &another);

protected: // for testing
  /// establish and initialize proper units conversion from input to output
  /// units;
  CnvrtToMD::ConvertUnits analyzeUnitsConversion(const std::string &UnitsFrom, const std::string &UnitsTo,
                                                 bool forceViaTOF = false);
};

} // namespace MDAlgorithms
} // namespace Mantid
