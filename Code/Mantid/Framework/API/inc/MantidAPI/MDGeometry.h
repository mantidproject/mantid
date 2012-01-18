#ifndef MANTID_API_MDGEOMETRY_H_
#define MANTID_API_MDGEOMETRY_H_
    
#include "MantidAPI/CoordTransform.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/System.h"
#include "MantidKernel/VMD.h"
#include "MantidAPI/AnalysisDataService.h"
#include <Poco/NObserver.h>


namespace Mantid
{
namespace API
{

  class IMDWorkspace;

  /** Describes the geometry (i.e. dimensions) of an IMDWorkspace.
   * This defines the dimensions contained in the workspace.
   * On option, it can also relate the coordinates of this workspace
   * to another workspace, e.g. if a workspace is a slice or a view
   * onto an original workspace.
    
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
    MDGeometry(const MDGeometry & other);
    ~MDGeometry();
    void initGeometry(std::vector<Mantid::Geometry::IMDDimension_sptr> & dimensions);

    // --------------------------------------------------------------------------------------------
    // These are the main methods for dimensions, that CAN be overridden (e.g. by MatrixWorkspace)
    virtual size_t getNumDims() const;
    virtual boost::shared_ptr<const Mantid::Geometry::IMDDimension> getDimension(size_t index) const;
    virtual boost::shared_ptr<const Mantid::Geometry::IMDDimension> getDimensionNamed(std::string id) const;
    size_t getDimensionIndexByName(const std::string & name) const;
    Mantid::Geometry::VecIMDDimension_const_sptr getNonIntegratedDimensions() const;

    // --------------------------------------------------------------------------------------------
    boost::shared_ptr<const Mantid::Geometry::IMDDimension> getXDimension() const;
    boost::shared_ptr<const Mantid::Geometry::IMDDimension> getYDimension() const;
    boost::shared_ptr<const Mantid::Geometry::IMDDimension> getZDimension() const;
    boost::shared_ptr<const Mantid::Geometry::IMDDimension> getTDimension() const;

    std::string getGeometryXML() const;

    void addDimension(boost::shared_ptr<Mantid::Geometry::IMDDimension> dim);
    void addDimension(Mantid::Geometry::IMDDimension * dim);

    // --------------------------------------------------------------------------------------------
    Mantid::Kernel::VMD & getBasisVector(size_t index);
    const Mantid::Kernel::VMD & getBasisVector(size_t index) const;
    void setBasisVector(size_t index, const Mantid::Kernel::VMD & vec);

    // --------------------------------------------------------------------------------------------
    bool hasOriginalWorkspace() const;
    boost::shared_ptr<Workspace> getOriginalWorkspace() const;
    void setOriginalWorkspace(boost::shared_ptr<Workspace> ws);
    Mantid::API::CoordTransform * getTransformFromOriginal() const;
    void setTransformFromOriginal(Mantid::API::CoordTransform * transform);
    Mantid::API::CoordTransform * getTransformToOriginal() const;
    void setTransformToOriginal(Mantid::API::CoordTransform * transform);

    void transformDimensions(std::vector<double> & scaling, std::vector<double> & offset);

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

    /// Function called when observer objects recieves a notification
    void deleteNotificationReceived(Mantid::API::WorkspacePreDeleteNotification_ptr notice);

    /// Vector of the dimensions used, in the order X Y Z t, etc.
    std::vector<Mantid::Geometry::IMDDimension_sptr> m_dimensions;

    /// Pointer to the original workspace, if this workspace is a coordinate transformation from an original workspace.
    boost::shared_ptr<Workspace> m_originalWorkspace;

    /// Vector of the basis vector (in the original workspace) for each dimension of this workspace.
    std::vector<Mantid::Kernel::VMD> m_basisVectors;

    /// Vector of the origin (in the original workspace) that corresponds to 0,0,0... in this workspace
    Mantid::Kernel::VMD m_origin;

    /// Coordinate Transformation that goes from the original workspace to this workspace's coordinates.
    Mantid::API::CoordTransform * m_transformFromOriginal;

    /// Coordinate Transformation that goes from this workspace's coordinates to the original workspace coordinates.
    Mantid::API::CoordTransform * m_transformToOriginal;

    /// Poco delete notification observer object
    Poco::NObserver<MDGeometry, Mantid::API::WorkspacePreDeleteNotification> m_delete_observer;

    /// Set to True when the m_delete_observer is observing workspace deletions.
    bool m_observingDelete;


  };


} // namespace API
} // namespace Mantid

#endif  /* MANTID_API_MDGEOMETRY_H_ */
