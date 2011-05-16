#ifndef MANTID_CURVEFITTING_GAUSSIAN1D_H_
#define MANTID_CURVEFITTING_GAUSSIAN1D_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Fit1D.h"

namespace Mantid
{
  namespace CurveFitting
  {
    /**
    Deprecation notice: instead of using this algorithm please use the Fit algorithm 
    where the Function parameter of this algorithm is used 
    to specified the fitting function. 

    Takes a histogram in a 2D workspace and fit it to a gaussian on top of a flat background.
    i.e. the function: BG0+Height*exp(-0.5*((x-PeakCentre)/Sigma)^2).

    This function actually performs the fitting on 1/Sigma^2 rather than Sigma
    for stability reasons.

    Properties specific to this derived class:
    <UL>
    <LI> BG0 - background intercept value (default 0.0)</LI>
    <LI> Height - height of peak (default 0.0)</LI>
    <LI> PeakCentre - centre of peak (default 0.0)</LI>
    <LI> Sigma - standard deviation (default 1.0)</LI>
    </UL>

    @author Anders Markvardsen, ISIS, RAL
    @date 15/5/2009

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
    class DLLExport Gaussian1D : public Fit1D
    {
    public:
      /// Destructor
      virtual ~Gaussian1D() {};
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "Gaussian1D";}
      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return (1);}
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "CurveFitting";}


    protected:
      /// Sets documentation strings for this algorithm
      virtual void initDocs();
      /// overwrite base class methods
      virtual void function(const double* in, double* out, const double* xValues, const size_t nData);
      virtual void declareParameters();
      virtual void functionDeriv(const double* in, Jacobian* out, const double* xValues, const size_t nData);
      virtual void modifyStartOfRange(double& startX);
      virtual void modifyEndOfRange(double& endX);
      virtual void modifyInitialFittedParameters(std::vector<double>& fittedParameter);
      virtual void modifyFinalFittedParameters(std::vector<double>& fittedParameter);

    };

  } // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_GAUSSIAN1D_H_*/
