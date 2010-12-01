#include "MantidAPI/IMDWorkspace.h"

namespace Mantid
{
  namespace API
  {
     boost::shared_ptr<const Mantid::Geometry::IMDDimension> IMDWorkspace::getXDimension() const
      { 
        return boost::shared_ptr<const Mantid::Geometry::IMDDimension>(getXDimensionImp());
      }

      boost::shared_ptr< const Mantid::Geometry::IMDDimension> IMDWorkspace::getYDimension() const
      { 
        return boost::shared_ptr<const Mantid::Geometry::IMDDimension>(getYDimensionImp());
      }

      boost::shared_ptr<const Mantid::Geometry::IMDDimension> IMDWorkspace::getZDimension() const
      { 
        return boost::shared_ptr<const Mantid::Geometry::IMDDimension>(getZDimensionImp());
      }

      boost::shared_ptr<const Mantid::Geometry::IMDDimension> IMDWorkspace::gettDimension() const
      { 
        return boost::shared_ptr<const Mantid::Geometry::IMDDimension>(gettDimensionImp());
      }

      boost::shared_ptr<const Mantid::Geometry::IMDDimension> IMDWorkspace::getDimension(std::string id) const
      { 
        return boost::shared_ptr<const Mantid::Geometry::IMDDimension>(getDimensionImp(id));
      }

      boost::shared_ptr<const Mantid::Geometry::MDPoint> IMDWorkspace::getPoint(int index) const
      { 
        return boost::shared_ptr<const Mantid::Geometry::MDPoint>(getPointImp(index));
      }

      boost::shared_ptr<const Mantid::Geometry::MDCell> IMDWorkspace::getCell(int dim1Increment) const
      { 
        return boost::shared_ptr<const Mantid::Geometry::MDCell>(getCellImp(dim1Increment));
      }

      boost::shared_ptr<const Mantid::Geometry::MDCell> IMDWorkspace::getCell(int dim1Increment, int dim2Increment) const
      { 
        return boost::shared_ptr<const Mantid::Geometry::MDCell>(getCellImp(dim1Increment, dim2Increment));
      }

      boost::shared_ptr<const Mantid::Geometry::MDCell> IMDWorkspace::getCell(int dim1Increment, int dim2Increment, int dim3Increment) const
      { 
        return boost::shared_ptr<const Mantid::Geometry::MDCell>(getCellImp(dim1Increment, dim2Increment, dim3Increment));
      }

      boost::shared_ptr<const Mantid::Geometry::MDCell> IMDWorkspace::getCell(int dim1Increment, int dim2Increment, int dim3Increment, int dim4Increment) const
      { 
        return boost::shared_ptr<const Mantid::Geometry::MDCell>(getCellImp(dim1Increment, dim2Increment, dim3Increment, dim4Increment));
      }

      boost::shared_ptr<const Mantid::Geometry::MDCell> IMDWorkspace::getCell(...) const
      { 
        throw std::runtime_error("Cannot handle dimensions > 4 at present.");
      }

      IMDWorkspace::~IMDWorkspace()
      {
      }
  }
}