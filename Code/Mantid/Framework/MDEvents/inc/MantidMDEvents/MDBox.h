#ifndef MDBOX_H_
#define MDBOX_H_

#include "MantidAPI/IMDWorkspace.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/System.h"
#include "MantidMDEvents/BoxController.h"
#include "MantidMDEvents/IMDBox.h"
#include "MantidGeometry/MDGeometry/MDDimensionExtents.h"
#include "MantidMDEvents/MDDimensionStats.h"
#include "MantidMDEvents/MDEvent.h"
#include "MantidAPI/ImplicitFunction.h"

#undef MDBOX_TRACK_SIGNAL_WHEN_ADDING

namespace Mantid
{
namespace MDEvents
{

  //===============================================================================================
  /** Templated class for a multi-dimensional event "box".
   *
   * A box is a container of MDEvent's within a certain range of values
   * within the nd dimensions. This range defines a n-dimensional "box"
   * or rectangular prism.
   *
   * This class is a simple list of points with no more internal structure.
   *
   * @tparam nd :: the number of dimensions that each MDEvent will be tracking.
   *                  an int > 0.
   *
   * @author Janik Zikovsky, SNS
   * @date Dec 7, 2010
   *
   * */
  TMDE_CLASS
  class DLLExport MDBox : public IMDBox<MDE, nd>
  {
  public:
    MDBox();

    MDBox(BoxController_sptr splitter, const size_t depth = 0);

    virtual ~MDBox() {}

    void clear();

    size_t getNPoints() const;

    size_t getNumDims() const;

    size_t getNumMDBoxes() const;

    /// Get the # of children IMDBox'es (non-recursive)
    size_t getNumChildren() const
    { return 0; }

    /// Return the indexth child IMDBox.
    IMDBox<MDE,nd> * getChild(size_t /*index*/)
        { throw std::runtime_error("MDBox does not have children."); }

    /// Sets the children from a vector of children
    void setChildren(const std::vector<IMDBox<MDE,nd> *> & /*boxes*/, const size_t /*indexStart*/, const size_t /*indexEnd*/)
    { throw std::runtime_error("MDBox cannot have children."); }



    /// @return Start point in the NXS file where the events are located
    uint64_t getFileIndexStart() const { return m_fileIndexStart; }

    /// @return Number of events saved in the file, after the start index location
    uint64_t getFileNumEvents() const { return m_fileNumEvents; }

    void setFileIndex(uint64_t start, uint64_t numEvents);


    /** Set whether the box is saved on disk (true) or in memory (false)
     * @param onDisk :: true if it is on disk  */
    void setOnDisk(const bool onDisk)
    { m_onDisk = onDisk; }

    /// @return whether the box is saved on disk (true) or in memory (false)
    bool getOnDisk() const
    { return m_onDisk; }



    std::vector< MDE > & getEvents();

    const std::vector< MDE > & getEvents() const;

    void releaseEvents() const;


    std::vector< MDE > * getEventsCopy();

    void addEvent(const MDE & point);

    size_t addEvents(const std::vector<MDE> & events)
    {
      return this->addEvents(events, 0, events.size());
    }

    size_t addEvents(const std::vector<MDE> & events, const size_t start_at, const size_t stop_at);

    void centerpointBin(MDBin<MDE,nd> & bin, bool * fullyContained) const;

    void generalBin(MDBin<MDE,nd> & bin, Mantid::API::ImplicitFunction & function) const;

    void calculateDimensionStats(MDDimensionStats * stats) const;

    void integrateSphere(CoordTransform & radiusTransform, const coord_t radiusSquared, signal_t & signal, signal_t & errorSquared) const;

    void centroidSphere(CoordTransform & radiusTransform, const coord_t radiusSquared, coord_t * centroid, signal_t & signal) const;

    void refreshCache(Kernel::ThreadScheduler * /*ts*/ = NULL);

    void refreshCentroid(Kernel::ThreadScheduler * /*ts*/ = NULL);

    void saveNexus(::NeXus::File * file) const;

    void loadNexus(::NeXus::File * file);

    void getBoxes(std::vector<IMDBox<MDE,nd> *> & boxes, size_t /*maxDepth*/, bool /*leafOnly*/);

    void getBoxes(std::vector<IMDBox<MDE,nd> *> & boxes, size_t maxDepth, bool leafOnly, Mantid::Geometry::MDImplicitFunction * function);

  protected:

    /** Vector of MDEvent's, in no particular order.
     * */
    mutable std::vector< MDE > data;

    /// Mutex for modifying the event list
    Mantid::Kernel::Mutex dataMutex;

    /// Start point in the NXS file where the events are located
    uint64_t m_fileIndexStart;

    /// Number of events saved in the file, after the start index location
    uint64_t m_fileNumEvents;

    /// True when the events are on disk and not in memory.
    bool m_onDisk;

  public:
    /// Typedef for a shared pointer to a MDBox
    typedef boost::shared_ptr< MDBox<MDE, nd> > sptr;

    /// Typedef for a vector of the conatined events
    typedef std::vector< MDE > vec_t;


  };







}//namespace MDEvents

}//namespace Mantid

#endif /* MDBOX_H_ */
