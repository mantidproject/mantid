#ifndef MANTID_KERNEL_IINSTRUMENT_H_
#define MANTID_KERNEL_IINSTRUMENT_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/Logger.h"
//#include "MantidGeometry/CompAssembly.h"
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

/** @class IInstrument IInstrument.h

 	IInstrument Class. The abstract instrument class it is the base for 
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

  virtual boost::shared_ptr<Geometry::IObjComponent> getSource() const = 0;
  virtual boost::shared_ptr<Geometry::IObjComponent>  getSample() const = 0;
  virtual boost::shared_ptr<Geometry::IDetector> getDetector(const int &detector_id) const = 0;
  virtual const double detectorTwoTheta(const boost::shared_ptr<Geometry::IDetector>) const = 0;

  virtual std::string getName() const = 0;


  /// return reference to detector cache 
  virtual std::map<int,  boost::shared_ptr<Geometry::IDetector> > getDetectors() = 0;

  /// Get pointers to plottable components
  virtual std::vector< boost::shared_ptr<Geometry::IObjComponent> > getPlottable()const = 0;

};

} // namespace API
} //Namespace Mantid
#endif /*MANTID_KERNEL_PARINSTRUMENT_H_*/
