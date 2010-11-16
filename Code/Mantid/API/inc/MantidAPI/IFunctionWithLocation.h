#ifndef MANTID_API_IIFUNCTIONWITHLOCATION_H_
#define MANTID_API_IIFUNCTIONWITHLOCATION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include "MantidAPI/Function.h"

namespace Mantid
{
namespace API
{
/** An interface to a function with location, which here means a 
    function for which the user may ask what is its centre and  height. 
    Also allow the user to set these values. Setting the centre and height 
    may e.g. be used to get better starting values for the fitting.

    @author Anders Markvardsen, ISIS, RAL
    @date 2/11/2009

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport IFunctionWithLocation : public Function
{
public:
  /// Returns the centre of the function, which may be something as simple as the centre of
  /// the fitting range in the case of a background function or peak shape function this 
  /// return value reflects the centre of the peak
  virtual double centre()const = 0;

  /// Returns the height of the function. For a background function this may return an average height of the background. For a peak function this return value is the height of the peak
  virtual double height()const = 0;

  /// Sets the parameters such that centre == c
  virtual void setCentre(const double c) = 0;

  /// Sets the parameters such that height == h
  virtual void setHeight(const double h) = 0;
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_IIFUNCTIONWITHLOCATION_H_*/ 
