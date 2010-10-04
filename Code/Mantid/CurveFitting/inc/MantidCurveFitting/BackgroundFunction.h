#ifndef MANTID_CURVEFITTING_BACKGROUNDFUNCTION_H_
#define MANTID_CURVEFITTING_BACKGROUNDFUNCTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IBackgroundFunction.h"
#include <boost/shared_array.hpp>

namespace mu
{
  class Parser;
}

namespace Mantid
{
  namespace CurveFitting
  {
    /**
    A background function. Functions that are intended to be used as backgrounds
    should inherit from this class to enable certain features. E.g. querying

    @author Roman Tolchenov, Tessella plc
    @date 26/04/2010

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
    class DLLExport BackgroundFunction : public API::IBackgroundFunction
    {
    public:
      /// Returns the centre of the function, which may be something as simple as the centre of
      /// the fitting range in the case of a background function or peak shape function this 
      /// return value reflects the centre of the peak
      double centre() const
      {return 0.;}

      /// Returns the height of the function. For a background function this may return an average
      /// height of the background. For a peak function this return value is the height of the peak
      double height() const
      {return 0.;}

      /// Sets the parameters such that centre == c
      void setCentre(const double c)
      {
        (void) c; //Avoid compiler warning
      }

      /// Sets the parameters such that height == h
      void setHeight(const double h)
      {
        (void) h; //Avoid compiler warning
      }

      void fit(const std::vector<double>& X,const std::vector<double>& Y);
    };

  } // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_BACKGROUNDFUNCTION_H_*/
