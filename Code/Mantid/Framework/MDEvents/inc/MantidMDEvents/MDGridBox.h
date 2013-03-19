#ifndef MDGRIDBOX_H_
#define MDGRIDBOX_H_

#include "MantidAPI/IMDWorkspace.h"
#include "MantidGeometry/MDGeometry/MDDimensionExtents.h"
#include "MantidKernel/ProgressBase.h"
#include "MantidKernel/ProgressBase.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Task.h"
#include "MantidKernel/ThreadScheduler.h"
#include "MantidMDEvents/MDBoxBase.h"
#include "MantidMDEvents/MDBox.h"
#include "MantidMDEvents/MDLeanEvent.h"

namespace Mantid
{
namespace MDEvents
{

#pragma pack(push, 4) //Ensure the structure is no larger than it needs to

  //===============================================================================================
  /** Templated class for a GRIDDED multi-dimensional event "box".
   * A MDGridBox contains a dense array with nd dimensions
   * of MDBoxBase'es, each being either a regular MDBox or a MDGridBox itself.
   *
   * This means that MDGridBoxes can be recursively gridded finer and finer.
   *
   * @tparam nd :: the number of dimensions that each MDLeanEvent will be tracking.
   *                  an int > 0.
   *
   * @author Janik Zikovsky, SNS
   * @date Dec 7, 2010
   *
   * */
  TMDE_CLASS
  class DLLExport MDGridBox : public MDBoxBase<MDE, nd>
  {
  public:
    MDGridBox();

    MDGridBox(Mantid::API::BoxController *const bc, const uint32_t depth, const std::vector<Mantid::Geometry::MDDimensionExtents<coord_t> > & extentsVector);

    MDGridBox(MDBox<MDE, nd> * box, bool splitRecursively=false);

    MDGridBox(const MDGridBox<MDE, nd> & box,Mantid::API::BoxController *const otherBC);

    virtual ~MDGridBox();
    // ----------------------------- ISaveable Methods ------------------------------------------------------
    virtual Kernel::ISaveable *const getISaveable(){return NULL;}
    virtual Kernel::ISaveable *const getISaveable()const{return NULL;}
    //-------------------------------------------------------------------------------------------------------
    void clear();


    /** Returns the total number of points (events) in this box  (in memory and in file if present)*/
    uint64_t getNPoints() const //Use the cached value    
    {    return nPoints; }
    /// @return the amount of memory that the object takes up in t.
    virtual uint64_t getTotalDataSize() const
    {   return nPoints; }
   size_t  getDataInMemorySize()const;

   size_t getNumDims() const;

   size_t getNumMDBoxes() const;

   size_t getNumChildren() const;

   //TODO: -- The meaning of this stuff have changed
    //size_t getChildIndexFromID(size_t childId) const;

    API::IMDNode * getChild(size_t index);
    //void setChild(size_t index,MDGridBox<MDE,nd> * newChild)
    //{
    //  // Delete the old box  (supposetly ungridded);
    //  delete this->boxes[index];
    //  // set new box, supposetly gridded
    //  this->boxes[index]=newChild;
    //}

    void setChildren(const std::vector<API::IMDNode *> & boxes, const size_t indexStart, const size_t indexEnd);

    void getBoxes(std::vector<API::IMDNode *> & boxes, size_t maxDepth, bool leafOnly);
    void getBoxes(std::vector<API::IMDNode *> & boxes, size_t maxDepth, bool leafOnly, Mantid::Geometry::MDImplicitFunction * function);

    const API::IMDNode * getBoxAtCoord(const coord_t * coords);

    void transformDimensions(std::vector<double> & scaling, std::vector<double> & offset);
    //----------------------------------------------------------------------------

    std::vector< MDE > * getEventsCopy();

    //----------------------------------------------------------------------------------------------------------------------
    void addEvent(const MDE & event);
    void addEventUnsafe(const MDE & event);
    void addAndTraceEvent(const MDE & point,size_t index);
    // unhide MDBoxBase methods
    virtual size_t addEvents(const std::vector<MDE> & events)
    { return MDBoxBase::addEvents(events); }
    virtual size_t addEventsUnsafe(const std::vector<MDE> & events)
    {return MDBoxBase::addEventsUnsafe( events);}


    /*--------------->  EVENTS from event data              <-------------------------------------------------------------*/
    virtual void addEvent(const signal_t Signal,const  signal_t errorSq,const std::vector<coord_t> &point, uint16_t runIndex,uint32_t detectorId);
    virtual void addAndTraceEvent(const signal_t Signal,const signal_t errorSq,const std::vector<coord_t> &point, uint16_t runIndex,uint32_t detectorId,size_t index);
    virtual void addEventUnsafe(const signal_t Signal,const  signal_t errorSq,const std::vector<coord_t> &point, uint16_t runIndex,uint32_t detectorId);
    virtual size_t addEvents(const std::vector<signal_t> &sigErrSq,const  std::vector<coord_t> &Coord,const std::vector<uint16_t> &runIndex,const std::vector<uint32_t> &detectorId);
    //----------------------------------------------------------------------------------------------------------------------

