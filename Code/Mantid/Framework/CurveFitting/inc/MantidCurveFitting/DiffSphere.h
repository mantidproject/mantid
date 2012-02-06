#ifndef MANTID_DIFFSPHERE_H_
#define MANTID_DIFFSPHERE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/IFunctionMW.h"

namespace Mantid
{
  namespace CurveFitting
  {
    /**
    @author Jose Borreguero, NScD
    @date 11/14/2011

    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    //structure to hold info on Volino's coefficients
    struct xnlc{
      double x;
      unsigned int l;
      unsigned int n;
    };
    // simple structure to hold a linear interpolation of factor J around its numerical divergence point
    struct linearJ{
      double slope;
      double intercept;
    };

    class DLLExport DiffSphere : public API::ParamFunction, public API::IFunctionMW
    {
    public:
      /// Constructor
      DiffSphere();
      /// Destructor
      virtual ~DiffSphere() {}

      /// overwrite IFunction base class methods
      std::string name()const{return "DiffSphere";}
      std::vector<double> LorentzianCoefficients(double a) const;

    protected:
      virtual void functionMW(double* out, const double* xValues, const size_t nData)const;
      virtual void functionDerivMW(API::Jacobian* out, const double* xValues, const size_t nData);
      //std::vector<double> LorentzianCoefficients(double a);

    private:
      std::vector<xnlc> xnl;     //xnl coefficients
      std::vector<double> alpha; //certain coefficients invariant during fitting
      unsigned int lmax;         //maximum value of l in xnlist
      unsigned int ncoeff;      //number of coefficients
      double divZone;   //linear interpolation zone around the numerical divergence of factor J
      std::vector<linearJ> linearJlist;
      void initXnlCoeff();
      void initAlphaCoeff();
      void initLinJlist();
    };

  } // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_DIFFSPHERE_LOGNORMAL_H_*/
