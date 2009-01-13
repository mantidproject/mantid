#ifndef MANTID_API_IINSTRUMENT_H_
#define MANTID_API_IINSTRUMENT_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/Logger.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Detector.h"
#include <boost/shared_ptr.hpp>
#include <string>
#include <ostream>

namespace Mantid
{
namespace API
{

class Geometry::IObjComponent;

/** IInstrument Class. The abstract instrument class it is the base for 
    Instrument and ParInstrument classes.

    @author Nick Draper, ISIS, RAL
    @date 26/09/2007
    @author Anders Markvardsen, ISIS, RAL
    @date 1/4/2008

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport IInstrument: public virtual Geometry::IComponent
{
public:
  ///String description of the type of component
  virtual std::string type() const { return "IInstrument"; }

  ///Virtual destructor
  virtual ~IInstrument() {}

  virtual Geometry::IObjComponent_sptr getSource() const = 0;
  virtual Geometry::IObjComponent_sptr getSample() const = 0;
  virtual Geometry::IDetector_sptr getDetector(const int &detector_id) const = 0;
  virtual const double detectorTwoTheta(Geometry::IDetector_const_sptr) const = 0;

  virtual std::string getName() const = 0;

  /// return reference to detector cache 
  virtual std::map<int, Geometry::IDetector_sptr> getDetectors() = 0;

  /// Get pointers to plottable components
  virtual std::vector<Geometry::IObjComponent_sptr> getPlottable()const = 0;
};

/// Shared pointer to IInstrument
typedef boost::shared_ptr<IInstrument> IInstrument_sptr;
/// Shared pointer to IInstrument (const version)
typedef const boost::shared_ptr<const IInstrument> IInstrument_const_sptr;

} // namespace API
} // namespace Mantid

#endif /*MANTID_KERNEL_PARINSTRUMENT_H_*/
