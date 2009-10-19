#ifndef MANTID_CURVEFITTING_GAUSSIAN_H_
#define MANTID_CURVEFITTING_GAUSSIAN_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IPeakFunction.h"

namespace Mantid
{
  namespace CurveFitting
  {
    /**
    Takes a histogram in a 2D workspace and fit it to a gaussian on top of a flat background.
    i.e. the function: Height*exp(-0.5*((x-PeakCentre)/Sigma)^2).

    This function actually performs the fitting on 1/Sigma^2 rather than Sigma
    for stability reasons.

    Properties specific to this derived class:
    <UL>
    <LI> Height - height of peak (default 0.0)</LI>
    <LI> PeakCentre - centre of peak (default 0.0)</LI>
    <LI> Sigma - standard deviation (default 1.0)</LI>
    </UL>

    @author Anders Markvardsen, ISIS, RAL
    @date 19/10/2009

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
    class DLLExport Gaussian : public API::IPeakFunction
    {
    public:
      /// Destructor
      virtual ~Gaussian() {};
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "Gaussian";}
      /// Algorithm's version for identification overriding a virtual method
      virtual const int version() const { return (1);}
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "CurveFitting";}


virtual void init();

  /// overwrite base class methods
      virtual double centre()const {return getParameter("PeakCentre");};

  virtual double height()const {return getParameter("Height");};
  /// Returns the peak FWHM
  virtual double width()const {return 2*getParameter("Sigma");};
  /// Sets the parameters such that centre == c
  virtual void setCentre(const double c) {};
  /// Sets the parameters such that height == h
  virtual void setHeight(const double h) {};
  /// Sets the parameters such that FWHM = w
  virtual void setWidth(const double w)const {};


    /// Function you want to fit to.
  virtual void function(double* out, const double* xValues, const int& nData);
  /// Derivatives of function with respect to parameters you are trying to fit
  virtual void functionDeriv(API::Jacobian* out, const double* xValues, const int& nData);

    protected:

      //virtual void modifyStartOfRange(double& startX);
      //virtual void modifyEndOfRange(double& endX);


    };

  } // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_GAUSSIAN_H_*/