    void centerpointBin(MDBin<MDE,nd> & bin, bool * fullyContained) const;

    void generalBin(MDBin<MDE,nd> & /*bin*/, Mantid::Geometry::MDImplicitFunction & /*function*/) const {};

    void integrateSphere(Mantid::API::CoordTransform & radiusTransform, const coord_t radiusSquared, signal_t & signal, signal_t & errorSquared) const;

    void centroidSphere(Mantid::API::CoordTransform & radiusTransform, const coord_t radiusSquared, coord_t * centroid, signal_t & signal) const;

    void splitContents(size_t index, Kernel::ThreadScheduler * ts = NULL);
    //void splitContentsById(size_t childId); // No definition causes a myriad of warnings on MSVC

    void splitAllIfNeeded(Kernel::ThreadScheduler * ts = NULL);

    //void cacheChildrenToDisk(); // No definition causes a myriad of warnings on MSVC

    void refreshCache(Kernel::ThreadScheduler * ts = NULL);

    void refreshCentroid(Kernel::ThreadScheduler * ts = NULL);

    // Set the box controller overrriden.
    //virtual void setBoxController(Mantid::API::BoxController *controller);
 
    virtual bool getIsMasked() const;
    ///Setter for masking the box
    virtual void mask();
    ///Setter for unmasking the box
    virtual void unmask();
    // ======================= Testing/Debugging Methods =================
    /** For testing: get (a reference to) the vector of boxes */
    std::vector<MDBoxBase<MDE,nd> *> & getBoxes()
    { return m_Children; }


//------------------------------------------------------------------------- 
  /** The function used to satisfy IMDNode interface but the physical meaning is unclear */
  void calculateCentroid(coord_t *  /*centroid*/) const
  {
      throw(std::runtime_error("This function should not be called on MDGridBox (as its meaning for MDbox is dubious too)"));
  }
  public:
    /// Typedef for a shared pointer to a MDGridBox
    typedef boost::shared_ptr< MDGridBox<MDE, nd> > sptr;

    /// Typedef for a vector of MDBoxBase pointers
    typedef std::vector<MDBoxBase<MDE, nd>*> boxVector_t;


  private:
    /// Each dimension is split into this many equally-sized boxes
    size_t split[nd];
    /** Cumulative dimension splitting: split[n] = 1*split[0]*split[..]*split[n-1]     */
    size_t splitCumul[nd];
    /// size of each sub-box (the one this GridBox can be split into) in correspondent direction
    double m_SubBoxSize[nd];

    /// How many boxes in the boxes vector? This is just to avoid boxes.size() calls.
    size_t numBoxes;
  
    /** 1D array of boxes contained within. These map
     * to the nd-array.     */
    std::vector<MDBoxBase<MDE,nd> *> m_Children;

   /** Length (squared) of the diagonal through every dimension = sum( boxSize[i]^2 )
     * Used in some calculations like peak integration */
    coord_t diagonalSquared;

    /// Cached number of points contained (including all sub-boxes)
    size_t nPoints;

    /// Mutex for counting points and total signal
    Mantid::Kernel::Mutex statsMutex;


    //=================== PRIVATE METHODS =======================================

    size_t getLinearIndex(size_t * indices) const;

    size_t computeSizesFromSplit();
    void fillBoxShell(const size_t tot,const coord_t inverseVolume);

    MDGridBox(const MDGridBox<MDE, nd> & box);
  public:

    ////===============================================================================================
    ////===============================================================================================
    ///** Task for adding events to a MDGridBox. */
    //class AddEventsTask : public Mantid::Kernel::Task
    //{
    //public:
    //  /// Pointer to MDGridBox.
    //  MDBoxBase<MDE, nd> * box;
    //  /// Reference to the MD events that will be added
    //  const std::vector<MDE> & events;
    //  /// Where to start in vector
    //  size_t start_at;
    //  /// Where to stop in vector
    //  size_t stop_at;
    //  /// Progress report
    //  Mantid::Kernel::ProgressBase * prog;

    //  /** Ctor
    //   *
    //   * @param box :: Pointer to MDGridBox
    //   * @param events :: Reference to the MD events that will be added
    //   * @param start_at :: Where to start in vector
    //   * @param stop_at :: Where to stop in vector
    //   * @param prog :: ProgressReporting
    //   * @return
    //   */
    //  AddEventsTask(MDBoxBase<MDE, nd> * box, const std::vector<MDE> & events,
    //                const size_t start_at, const size_t stop_at, Mantid::Kernel::ProgressBase * prog)
    //  : Mantid::Kernel::Task(),
    //    box(box), events(events), start_at(start_at), stop_at(stop_at), prog(prog)
    //  {
    //  }

    //  /// Add the events in the MDGridBox.
    //  void run()
    //  {
    //    box->addEvents(events);
    //    if (prog)
    //    {
    //      std::ostringstream out;
    //      out << "Adding events " << start_at;
    //      prog->report(out.str());
    //    }
    //  }
    //};



  };


#pragma pack(pop) //Return to default packing size




}//namespace MDEvents

}//namespace Mantid

#endif /* MDGRIDBOX_H_ */
