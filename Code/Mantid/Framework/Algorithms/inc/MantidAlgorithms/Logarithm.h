#ifndef MANTID_ALGORITHM_LOG_H_
#define MANTID_ALGORITHM_LOG_H_

#include "MantidAlgorithms/UnaryOperation.h"
namespace Mantid{
namespace Algorithms {
/** Takes a workspace as input and calculates the natural logarithm of number of counts for each 1D spectrum.
    The algorithm creates a new workspace containing logarithms of signals (numbers of counts) in

    Required Properties:
    <UL>
    <LI> InputWorkspace - The name of the workspace to take as input </LI>
    <LI> OutputWorkspace - The name of the workspace in which to store the result </LI>
    </UL>

    Optional Properties (assume that you count from zero):
    <UL>
    <LI> Filler  -- A workspace can normally have zeros in it and the logarithm goes to minys infinity for zeros.
                    This field keeps value, that should be placed in the workspace instead of minus infinity </LI>
    <LI> Natural -- Natural or base 10 logarithm. Default is natural </LI>
    </UL>

    @author AB,    ISIS, Rutherford Appleton Laboratory
    @date 12/05/2010
    Copyright &copy; 2007-2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
class DLLExport Logarithm : public UnaryOperation
{
public:
    Logarithm();
    virtual ~Logarithm(void){};
    /// Algorithm's name for identification
    virtual const std::string name() const { return "Logarithm";}
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Logarithm function calculates the logarithm of the data, held in a workspace. A user can choose between natural (default) or base 10 logarithm";}

   /// Algorithm's version for identification
    virtual int version() const { return (1);}
    /// Algorithm's category for identification
    virtual const std::string category() const { return "Arithmetic";}
private:
  
  /// The value to replace ln(0)
  double log_Min;
  /// If the logarithm natural or 10-based
  bool   is_natural;

  /// Declare additional properties for this algorithm
  virtual void defineProperties();
  /// get properties from GUI
  virtual void retrieveProperties();
  /// Actually the function, which is run on values when the operation is performed
  virtual void performUnaryOperation(const double XIn, const double YIn, const double EIn, double& YOut, double& EOut);

};

} // End namespace Algorithms
} // End namespace Mantid
#endif
