#ifndef MANTID_API_MDLEANGEOMETRY_H_
#define MANTID_API_MDLEANGEOMETRY_H_

#include "MantidAPI/IMDLeanGeometry.h"
#include "MantidKernel/VMD.h"

namespace Mantid {
namespace API {

/**
MD geometry, which should be valid to describe the basics of the
geometry of any MD workspace but also general enough that it can be
used for MDImage workspaces and the like.

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
class MANTID_API_DLL MDLeanGeometry : public IMDLeanGeometry {
public:
  MDLeanGeometry();

  MDLeanGeometry(const MDLeanGeometry &other);

  virtual ~MDLeanGeometry();

  void initGeometry(
      const std::vector<Mantid::Geometry::IMDDimension_sptr> &dimensions) final;

  virtual size_t getNumDims() const override;

  virtual boost::shared_ptr<const Mantid::Geometry::IMDDimension>
  getDimension(size_t index) const override;

  virtual boost::shared_ptr<const Mantid::Geometry::IMDDimension>
  getDimensionWithId(std::string id) const override;

  size_t getDimensionIndexByName(const std::string &name) const final;

  size_t getDimensionIndexById(const std::string &id) const final;

  Mantid::Geometry::VecIMDDimension_const_sptr
  getNonIntegratedDimensions() const override;

  virtual std::vector<coord_t> estimateResolution() const override;

  void
  addDimension(boost::shared_ptr<Mantid::Geometry::IMDDimension> dim) final;

  void addDimension(Mantid::Geometry::IMDDimension *dim) final;

  // --------------------------------------------------------------------------------------------
  Mantid::Kernel::VMD &getBasisVector(size_t index) final;

  const Mantid::Kernel::VMD &getBasisVector(size_t index) const final;

  void setBasisVector(size_t index, const Mantid::Kernel::VMD &vec) final;

  bool allBasisNormalized() const final;

protected:
  /// Vector of the dimensions used, in the order X Y Z t, etc.
  std::vector<Mantid::Geometry::IMDDimension_sptr> m_dimensions;

  /// Vector of the basis vector (in the original workspace) for each dimension
  /// of this workspace.
  std::vector<Mantid::Kernel::VMD> m_basisVectors;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_MDLEANGEOMETRY_H_ */
