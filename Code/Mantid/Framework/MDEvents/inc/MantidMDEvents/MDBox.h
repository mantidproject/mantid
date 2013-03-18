#ifndef MDBOX_H_
#define MDBOX_H_



#include "MantidKernel/ThreadScheduler.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/System.h"
#include "MantidGeometry/MDGeometry/MDDimensionExtents.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidMDEvents/MDBoxBase.h"
#include "MantidMDEvents/MDDimensionStats.h"
#include "MantidMDEvents/MDLeanEvent.h"


#undef MDBOX_TRACK_SIGNAL_WHEN_ADDING

namespace Mantid
{
namespace MDEvents
{

#pragma pack(push, 4) //Ensure the structure is no larger than it needs to
    // predefinition;
    class MDBoxSaveable;
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
    MDBox(Mantid::API::BoxController *const splitter, const uint32_t depth = 0,
                        const size_t nBoxEvents=UNDEF_SIZET,const size_t boxID=UNDEF_SIZET);

    MDBox(Mantid::API::BoxController *const splitter, const uint32_t depth, const std::vector<Mantid::Geometry::MDDimensionExtents<coord_t> > & extentsVector,
                       const size_t nBoxEvents=UNDEF_SIZET,const size_t boxID=UNDEF_SIZET);

    MDBox(const MDBox<MDE,nd> & other,Mantid::API::BoxController *const otherBC);

    virtual ~MDBox();


    // ----------------------------- ISaveable Methods ------------------------------------------------------
    virtual Kernel::ISaveable *const getISaveable();
    virtual Kernel::ISaveable *const getISaveable()const;   
    //-----------------------------------------------------------------------------------------------
    void clear();


    uint64_t getNPoints() const;
    size_t getDataInMemorySize()const{return data.size();}
    uint64_t getTotalDataSize()const{return getNPoints();}

    size_t getNumDims() const;

    size_t getNumMDBoxes() const;

    /// Get the # of children MDBoxBase'es (non-recursive)
    size_t getNumChildren() const
    { return 0; }

    /// Return the indexth child MDBoxBase.
    API::IMDNode * getChild(size_t /*index*/)
        { throw std::runtime_error("MDBox does not have children."); }

    /// Sets the children from a vector of children
    void setChildren(const std::vector<API::IMDNode *> & /*boxes*/, const size_t /*indexStart*/, const size_t /*indexEnd*/)
    { throw std::runtime_error("MDBox cannot have children."); }

   
    /// @return true if events were added to the box (using addEvent()) while the rest of the event list is cached to disk
    bool isDataAdded() const;

 
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

    virtual void getEventsData(std::vector<coord_t> &coordTable,size_t &nColumns)const ;
    virtual void setEventsData(const std::vector<coord_t> &coordTable);


    void addEvent(const MDE & Evnt);
    void addAndTraceEvent(const MDE & point,size_t index);
    void addEventUnsafe(const MDE & Evnt);
    size_t addEventsPart(const std::vector<MDE> & events, const size_t start_at, const size_t stop_at);
    size_t addEventsPartUnsafe(const std::vector<MDE> & events, const size_t start_at, const size_t stop_at);
    size_t addEvents(const std::vector<MDE> & events);

    virtual size_t addEvents(const std::vector<signal_t> &sigErrSq,const  std::vector<coord_t> &Coord,const std::vector<uint16_t> &runIndex,const std::vector<uint32_t> &detectorId);

    void centerpointBin(MDBin<MDE,nd> & bin, bool * fullyContained) const;

    void generalBin(MDBin<MDE,nd> & bin, Mantid::Geometry::MDImplicitFunction & function) const;

  //---------------------------------------------------------------------------------------------------------------------------------
    void splitAllIfNeeded(Mantid::Kernel::ThreadScheduler * /*ts*/ = NULL)
    { /* Do nothing with a box default. */ }

    /** Recalculate signal etc. */
    void refreshCache(Kernel::ThreadScheduler * /*ts*/ = NULL);
   /** Calculate the centroid of this box. */
    void refreshCentroid(Kernel::ThreadScheduler * /*ts*/ = NULL)
    {};
    void calculateCentroid(coord_t * centroid) const;

    void calculateDimensionStats(MDDimensionStats * stats) const;

    void integrateSphere(Mantid::API::CoordTransform & radiusTransform, const coord_t radiusSquared, signal_t & signal, signal_t & errorSquared) const;

    void centroidSphere(Mantid::API::CoordTransform & radiusTransform, const coord_t radiusSquared, coord_t * centroid, signal_t & signal) const;
 
  //------------------------------------------------------------------------------------------------------------------------------------
    //void saveNexus(::NeXus::File * file) const;

    //void loadNexus(::NeXus::File * file, bool setLoaded=true);

    void getBoxes(std::vector<MDBoxBase<MDE,nd> *> & boxes, size_t /*maxDepth*/, bool /*leafOnly*/);
    void getBoxes(std::vector<API::IMDNode *> & boxes, size_t /*maxDepth*/, bool /*leafOnly*/);

    void getBoxes(std::vector<MDBoxBase<MDE,nd> *> & boxes, size_t maxDepth, bool leafOnly, Mantid::Geometry::MDImplicitFunction * function);
    void getBoxes(std::vector<API::IMDNode *> & boxes, size_t maxDepth, bool leafOnly, Mantid::Geometry::MDImplicitFunction * function);

    void transformDimensions(std::vector<double> & scaling, std::vector<double> & offset);

    ///Setter for masking the box
    void mask();

    ///Setter for unmasking the box
    void unmask();

  protected:
    // the pointer to the class, responsible for saving/restoring this class to the hdd
    mutable MDBoxSaveable *m_Saveable;
    /// Mutex for modifying the event list
    Mantid::Kernel::Mutex dataMutex;
 
    /** Vector of MDLeanEvent's, in no particular order.
     * */
    mutable std::vector< MDE > data;


     /// Flag indicating that masking has been applied.
    bool m_bIsMasked;
  private:
    void clearDataFromMemory();
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
