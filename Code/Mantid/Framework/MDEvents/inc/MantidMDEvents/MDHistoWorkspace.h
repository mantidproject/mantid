#ifndef MANTID_MDEVENTS_MDHISTOWORKSPACE_H_
#define MANTID_MDEVENTS_MDHISTOWORKSPACE_H_

#include "MantidKernel/System.h"
#include "MantidKernel/Exception.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidAPI/IMDWorkspace.h"


namespace Mantid
{
namespace MDEvents
{

  /** MDHistoWorkspace:
  *
  * An implementation of IMDWorkspace that contains a (normally dense) histogram
  * representation in up to 4 dimensions.
  *
  * This will be the result of a slice or rebin of another workspace, e.g. a
  * MDEventWorkspace.
  *
  * This will be used by ParaView e.g. for visualization.
  *
  * @author Janik Zikovsky
  * @date 2011-03-24 11:21:06.280523
  */
  class DLLExport MDHistoWorkspace : public API::IMDWorkspace
  {
  public:
    MDHistoWorkspace();
    ~MDHistoWorkspace();

    /// Get the number of points associated with the workspace; For MD workspace it is number of points contributing into the workspace
    uint64_t getNPoints() const
    {
    }

    /// Get the number of dimensions
    int getNDimensions() const
    {
    }

    /// Get the x-dimension mapping.
    boost::shared_ptr<const Mantid::Geometry::IMDDimension> getXDimension() const
    {
    }

    /// Get the y-dimension mapping.
    boost::shared_ptr<const Mantid::Geometry::IMDDimension> getYDimension() const
    {
    }

    /// Get the z-dimension mapping.
    boost::shared_ptr<const Mantid::Geometry::IMDDimension> getZDimension() const
    {
    }

    /// Get the t-dimension mapping.
    boost::shared_ptr<const Mantid::Geometry::IMDDimension> getTDimension() const
    {
    }

    /// Get the dimension with the specified id.
    boost::shared_ptr<const Mantid::Geometry::IMDDimension> getDimension(std::string id) const
    {
    }

    /// Get the dimension ids in their order
    const std::vector<std::string> getDimensionIDs() const
    {
    }

    /// Get the point at the specified index.
    const Mantid::Geometry::SignalAggregate& getPoint(unsigned int index) const
    {
    }

    /// Get the cell at the specified index/increment.
    const Mantid::Geometry::SignalAggregate& getCell(unsigned int dim1Increment) const
    {
    }

    /// Get the cell at the specified index/increment.
    const Mantid::Geometry::SignalAggregate& getCell(unsigned int dim1Increment, unsigned int dim2Increment) const
    {
    }

    /// Get the cell at the specified index/increment.
    const Mantid::Geometry::SignalAggregate& getCell(unsigned int dim1Increment, unsigned int dim2Increment, unsigned int dim3Increment) const
    {
    }

    /// Get the cell at the specified index/increment.
    const Mantid::Geometry::SignalAggregate& getCell(unsigned int dim1Increment, unsigned int dim2Increment, unsigned int dim3Increment, unsigned int dim4Increment) const
    {
    }

    /// Get the cell at the specified index/increment.
    const Mantid::Geometry::SignalAggregate& getCell(...) const
    {
      throw Mantid::Kernel::Exception::NotImplementedError("MDHistoWorkspace does not support more than 4 dimensions!");
    }

    /// Horace sytle implementations need to have access to the underlying file.
    std::string getWSLocation() const
    {
      return "";
    }

    /// All MD type workspaces have an effective geometry. MD type workspaces must provide this geometry in a serialized format.
    std::string getGeometryXML() const
    {
      throw Mantid::Kernel::Exception::NotImplementedError("Not yet!");
    }


  private:

    /// Linear array of signals for each bin
    double * m_signals;

    /// Linear array of errors for each bin
    double * m_errors;



  };


} // namespace Mantid
} // namespace MDEvents

#endif  /* MANTID_MDEVENTS_MDHISTOWORKSPACE_H_ */
