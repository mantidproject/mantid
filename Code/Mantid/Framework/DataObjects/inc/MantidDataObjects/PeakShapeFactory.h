#ifndef MANTID_DATAOBJECTS_PEAKSHAPEFACTORY_H_
#define MANTID_DATAOBJECTS_PEAKSHAPEFACTORY_H_

#include "MantidKernel/System.h"
#include <boost/shared_ptr.hpp>

namespace Mantid
{
namespace DataObjects
{

// Forward declaration
class PeakShape;

  /** PeakShapeFactory : Factory for creating peak shapes

    Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
  class DLLExport PeakShapeFactory 
  {
  public:
    /// Destructor
    virtual ~PeakShapeFactory(){};
    /// Make the product
    virtual PeakShape* create(const std::string& source) const = 0;
    /// Set the successor factory. create will be called on that if this instance is not suitable.
    virtual void setSuccessor(boost::shared_ptr<const PeakShapeFactory> successorFactory) = 0;
  };

  /// Helper typedef
  typedef boost::shared_ptr<PeakShapeFactory> PeakShapeFactory_sptr;
  /// Helper typedef
  typedef boost::shared_ptr<const PeakShapeFactory> PeakShapeFactory_const_sptr;


} // namespace DataObjects
} // namespace Mantid

#endif  /* MANTID_DATAOBJECTS_PEAKSHAPEFACTORY_H_ */
