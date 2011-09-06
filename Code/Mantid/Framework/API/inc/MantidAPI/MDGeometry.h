#ifndef MANTID_API_MDGEOMETRY_H_
#define MANTID_API_MDGEOMETRY_H_
    
#include "MantidKernel/System.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidKernel/VMD.h"


namespace Mantid
{
namespace API
{

  /** Describes the geometry (i.e. dimensions) of an IMDWorkspace.
    
    @author Janik Zikovsky
    @date 2011-09-06

    Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  class DLLExport MDGeometry 
  {
  public:
    MDGeometry();
    ~MDGeometry();

    // --------------------------------------------------------------------------------------------
    boost::shared_ptr<const Mantid::Geometry::IMDDimension> getXDimension() const;
    boost::shared_ptr<const Mantid::Geometry::IMDDimension> getYDimension() const;
    boost::shared_ptr<const Mantid::Geometry::IMDDimension> getZDimension() const;
    boost::shared_ptr<const Mantid::Geometry::IMDDimension> getTDimension() const;
    boost::shared_ptr<Mantid::Geometry::IMDDimension> getDimensionNum(size_t index) const;
    boost::shared_ptr<const Mantid::Geometry::IMDDimension> getDimension(std::string id) const;

    /// All MD type workspaces have an effective geometry. MD type workspaces must provide this geometry in a serialized format.
    std::string getGeometryXML() const;

    // --------------------------------------------------------------------------------------------
    Mantid::Kernel::VMD & getBasisVector(size_t index);
    const Mantid::Kernel::VMD & getBasisVector(size_t index) const;
    void setBasisVector(size_t index, const Mantid::Kernel::VMD & vec);

    // --------------------------------------------------------------------------------------------
    ///@return the vector of the origin (in the original workspace) that corresponds to 0,0,0... in this workspace
    Mantid::Kernel::VMD & getOrigin()
    { return m_origin; }

    ///@return the vector of the origin (in the original workspace) that corresponds to 0,0,0... in this workspace
    const Mantid::Kernel::VMD & getOrigin() const
    { return m_origin; }

    /// Sets the origin of this geometry.
    ///@param orig :: the vector of the origin (in the original workspace) that corresponds to 0,0,0... in this workspace
    void setOrigin(const Mantid::Kernel::VMD & orig)
    { m_origin = orig; }


  protected:
    /// Vector of the dimensions used, in the order X Y Z t, etc.
    std::vector<Mantid::Geometry::IMDDimension_sptr> m_dimensions;

    /// Pointer to the original workspace, if this workspace is a coordinate transformation from an original workspace.
    IMDWorkspace_sptr m_originalWorkspace;

    /// Vector of the basis vector (in the original workspace) for each dimension of this workspace.
    std::vector<Mantid::Kernel::VMD> m_basisVectors;

    /// Vector of the origin (in the original workspace) that corresponds to 0,0,0... in this workspace
    Mantid::Kernel::VMD m_origin;


  };


} // namespace API
} // namespace Mantid

#endif  /* MANTID_API_MDGEOMETRY_H_ */
