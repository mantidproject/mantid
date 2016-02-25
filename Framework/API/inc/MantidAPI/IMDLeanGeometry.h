#ifndef MANTID_API_IMDLEANGEOMETRY_H_
#define MANTID_API_IMDLEANGEOMETRY_H_

#include "MantidAPI/DllConfig.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidKernel/VMD.h"

namespace Mantid {
namespace API {

/**
General interface for any MD geometry, which should be valid to
describe the geometry of any MD workspace.

Copyright &copy; 2015,2016 ISIS Rutherford Appleton Laboratory, NScD
Oak Ridge National Laboratory & European Spallation Source

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
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_API_DLL IMDLeanGeometry {

public:
  virtual ~IMDLeanGeometry(){};

  virtual void initGeometry(
      const std::vector<Mantid::Geometry::IMDDimension_sptr> &dimensions) = 0;

  //@name Main methods to access dimensions. Can be overriden by e.g.
  // MatrixWorkspace
  //@{
  virtual size_t getNumDims() const = 0;

  virtual boost::shared_ptr<const Mantid::Geometry::IMDDimension>
  getDimension(size_t index) const = 0;

  virtual boost::shared_ptr<const Mantid::Geometry::IMDDimension>
  getDimensionWithId(std::string id) const = 0;

  virtual size_t getDimensionIndexByName(const std::string &name) const = 0;

  virtual size_t getDimensionIndexById(const std::string &id) const = 0;

  virtual Mantid::Geometry::VecIMDDimension_const_sptr
  getNonIntegratedDimensions() const = 0;

  virtual std::vector<coord_t> estimateResolution() const = 0;
  //@}

  //@name Methods to add dimensions.
  //@{
  virtual void
  addDimension(boost::shared_ptr<Mantid::Geometry::IMDDimension> dim) = 0;

  virtual void addDimension(Mantid::Geometry::IMDDimension *dim) = 0;
  //@}

  //@name Methods to manipulate the basis vectors for the dimensions.
  //@{
  virtual Mantid::Kernel::VMD &getBasisVector(size_t index) = 0;

  virtual const Mantid::Kernel::VMD &getBasisVector(size_t index) const = 0;

  virtual void setBasisVector(size_t index, const Mantid::Kernel::VMD &vec) = 0;

  virtual bool allBasisNormalized() const = 0;
  //@}
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_IMDLEANGEOMETRY_H_ */
