#ifndef MANTID_ALGORITHMS_POLYNOMIALCORRECTION_H_
#define MANTID_ALGORITHMS_POLYNOMIALCORRECTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/UnaryOperation.h"

namespace Mantid
{
  namespace Algorithms
  {
    /** 
    Corrects the data and error values on a workspace by the value of a polynomial function
    which is evaluated at the X value of each data point. The data and error values are multiplied or divided
    by the value of this function.

    Required Properties:
    <UL>
    <LI> InputWorkspace  - The name of the workspace to correct</LI>
    <LI> OutputWorkspace - The name of the corrected workspace (can be the same as the input one)</LI>
    <LI> Coefficients    - The coefficients of the polynomial correction function in ascending powers of X</LI>
    <LI> Operatiopn      - Multiply/Divide The operation with which the correction is applied to the data</LI>
    </UL>

    @author Russell Taylor, Tessella plc
    @date 24/03/2009

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
    class DLLExport PolynomialCorrection : public UnaryOperation
    {
    public:
      /// Default constructor
      PolynomialCorrection() : UnaryOperation() {};
      /// Destructor
      virtual ~PolynomialCorrection() {};
      /// Algorithm's name for identification
      virtual const std::string name() const { return "PolynomialCorrection";}
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Corrects the data in a workspace by the value of a polynomial function which is evaluated at the X value of each data point.";}

      /// Algorithm's version for identification
      virtual int version() const { return 1;}
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const{ return "CorrectionFunctions"; }

    private:
      
      // Overridden UnaryOperation methods
      void defineProperties();
      void retrieveProperties();
      void performUnaryOperation(const double XIn, const double YIn, const double EIn, double& YOut, double& EOut);

      std::vector<double> m_coeffs;              ///< Holds the coefficients for the polynomial correction function
      std::vector<double>::size_type m_polySize; ///< The order of the polynomial
      bool m_isOperationMultiply;                ///< True is the operation is multiply, false  means divide
    };

  } // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_POLYNOMIALCORRECTION_H_*/
