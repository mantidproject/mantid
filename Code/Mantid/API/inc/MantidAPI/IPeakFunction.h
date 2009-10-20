#ifndef MANTID_API_IPEAKFUNCTION_H_
#define MANTID_API_IPEAKFUNCTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFunction.h"

namespace Mantid
{
namespace API
{
/** An interface to a peak function.

    @author Roman Tolchenov, Tessella Support Services plc
    @date 16/10/2009

    Copyright &copy; 2008 STFC Rutherford Appleton Laboratory

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
class DLLExport IPeakFunction : public IFunction
{
public:
  /// Returns the peak centre
  virtual double centre()const = 0;
  /// Returns the peak height
  virtual double height()const = 0;
  /// Returns the peak FWHM
  virtual double width()const = 0;
  /// Sets the parameters such that centre == c
  virtual void setCentre(const double c) = 0;
  /// Sets the parameters such that height == h
  virtual void setHeight(const double h) = 0;
  /// Sets the parameters such that FWHM = w
  virtual void setWidth(const double w) = 0;
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_IPEAKFUNCTION_H_*/
