#ifndef MANTID_DIFFSPHERE_H_
#define MANTID_DIFFSPHERE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/IFunctionMW.h"
#include "MantidAPI/CompositeFunctionMW.h"
#include "DeltaFunction.h"

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

    class DLLExport ElasticDiffSphere : public DeltaFunction
    {
    public:
      /// Constructor
      ElasticDiffSphere();
      /// Destructor
      virtual ~ElasticDiffSphere() {};
      double HeightPrefactor() const;

    };

    /* Class representing the inelastic portion of the DiffSphere algorithm.
     * Contains the 98 Lorentzians.
     */
    class DLLExport InelasticDiffSphere : public API::ParamFunction, public API::IFunctionMW
    {
    public:
      /// Constructor
      InelasticDiffSphere();
      /// Destructor
      virtual ~InelasticDiffSphere() {}
      /// overwrite IFunction base class methods
      std::string name()const{return "InelasticDiffSphere";}
      virtual const std::string category() const { return "Optimization\\FitModels";}

    protected:
      void functionMW(double* out, const double* xValues, const size_t nData)const;
      void functionDerivMW(API::Jacobian* out, const double* xValues, const size_t nData);
      std::vector<double> LorentzianCoefficients(double a) const;

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
    }; // end of class InelasticDiffSphere

    class DLLExport DiffSphere : public API::CompositeFunctionMW
    {
    public:
      /// Constructor
      DiffSphere();
      /// Destructor
      ~DiffSphere() {};

      /// overwrite IFunction base class methods
      std::string name()const{return "DiffSphere";}
      virtual const std::string category() const { return "Optimization\\FitModels";}
      virtual int version() const { return 1;}

    private:
      API::IFunctionMW* m_elastic;    //elastic intensity of the DiffSphere structure factor
      API::IFunctionMW* m_inelastic;  //inelastic intensity of the DiffSphere structure factor
    };

  } // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_DIFFSPHERE_H_*/
