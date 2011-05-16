#ifndef MANTID_CURVEFITTING_GAUSSIAN1D2_H_
#define MANTID_CURVEFITTING_GAUSSIAN1D2_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Fit1D.h"

namespace Mantid
{
  namespace CurveFitting
  {
    /**
    Takes a histogram in a 2D workspace and fit it to a Gaussian on top of
    a linear background.
    i.e. a function: Height*exp(-0.5*((x-PeakCentre)/Sigma)^2) + BG0 + BG1*x

    This function actually performs the fitting on 1/Sigma^2 rather than Sigma
    for stability reasons.

    Properties specific to this derived class:
    <UL>
    <LI> BG0 - background intercept value (default 0.0)</LI>
    <LI> BG1 - background slope value (default 0.0)</LI>
    <LI> Height - height of peak (default 0.0)</LI>
    <LI> PeakCentre - centre of peak (default 0.0)</LI>
    <LI> Sigma - standard deviation (default 1.0)</LI>
    </UL>

    @author Anders Markvardsen, ISIS, RAL
    @date 21/5/2009

    Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    class DLLExport GaussianLinearBG1D : public Fit1D
    {
    public:
      /// Destructor
      virtual ~GaussianLinearBG1D() {};
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "Gaussian1D";}
      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return (2);}
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "CurveFitting";}

    private:
      /// Sets documentation strings for this algorithm
      virtual void initDocs();
      // Overridden Fit1D methods
      void declareParameters();
      void function(const double* in, double* out, const double* xValues, const size_t nData);
      void functionDeriv(const double* in, Jacobian* out, const double* xValues, const size_t nData);
      void modifyStartOfRange(double& startX);
      void modifyEndOfRange(double& endX);
      void modifyInitialFittedParameters(std::vector<double>& fittedParameter);
      void modifyFinalFittedParameters(std::vector<double>& fittedParameter);

    };

  } // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_GAUSSIANLINEARBG1Dl1D2_H_*/
