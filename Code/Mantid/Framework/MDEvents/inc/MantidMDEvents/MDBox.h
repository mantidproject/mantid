#ifndef MDBOX_H_
#define MDBOX_H_

#include "MantidAPI/IMDWorkspace.h"
#include "MantidGeometry/MDGeometry/MDDimensionExtents.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/System.h"
#include "MantidMDEvents/MDBoxBase.h"
#include "MantidMDEvents/MDDimensionStats.h"
#include "MantidMDEvents/MDLeanEvent.h"
#include "MantidKernel/ThreadScheduler.h"

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
  class DLLExport MDBox :  public MDBoxBase<MDE, nd>
  {
  public:
    MDBox();

    MDBox(Mantid::API::BoxController_sptr splitter, const size_t depth = 0,int64_t boxSize=-1,int64_t boxID=-1);

    MDBox(Mantid::API::BoxController_sptr splitter, const size_t depth, const std::vector<Mantid::Geometry::MDDimensionExtents<coord_t> > & extentsVector, int64_t boxSize=-1,int64_t boxID=-1);

    MDBox(const MDBox & other);

    virtual ~MDBox() {}


    // ----------------------------- ISaveable Methods ------------------------------------------------------

    /// Save the data to the place, specified by the object
    virtual void save()const;

    /// Load the data which are not in memory yet and merge them with the data in memory;
    virtual void load();
    /** Method sets the attribite which tells that data were loaded in memory. 

        Usually one should not use this method  as load operation sets this attribute by itself 
        but in some specific situations where the data were loaded from extermal source and exchanged with the box and the
        data file position in new place is known this attribute should be set to true manually, to not allowing attempt to download 
        data again (see mergeMD files where target box data were loaded from the external source and been added to the box
    */
    void setLoaded()
    {
    m_isLoaded=true;
    }
   
    /// @return the amount of memory that the object takes up in the MRU.
    virtual uint64_t getTotalDataSize() const
            { return getNPoints(); }
  /** @return the size of the event vector. ! Note that this is NOT necessarily the same as the number of points 
    (because it might be cached to disk) or the size on disk (because you might have called AddEvents) */
    virtual size_t getDataMemorySize()const 
            {  return data.size();}
    virtual bool isBox()const{return true;}
    //-----------------------------------------------------------------------------------------------

    void clear();
    /** Remove box data from memory */
    void clearDataFromMemory();

    uint64_t getNPoints() const;

    size_t getNumDims() const;

    size_t getNumMDBoxes() const;

    /// Get the # of children MDBoxBase'es (non-recursive)
    size_t getNumChildren() const
    { return 0; }

    /// Return the indexth child MDBoxBase.
    MDBoxBase<MDE,nd> * getChild(size_t /*index*/)
        { throw std::runtime_error("MDBox does not have children."); }

    /// Sets the children from a vector of children
    void setChildren(const std::vector<MDBoxBase<MDE,nd> *> & /*boxes*/, const size_t /*indexStart*/, const size_t /*indexEnd*/)
    { throw std::runtime_error("MDBox cannot have children."); }


    /// @return whether the box data (from disk) is loaded in memory.
    bool getInMemory() const
    { return m_isLoaded; }

   
    /// @return true if events were added to the box (using addEvent()) while the rest of the event list is cached to disk
    bool isDataAdded() const
    {
      if(m_isLoaded)
        return data.size()!=this->getFileSize();
      else
        return (data.size() != 0);
    }

 
    /* Getter to determine if masking is applied.
    @return true if masking is applied.
    */
    virtual bool getIsMasked() const
    {
      return m_bIsMasked;
    }
    /**Get vector of events to change. Beware, that calling this funtion for file-based workspace sets both dataChanged and dataBusy flags
       first forces disk buffer to write the object contents to HDD when disk buffer is full and the second one prevents DB 
       from clearing object from memory untill the events are released. One HAS TO call releaseEvents when finished using data on file-based WS    */ 
    std::vector< MDE > & getEvents();
    /**Get vector of constant events to use. Beware, that calling this funtion for file-based workspace sets dataBusy flag
       This flag prevents DB from clearing object from memory untill the events are released.
       One HAS TO call releaseEvents when finished using data on file-based WS to allow DB clearing them  */ 
    const std::vector<MDE> & getConstEvents()const ;
    // the same as getConstEvents above, 
    const std::vector< MDE > & getEvents()const;

    void releaseEvents() ;

    std::vector< MDE > * getEventsCopy();


    void addEvent(const MDE & point);
    void addAndTraceEvent(const MDE & point,size_t index);
    void addEventUnsafe(const MDE & point);
    size_t addEventsPart(const std::vector<MDE> & events, const size_t start_at, const size_t stop_at);
    size_t addEventsPartUnsafe(const std::vector<MDE> & events, const size_t start_at, const size_t stop_at);

    void centerpointBin(MDBin<MDE,nd> & bin, bool * fullyContained) const;

    void generalBin(MDBin<MDE,nd> & bin, Mantid::Geometry::MDImplicitFunction & function) const;

    void calculateDimensionStats(MDDimensionStats * stats) const;

    void integrateSphere(Mantid::API::CoordTransform & radiusTransform, const coord_t radiusSquared, signal_t & signal, signal_t & errorSquared) const;

    void centroidSphere(Mantid::API::CoordTransform & radiusTransform, const coord_t radiusSquared, coord_t * centroid, signal_t & signal) const;

    void refreshCache(Kernel::ThreadScheduler * /*ts*/ = NULL);

    void refreshCentroid(Kernel::ThreadScheduler * /*ts*/ = NULL);

    void calculateCentroid(coord_t * centroid) const;

    void saveNexus(::NeXus::File * file) const;

    void loadNexus(::NeXus::File * file, bool setLoaded=true);

    void getBoxes(std::vector<MDBoxBase<MDE,nd> *> & boxes, size_t /*maxDepth*/, bool /*leafOnly*/);
    void getBoxes(std::vector<Kernel::ISaveable *> & boxes, size_t /*maxDepth*/, bool /*leafOnly*/);

    void getBoxes(std::vector<MDBoxBase<MDE,nd> *> & boxes, size_t maxDepth, bool leafOnly, Mantid::Geometry::MDImplicitFunction * function);
    void getBoxes(std::vector<Kernel::ISaveable *> & boxes, size_t maxDepth, bool leafOnly, Mantid::Geometry::MDImplicitFunction * function);

    void transformDimensions(std::vector<double> & scaling, std::vector<double> & offset);

    ///Setter for masking the box
    void mask();

    ///Setter for unmasking the box
    void unmask();

  protected:

  
    /** Vector of MDLeanEvent's, in no particular order.
     * */
    mutable std::vector< MDE > data;

    /// Mutex for modifying the event list
    Mantid::Kernel::Mutex dataMutex;

    /// True when the events, which were saved before have been loaded up from disk. Load sets it true and memory clean-up resets it to false
    mutable bool m_isLoaded;

    /// Flag indicating that masking has been applied.
    bool m_bIsMasked;

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
