#ifndef MANTID_DATAOBJECTS_PEAKNOSHAPEFACTORY_H_
#define MANTID_DATAOBJECTS_PEAKNOSHAPEFACTORY_H_

#include "MantidKernel/System.h"
#include "MantidDataObjects/PeakShapeFactory.h"


namespace Mantid
{
namespace Geometry
{
// Forward declaration
class PeakShape;
}
namespace DataObjects
{
  /** PeakNoShapeFactory : Factory method for types of NoShape

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
  class DLLExport PeakNoShapeFactory  : public PeakShapeFactory
  {
  public:
    // Constructor
    PeakNoShapeFactory();
    // Destructor
    virtual ~PeakNoShapeFactory();
    // Factory method
    Mantid::Geometry::PeakShape *create(const std::string &source) const;
    // Set successor. No shape will not delegate.
    void setSuccessor(boost::shared_ptr<const PeakShapeFactory> successorFactory);
  private:

  };


} // namespace DataObjects
} // namespace Mantid

#endif  /* MANTID_DATAOBJECTS_PEAKNOSHAPEFACTORY_H_ */
