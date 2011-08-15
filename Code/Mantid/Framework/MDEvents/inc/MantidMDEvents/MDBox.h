#ifndef MDBOX_H_
#define MDBOX_H_

#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/ImplicitFunction.h"
#include "MantidGeometry/MDGeometry/MDDimensionExtents.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/System.h"
#include "MantidMDEvents/BoxController.h"
#include "MantidMDEvents/IMDBox.h"
#include "MantidMDEvents/MDDimensionStats.h"
#include "MantidMDEvents/MDLeanEvent.h"

#undef MDBOX_TRACK_SIGNAL_WHEN_ADDING

namespace Mantid
{
namespace MDEvents
{

#pragma pack(push, 4) //Ensure the structure is no larger than it needs to

  //===============================================================================================
  /** Templated class for a multi-dimensional event "box".
   *
   * A box is a container of MDLeanEvent's within a certain range of values
   * within the nd dimensions. This range defines a n-dimensional "box"
   * or rectangular prism.
   *
   * This class is a simple list of points with no more internal structure.
   *
   * @tparam nd :: the number of dimensions that each MDLeanEvent will be tracking.
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

    MDBox(BoxController_sptr splitter, const size_t depth, const std::vector<Mantid::Geometry::MDDimensionExtents> & extentsVector);

    MDBox(const MDBox & other);

    virtual ~MDBox() {}


    // ----------------------------- ISaveable Methods ------------------------------------------------------

    /// Save the data
    virtual void save() const;

    /// Load the data - unused
    virtual void load()
    { }

    /// @return the amount of memory that the object takes up in the MRU.
    virtual uint64_t getSizeOnFile() const
    { return m_fileNumEvents; }

    /// @return true if it the data of the object is busy and so cannot be cleared by the MRU; false if the data was released and can be cleared/written.
    virtual bool dataBusy() const
    { return m_dataBusy; }

    /** @return the position in the file where the data will be stored. This is used to optimize file writing. */
    virtual uint64_t getFilePosition() const
    { return m_fileIndexStart; }

    //-----------------------------------------------------------------------------------------------


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


    /** Set whether the box is cached on disk (true) or in memory (false)
     * @param onDisk :: true if it is on disk  */
    void setOnDisk(const bool onDisk)
    { m_onDisk = onDisk; }

    /// @return whether the box is cached on disk (true) or in memory (false)
    bool getOnDisk() const
    { return m_onDisk; }

    /// @return whether the box data (from disk) is loaded in memory (for debugging purposes).
    bool getInMemory() const
    { return m_inMemory; }

    /// @return the size of the event vector. FOR DEBUGGING! Note that this is NOT necessarily the same as the number of points (because it might be cached to disk) or the size on disk (because you might have called AddEvents)
    size_t getEventVectorSize() const
    { return data.size(); }


    std::vector< MDE > & getEvents();

    const std::vector<MDE> & getConstEvents() const;

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

    void calculateCentroid(coord_t * centroid) const;

    void saveNexus(::NeXus::File * file) const;

    void loadNexus(::NeXus::File * file);

    void getBoxes(std::vector<IMDBox<MDE,nd> *> & boxes, size_t /*maxDepth*/, bool /*leafOnly*/);

    void getBoxes(std::vector<IMDBox<MDE,nd> *> & boxes, size_t maxDepth, bool leafOnly, Mantid::Geometry::MDImplicitFunction * function);

  protected:

    /** Vector of MDLeanEvent's, in no particular order.
     * */
    mutable std::vector< MDE > data;

    /// Mutex for modifying the event list
    Mantid::Kernel::Mutex dataMutex;

    /// Is the "data" vector currently in use by some algorithm?
    mutable bool m_dataBusy;

    /** Marker set to true when the data was accessed in a CONST getEvents() method
     * (so it has not changed and does not need to be saved).  */
    mutable bool m_dataConstAccess;

    /// Start point in the NXS file where the events are located
    mutable uint64_t m_fileIndexStart;

    /// Number of events saved in the file, after the start index location
    mutable uint64_t m_fileNumEvents;

    /// True when the events are cached to disk. If false, then the events are ALWAYS kept in memory
    bool m_onDisk;

    /// True when the events were loaded up from disk. Irrelevant if m_onDisk is false.
    mutable bool m_inMemory;

  public:
    /// Typedef for a shared pointer to a MDBox
    typedef boost::shared_ptr< MDBox<MDE, nd> > sptr;

    /// Typedef for a vector of the conatined events
    typedef std::vector< MDE > vec_t;


  };

#pragma pack(pop) //Return to default packing size





}//namespace MDEvents

}//namespace Mantid

#endif /* MDBOX_H_ */
