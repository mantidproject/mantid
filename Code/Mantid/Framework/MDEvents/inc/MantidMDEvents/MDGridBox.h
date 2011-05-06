#ifndef MDGRIDBOX_H_
#define MDGRIDBOX_H_

#include "MantidAPI/IMDWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidKernel/ProgressBase.h"
#include "MantidMDEvents/BoxController.h"
#include "MantidMDEvents/IMDBox.h"
#include "MantidMDEvents/MDBox.h"
//#include "MantidMDEvents/MDBoxTask.h"
#include "MantidMDEvents/MDDimensionExtents.h"
#include "MantidMDEvents/MDEvent.h"
#include "MantidKernel/ThreadScheduler.h"
#include "MantidKernel/Task.h"
#include "MantidKernel/ProgressBase.h"

namespace Mantid
{
namespace MDEvents
{

  //===============================================================================================
  /** Templated class for a GRIDDED multi-dimensional event "box".
   * A MDGridBox contains a dense array with nd dimensions
   * of IMDBox'es, each being either a regular MDBox or a MDGridBox itself.
   *
   * This means that MDGridBoxes can be recursively gridded finer and finer.
   *
   * @tparam nd :: the number of dimensions that each MDEvent will be tracking.
   *                  an int > 0.
   *
   * @author Janik Zikovsky, SNS
   * @date Dec 7, 2010
   *
   * */
  TMDE_CLASS
  class DLLExport MDGridBox : public IMDBox<MDE, nd>
  {
  public:
    MDGridBox(MDBox<MDE, nd> * box);

    virtual ~MDGridBox();

    void clear();

    size_t getNPoints() const;

    size_t getNumDims() const;

    size_t getNumMDBoxes() const;

    std::vector< MDE > * getEventsCopy();

    void addEvent(const MDE & point);

    size_t addEvents(const std::vector<MDE> & events);

    size_t addEvents(const std::vector<MDE> & events, const size_t start_at, const size_t stop_at);

    void centerpointBin(MDBin<MDE,nd> & bin, bool * fullyContained) const;

    void integrateSphere(CoordTransform & radiusTransform, const CoordType radiusSquared, double & signal, double & errorSquared) const;

    void splitContents(size_t index, Kernel::ThreadScheduler * ts = NULL);

    void splitAllIfNeeded(Kernel::ThreadScheduler * ts = NULL);

    void refreshCache(Kernel::ThreadScheduler * ts = NULL);

//    void runMDBoxTask(MDBoxTask<MDE,nd> * task, const bool fullyContained);

    // ======================= Testing/Debugging Methods =================
    /** For testing: get 9a copy of) the vector of boxes */
    std::vector<IMDBox<MDE, nd>*> getBoxes()
    { return boxes; }


  public:
    /// Typedef for a shared pointer to a MDGridBox
    typedef boost::shared_ptr< MDGridBox<MDE, nd> > sptr;

    /// Typedef for a vector of IMDBox pointers
    typedef std::vector<IMDBox<MDE, nd>*> boxVector_t;


  private:

    /// Each dimension is split into this many equally-sized boxes
    size_t split[nd];

    /** Cumulative dimension splitting: split[n] = 1*split[0]*split[..]*split[n-1]
     */
    size_t splitCumul[nd];

    /** 1D array of boxes contained within. These map
     * to the nd-array.
     */
    std::vector<IMDBox<MDE, nd>*> boxes;

    /// How many boxes in the boxes vector? This is just to avoid boxes.size() calls.
    size_t numBoxes;

    /// Size of each box size in the i^th dimension
    CoordType boxSize[nd];

    /** Length (squared) of the diagonal through every dimension = sum( boxSize[i]^2 )
     * Used in some calculations like peak integration */
    CoordType diagonalSquared;

    /// Cached number of points contained (including all sub-boxes)
    size_t nPoints;

    /// Mutex for counting points and total signal
    Mantid::Kernel::Mutex statsMutex;


    //=================== PRIVATE METHODS =======================================

    size_t getLinearIndex(size_t * indices) const;




  public:

    //===============================================================================================
    //===============================================================================================
    /** Task for adding events to a MDGridBox. */
    class AddEventsTask : public Mantid::Kernel::Task
    {
    public:
      /// Pointer to MDGridBox.
      MDGridBox<MDE, nd> * box;
      /// Reference to the MD events that will be added
      const std::vector<MDE> & events;
      /// Where to start in vector
      size_t start_at;
      /// Where to stop in vector
      size_t stop_at;
      /// Progress report
      Mantid::Kernel::ProgressBase * prog;

      /** Ctor
       *
       * @param box :: Pointer to MDGridBox
       * @param events :: Reference to the MD events that will be added
       * @param start_at :: Where to start in vector
       * @param stop_at :: Where to stop in vector
       * @return
       */
      AddEventsTask(MDGridBox<MDE, nd> * box, const std::vector<MDE> & events,
                    const size_t start_at, const size_t stop_at, Mantid::Kernel::ProgressBase * prog)
      : Mantid::Kernel::Task(),
        box(box), events(events), start_at(start_at), stop_at(stop_at), prog(prog)
      {
      }

      /// Add the events in the MDGridBox.
      void run()
      {
        box->addEvents(events, start_at, stop_at);
        if (prog)
        {
          std::ostringstream out;
          out << "Adding events " << start_at;
          prog->report(out.str());
        }
      }
    };



  };






}//namespace MDEvents

}//namespace Mantid

#endif /* MDGRIDBOX_H_ */
