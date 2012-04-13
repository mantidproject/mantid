#ifndef MDEVENTWORKSPACE_H_
#define MDEVENTWORKSPACE_H_

#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidDataObjects/Peak.h"
#include "MantidKernel/ProgressBase.h"
#include "MantidKernel/System.h"
#include "MantidAPI/BoxController.h"
#include "MantidAPI/BoxController.h"
#include "MantidAPI/CoordTransform.h"
#include "MantidMDEvents/IMDBox.h"
#include "MantidMDEvents/MDLeanEvent.h"
#include "MantidMDEvents/MDGridBox.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/IMDIterator.h"

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

    //------------------------ IMDWorkspace Methods -----------------------------------------

    /** @returns the number of dimensions in this workspace */
    virtual size_t getNumDims() const;

    /** @returns the total number of points (events) in this workspace */
    virtual uint64_t getNPoints() const;

    /// Creates a new iterator pointing to the first cell (box) in the workspace
    virtual std::vector<Mantid::API::IMDIterator*> createIterators(size_t suggestedNumCores = 1,
        Mantid::Geometry::MDImplicitFunction * function = NULL) const;

    /// Returns the (normalized) signal at a given coordinates
    virtual signal_t getSignalAtCoord(const coord_t * coords, const Mantid::API::MDNormalization & normalization) const;

    virtual void getLinePlot(const Mantid::Kernel::VMD & start, const Mantid::Kernel::VMD & end,
        MDNormalization normalize, std::vector<coord_t> & x, std::vector<signal_t> & y, std::vector<signal_t> & e) const;

    //------------------------ (END) IMDWorkspace Methods -----------------------------------------

    /** @returns the number of bytes of memory used by the workspace. */
    virtual size_t getMemorySize() const;

    //------------------------ IMDEventWorkspace Methods -----------------------------------------

    /// Returns the BoxController used in this workspace
    Mantid::API::BoxController_sptr getBoxController()
    { return m_BoxController; }

    /// Returns the BoxController used in this workspace
    Mantid::API::BoxController_const_sptr getBoxController() const
    { return m_BoxController; }

    virtual std::vector<std::string> getBoxControllerStats() const;

    /// @return true if the workspace is file-backed
    virtual bool isFileBacked() const
    { return m_BoxController->isFileBacked(); }

    std::vector<coord_t> estimateResolution() const;

    virtual void splitAllIfNeeded(Kernel::ThreadScheduler * ts);

    void splitTrackedBoxes(Kernel::ThreadScheduler * ts);

    virtual void splitBox();

    virtual void refreshCache();

    std::string getEventTypeName() const;

    virtual void setMinRecursionDepth(size_t minDepth);

    //------------------------ (END) IMDEventWorkspace Methods -----------------------------------------

    Mantid::API::ITableWorkspace_sptr makeBoxTable(size_t start, size_t num);

 
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

    /// Apply masking
    void setMDMasking(Mantid::Geometry::MDImplicitFunction* maskingRegion);

    /// Clear masking
    void clearMDMasking();

  protected:

    /** MDBox containing all of the events in the workspace. */
    IMDBox<MDE, nd> * data;

    /// Box controller in use
    Mantid::API::BoxController_sptr m_BoxController;

  public:
    /// Typedef for a shared pointer of this kind of event workspace
    typedef boost::shared_ptr<MDEventWorkspace<MDE, nd> > sptr;


  };





}//namespace MDEvents

}//namespace Mantid


#endif /* MDEVENTWORKSPACE_H_ */
