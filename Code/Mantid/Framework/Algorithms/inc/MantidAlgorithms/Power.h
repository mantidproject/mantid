
#ifndef MANTID_ALGORITHM_POWER_H_
#define MANTID_ALGORITHM_POWER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/UnaryOperation.h"

namespace Mantid
{
namespace Algorithms
{

/**
 Provides the ability to raise the values in the workspace to a specified power.

 Required Properties:
 <UL>
 <LI> InputWorkspace  - The name of the workspace to correct</LI>
 <LI> OutputWorkspace - The name of the corrected workspace (can be the same as the input one)</LI>
 <LI> exponent        - The exponent to use in the power calculation</LI>
 </UL>

 @author Owen Arnold, Tessella plc
 @date 12/04/2010

 Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
 */

class DLLExport Power: public UnaryOperation
{
public:
  /// Default constructor
  Power() ;
  /// Destructor
  virtual ~Power() {};
  /// Algorithm's name for identification
  virtual const std::string name() const { return "Power"; }
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "The Power algorithm will raise the base workspace to a particular power. Corresponding error values will be created.";}

  /// Algorithm's version for identification
  virtual int version() const { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "Arithmetic";}

private:
  
  // Overridden UnaryOperation methods
  void defineProperties();
  void retrieveProperties();
  void performUnaryOperation(const double XIn, const double YIn, const double EIn, double& YOut,
      double& EOut);
  ///calculate the power
  inline double calculatePower(const double base, const double exponent);
  ///Exponent to raise the base workspace to
  double m_exponent;
};

}
}

#endif
