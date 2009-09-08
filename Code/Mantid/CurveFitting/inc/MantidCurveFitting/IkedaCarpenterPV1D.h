#ifndef MANTID_CURVEFITTING_IKEDACARPENTERPV1D_H_
#define MANTID_CURVEFITTING_IKEDACARPENTERPV1D_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Fit1D.h"


namespace Mantid
{
  namespace CurveFitting
  {
    /**
    Takes a histogram in a 2D workspace and fit it to a IkedaCarpenterPV on top of
    a constant background. See wiki page www.mantidproject.org/IkedaCarpenterPV1D
    for documentation for this function.

    @author Anders Markvardsen, ISIS, RAL
    @date 1/9/2009

    Copyright &copy; 2007-8 STFC Rutherford Appleton Laboratory

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
    class DLLExport IkedaCarpenterPV1D : public Fit1D
    {
    public:
      /// Destructor
      virtual ~IkedaCarpenterPV1D() {};
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "IkedaCarpenterPV1D";}
      /// Algorithm's version for identification overriding a virtual method
      virtual const int version() const { return (1);}
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "CurveFitting";}

    private:
      // Overridden Fit1D methods
      void declareParameters();
      void function(const double* in, double* out, const double* xValues, const double* yValues, const double* yErrors, const int& nData);
      //void functionDeriv(const double* in, Jacobian* out, const double* xValues, const double* yValues, const double* yErrors, const int& nData);
      double function(const double* in, const double& xx);

    };


  } // namespace Algorithm
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_IKEDACARPENTERPV1D_H_*/
