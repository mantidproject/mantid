#ifndef IMDEVENTWORKSPACE_H_
#define IMDEVENTWORKSPACE_H_

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/Workspace.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidGeometry/MDGeometry/MDDimensionExtents.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidKernel/ProgressBase.h"
#include <boost/shared_ptr.hpp>

namespace Mantid
{
namespace API
{

  /** Abstract base class for multi-dimension event workspaces (MDEventWorkspace).
   * This class will handle as much of the common operations as possible;
   * but since MDEventWorkspace is a templated class, that makes some aspects
   * impossible.
   *
   * @author Janik Zikovsky, SNS
   * @date Dec 3, 2010
   *
   * */
  class MANTID_API_DLL IMDEventWorkspace  : public API::IMDWorkspace
  {
  public:
    IMDEventWorkspace();
    IMDEventWorkspace(const IMDEventWorkspace & other);

    /// Perform initialization after dimensions (and others) have been set.
    virtual void initialize() = 0;

    /// Get the minimum extents that hold the data
    virtual std::vector<Mantid::Geometry::MDDimensionExtents> getMinimumExtents(size_t depth=2) = 0;

    /// Returns some information about the box controller, to be displayed in the GUI, for example
    virtual std::vector<std::string> getBoxControllerStats() const = 0;

    /// Helper method that makes a table workspace with some box data
    virtual Mantid::API::ITableWorkspace_sptr makeBoxTable(size_t start, size_t num) = 0;

    /// @return true if the workspace is file-backed
    virtual bool isFileBacked() const = 0;


    ExperimentInfo_sptr getExperimentInfo(const uint16_t runIndex);

    ExperimentInfo_const_sptr getExperimentInfo(const uint16_t runIndex) const;

    uint16_t addExperimentInfo(ExperimentInfo_sptr ei);

    void setExperimentInfo(const uint16_t runIndex, ExperimentInfo_sptr ei);

    uint16_t getNumExperimentInfo() const;

  protected:
    /// Vector for each ExperimentInfo class
    std::vector<ExperimentInfo_sptr> m_expInfos;

  };

  /// Shared pointer to a generic IMDEventWorkspace
  typedef boost::shared_ptr<IMDEventWorkspace> IMDEventWorkspace_sptr;

  /// Shared pointer to a generic const IMDEventWorkspace
  typedef boost::shared_ptr<const IMDEventWorkspace> IMDEventWorkspace_const_sptr;


}//namespace MDEvents

}//namespace Mantid

#endif /* IMDEVENTWORKSPACE_H_ */
