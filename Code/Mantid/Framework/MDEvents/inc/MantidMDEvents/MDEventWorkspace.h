#ifndef MDEVENTWORKSPACE_H_
#define MDEVENTWORKSPACE_H_

#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidDataObjects/Peak.h"
#include "MantidKernel/ProgressBase.h"
#include "MantidKernel/System.h"
#include "MantidMDEvents/BoxController.h"
#include "MantidMDEvents/BoxController.h"
#include "MantidAPI/CoordTransform.h"
#include "MantidMDEvents/IMDBox.h"
#include "MantidMDEvents/MDLeanEvent.h"
#include "MantidMDEvents/MDGridBox.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"

namespace Mantid
{
namespace MDEvents
{

  /** Templated class for the multi-dimensional event workspace.
   *
   * @tparam MDE :: the type of MDEvent in the workspace. This can be, e.g.
   *                MDLeanEvent<nd> or MDEvent<nd>
   * @tparam nd :: the number of dimensions that each MDEvent will be tracking.
   *               an usigned int > 0.
   *               nd must match the number of dimensions in the MDE type!
   *
   *
   * @author Janik Zikovsky, SNS
   * @date Dec 3, 2010
   *
   * */
  TMDE_CLASS
  class DLLExport MDEventWorkspace  : public API::IMDEventWorkspace
  {
  public:
    MDEventWorkspace();
    MDEventWorkspace(const MDEventWorkspace<MDE,nd> & other);
    virtual ~MDEventWorkspace();

    /// Perform initialization after dimensions (and others) have been set.
    virtual void initialize();

    virtual const std::string id() const;

    /** @returns the number of dimensions in this workspace */
    virtual size_t getNumDims() const;

    /** @returns the total number of points (events) in this workspace */
    virtual uint64_t getNPoints() const;

    /** @returns the number of bytes of memory used by the workspace. */
    virtual size_t getMemorySize() const;

    /// Returns the BoxController used in this workspace
    BoxController_sptr getBoxController()
    { return m_BoxController; }

    /// @return true if the workspace is file-backed
    virtual bool isFileBacked() const
    { return m_BoxController->isFileBacked(); }


    virtual std::vector<std::string> getBoxControllerStats() const;

    Mantid::API::ITableWorkspace_sptr makeBoxTable(size_t start, size_t num);

    void splitBox();

    void splitAllIfNeeded(Kernel::ThreadScheduler * ts);

    void refreshCache();

    void addEvent(const MDE & event);

    size_t addEvents(const std::vector<MDE> & events);

    void addManyEvents(const std::vector<MDE> & events, Mantid::Kernel::ProgressBase * prog);

    std::vector<Mantid::Geometry::MDDimensionExtents> getMinimumExtents(size_t depth=2);

    /// Return true if the underlying box is a MDGridBox.
    bool isGridBox()
    {
      return (dynamic_cast<MDGridBox<MDE,nd> *>(data) != NULL);
    }

    /** @returns a pointer to the box (MDBox or MDGridBox) contained within, */
    IMDBox<MDE,nd> * getBox()
    {
      return data;
    }

    /** @returns a pointer to the box (MDBox or MDGridBox) contained within, const version.  */
    const IMDBox<MDE,nd> * getBox() const
    {
      return data;
    }

    /** Set the base-level box contained within.
     * Used in file loading */
    void setBox(IMDBox<MDE,nd> * box)
    {
      data = box;
    }


  protected:

    /** MDBox containing all of the events in the workspace. */
    IMDBox<MDE, nd> * data;

    /// Box controller in use
    BoxController_sptr m_BoxController;

  public:
    /// Typedef for a shared pointer of this kind of event workspace
    typedef boost::shared_ptr<MDEventWorkspace<MDE, nd> > sptr;


  };





}//namespace MDEvents

}//namespace Mantid


#endif /* MDEVENTWORKSPACE_H_ */
