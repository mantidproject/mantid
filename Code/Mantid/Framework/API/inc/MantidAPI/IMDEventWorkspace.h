#ifndef IMDEVENTWORKSPACE_H_
#define IMDEVENTWORKSPACE_H_

#include "MantidAPI/Workspace.h"
#include "MantidKernel/System.h"
#include "MantidAPI/Dimension.h"
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
  class DLLExport IMDEventWorkspace  : public API::Workspace
  {
  public:

    /// Perform initialization after dimensions (and others) have been set.
    virtual void initialize() = 0;

    /** Returns the number of dimensions in this workspace */
    virtual size_t getNumDims() const = 0;

    /** Returns the total number of points (events) in this workspace */
    virtual size_t getNPoints() const = 0;

    /// Add a new dimension
    virtual void addDimension(Dimension dimInfo);

    /// Get that dimension
    virtual Dimension getDimension(size_t dim);

  protected:
    /// Vector with each dimension (length must match nd)
    std::vector<Dimension> dimensions;

  };

  /// Shared pointer to a generic IMDEventWorkspace
  typedef boost::shared_ptr<IMDEventWorkspace> IMDEventWorkspace_sptr;


}//namespace MDEvents

}//namespace Mantid

#endif /* IMDEVENTWORKSPACE_H_ */
