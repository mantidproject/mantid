#ifndef MANTID_API_ITRANSFORMSCALE_H_
#define MANTID_API_ITRANSFORMSCALE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#ifndef Q_MOC_RUN
#include <boost/shared_ptr.hpp>
#endif

#include <vector>

#include "MantidAPI/DllConfig.h"

namespace Mantid {
namespace API {
/*Base class  representing a scaling transformation acting on a one-dimensional
  grid domain

  @author Jose Borreguero
  @date Aug/28/2012

  Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

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

  File change history is stored at: <https://github.com/mantidproject/mantid>.
  Code Documentation is available at: <http://doxygen.mantidproject.org>.
*/

class MANTID_API_DLL ITransformScale {
public:
  ITransformScale(){};
  /// Virtual destructor needed for an abstract class
  virtual ~ITransformScale(){};
  virtual const std::string name() const { return "ITransformScale"; }
  /// The scaling transformation. Define in derived classes
  virtual void transform(std::vector<double> &gd) = 0;
}; // class ITransformScale

/// typedef for a shared pointer
typedef boost::shared_ptr<ITransformScale> ITransformScale_sptr;

} // namespace API
} // namespace Mantid

#define DECLARE_TRANSFORMSCALE(classname)                                      \
  namespace {                                                                  \
  Mantid::Kernel::RegistrationHelper register_trs_##classname(                 \
      ((Mantid::API::TransformScaleFactory::Instance().subscribe<classname>(   \
           #classname)),                                                       \
       0));                                                                    \
  }

#endif /*MANTID_API_ITRANSFORMSCALE_H_*/
