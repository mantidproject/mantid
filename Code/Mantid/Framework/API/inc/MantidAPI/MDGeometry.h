#ifndef MANTID_API_MDGEOMETRY_H_
#define MANTID_API_MDGEOMETRY_H_
    
#include "MantidKernel/System.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidKernel/Exception.h"


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
    /// Get the x-dimension mapping.
    boost::shared_ptr<const Mantid::Geometry::IMDDimension> getXDimension() const
    {
      return m_dimensions[0];
    }

    /// Get the y-dimension mapping.
    boost::shared_ptr<const Mantid::Geometry::IMDDimension> getYDimension() const
    {
      if (m_dimensions.size() < 2) throw std::runtime_error("Workspace does not have a Y dimension.");
      return m_dimensions[1];
    }

    /// Get the z-dimension mapping.
    boost::shared_ptr<const Mantid::Geometry::IMDDimension> getZDimension() const
    {
      if (m_dimensions.size() < 3) throw std::runtime_error("Workspace does not have a X dimension.");
      return m_dimensions[2];
    }

    /// Get the t-dimension mapping.
    boost::shared_ptr<const Mantid::Geometry::IMDDimension> getTDimension() const
    {
      if (m_dimensions.size() < 4) throw std::runtime_error("Workspace does not have a T dimension.");
      return m_dimensions[3];
    }

    boost::shared_ptr<Mantid::Geometry::IMDDimension> getDimensionNum(size_t index)
    {
      if (index >= m_dimensions.size()) throw std::runtime_error("Workspace does not have a dimension at that index.");
      return m_dimensions[index];
    }

    /// Get the dimension with the specified id.
    boost::shared_ptr<const Mantid::Geometry::IMDDimension> getDimension(std::string id) const
    {
      for (size_t i=0; i < m_dimensions.size(); ++i)
        if (m_dimensions[i]->getDimensionId() == id)
          return m_dimensions[i];
      throw std::invalid_argument("Dimension tagged " + id + " was not found in the Workspace");
    }

    /// All MD type workspaces have an effective geometry. MD type workspaces must provide this geometry in a serialized format.
    std::string getGeometryXML() const;


  protected:
    /// Vector of the dimensions used, in the order X Y Z t, etc.
    std::vector<Mantid::Geometry::IMDDimension_sptr> m_dimensions;


  };


} // namespace API
} // namespace Mantid

#endif  /* MANTID_API_MDGEOMETRY_H_ */
