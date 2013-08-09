#ifndef MANTID_CURVEFITTING_REFLECTIVITYMULF_H_
#define MANTID_CURVEFITTING_REFLECTIVITYMULF_H_

#include "MantidKernel/System.h"
#include "MantidKernel/Logger.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/IFunction1D.h"
#include <complex>
#include <cmath>


namespace Mantid
{
namespace CurveFitting
{

  /** ReflectivityMulf : Calculate the ReflectivityMulf from a simple layer model.
    
    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  class DLLExport ReflectivityMulf : virtual public API::IFunction1D, public API::ParamFunction
  {
  public:
    ReflectivityMulf();
    virtual ~ReflectivityMulf();

    virtual void init();

    /// Overwrite IFunction base class
    std::string name()const{return "ReflectivityMulf";}

    virtual const std::string category() const { return "General";}

    virtual void function1D(double* out, const double* xValues, const size_t nData)const;

    //virtual void functionDeriv1D(API::Jacobian* out, const double* xValues, const size_t nData);
    //virtual void functionDeriv(const API::FunctionDomain& domain, API::Jacobian& jacobian);

    /// Returns the number of Layers in the model nlayer)
    size_t nAttributes()const{return 1;}

    /// Returns a list of attribute names
    std::vector<std::string> getAttributeNames()const;

    /// Return a value of attribute attName
    Attribute getAttribute(const std::string& attName)const;

    /// Set a value to attribute attName
    void setAttribute(const std::string& attName,const Attribute& );

    /// Check if attribute attName exists
    bool hasAttribute(const std::string& attName)const;

  private:

    /// ReflectivityMulf layers
    int m_nlayer,m_nlayer_old;

    /// Lower x boundary.
    double m_StartX;

    /// Upper x boundary
    double m_EndX;
  };

  typedef boost::shared_ptr<ReflectivityMulf> ReflectivityMulf_sptr;


} // namespace CurveFitting
} // namespace Mantid

#endif  /* MANTID_CURVEFITTING_REFLECTIVITYMULF_H_ */
