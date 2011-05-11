#ifndef MANTID_DATAOBJECTS_EVENTWORKSPACE_H_
#define MANTID_DATAOBJECTS_EVENTWORKSPACE_H_ 1

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

#include <string>
#include "MantidKernel/MRUList.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/System.h"
#include "MantidAPI/IEventWorkspace.h"
#include "MantidDataObjects/EventList.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAPI/Progress.h"

#include <boost/date_time/posix_time/posix_time.hpp>

namespace Mantid
{
//----------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------
namespace Kernel
{
  class Logger;
}

namespace DataObjects
{

//============================================================================
//============================================================================
/**
 * This little class holds a MantidVec of data and an index marker that
 * is used for uniqueness.
 * This is used in the MRUList.
 *
 */
class MantidVecWithMarker {
public:
  /**
   * Constructor.
   * @param the_index :: unique index into the workspace of this data
   */
  MantidVecWithMarker(const int the_index)
  : m_index(the_index)
  {
  }

  /// Destructor
  ~MantidVecWithMarker()
  {
    m_data.clear();
    //Trick to release the allocated memory
    // MantidVec().swap(m_data);
  }


private:
  /// Copy constructor
  MantidVecWithMarker(const MantidVecWithMarker &other)
  {
    (void) other; //Avoid compiler warning
    throw Kernel::Exception::NotImplementedError("Cannot copy MantidVecWithMarker.");
  }

  /// Assignment operator
  MantidVecWithMarker& operator=(const MantidVecWithMarker &other)
  {
    (void) other; //Avoid compiler warning
    throw Kernel::Exception::NotImplementedError("Cannot assign to MantidVecWithMarker.");
//    m_index = other.m_index;
//    m_data.assign(other.m_data.begin(), other.m_data.end());
    return *this;
  }

public:
  /// Unique index value.
  int m_index;

  /// Pointer to a vector of data
  MantidVec m_data;

  /// Function returns a unique index, used for hashing for MRU list
  int hashIndexFunction() const
  {
    return m_index;
  }

  /// Set the unique index value.
  void setIndex(const int the_index)
  {
    m_index = the_index;
  }
};


///Map to EventList objects, with an int as the index.
typedef std::map<const int, EventList*> EventListMap;
typedef std::vector<EventList*> EventListVector;



//============================================================================
//============================================================================
/** \class EventWorkspace

    This class is intended to fulfill the design specified in 
    <https://svn.mantidproject.org/mantid/trunk/Documents/Design/Event WorkspaceDetailed Design Document.doc>
 */

class DLLExport EventWorkspace : public API::IEventWorkspace
{

 public:
  /// Typedef for a Most-Recently-Used list of Data objects.
  typedef Mantid::Kernel::MRUList<MantidVecWithMarker> mru_list;
  /// Typedef for a vector of MRUlists.
  typedef std::vector< mru_list * > mru_lists;

  /// The name of the workspace type.
  virtual const std::string id() const {return "EventWorkspace";}

  /// Constructor
  EventWorkspace();

  /// Destructor
  virtual ~EventWorkspace();

  /// Initialize the pixels */
  void init(const int&, const int&, const int&);

  virtual bool threadSafe() const;

  //------------------------------------------------------------

  /// Returns the number of single indexable items in the workspace
  int size() const;

  /// Get the blocksize, aka the number of bins in the histogram
  int blocksize() const;

  /// Get the number of histograms. aka the number of pixels or detectors.
  int getNumberHistograms() const;

  size_t getMemorySize() const;

  void copyDataFrom(const EventWorkspace& source,
      int sourceStartWorkspaceIndex=-1, int sourceEndWorkspaceIndex=-1);

  double getTofMin() const;

  double getTofMax() const;

  //------------------------------------------------------------
  void ensureEnoughBuffersY(int thread_num) const;
  void ensureEnoughBuffersE(int thread_num) const;

  /// Return the data X vector at a given workspace index
  MantidVec& dataX(const int);

  /// Return the data Y vector at a given workspace index
  MantidVec& dataY(const int);

  /// Return the data E vector at a given workspace index
  MantidVec& dataE(const int);

  /// Return the X data erro vector at a given workspace index
  MantidVec& dataDx(const int);


  /// Return the const data X vector at a given workspace index
  const MantidVec& dataX(const int) const;

  /// Return the const data Y vector at a given workspace index
  const MantidVec& dataY(const int) const;

  /// Return the const data E vector at a given workspace index
  const MantidVec& dataE(const int) const;

  /// Return the const X data error vector at a given workspace index
  const MantidVec& dataDx(const int) const;

  /// Get a pointer to the x data at the given workspace index
  Kernel::cow_ptr<MantidVec> refX(const int) const;

  void readYE(int const index, MantidVec const*& Y, MantidVec const*& E) const;

  //------------------------------------------------------------

  /// Set the x-axis data for the given pixel via cow_ptr.
  void setX(const int, const  Kernel::cow_ptr<MantidVec> &);
  /// Set the x-axis data for the given pixel via MantidVec.
  void setX(const int, const MantidVec &);

  /// Set the x-axis data (histogram bins) for all pixels
  void setAllX(Kernel::cow_ptr<MantidVec> &x);

  /// Get an EventList object at the given pixelid/spectrum number
  EventList& getEventListAtPixelID(const int pixelid);

  /// Get an EventList object at the given workspace index number
  EventList& getEventList(const int workspace_index);

  /// Get a const EventList object at the given workspace index number
  const EventList& getEventList(const int workspace_index) const;

  /// Get an EventList pointer at the given workspace index number
  EventList * getEventListPtr(const int workspace_index);

  /// Get or add an EventList
  EventList& getOrAddEventList(const int workspace_index);

  /// Pad pixels in the workspace using the detectors in the instrument.
  void padPixels(bool parallel);

  /// Make all the mapping stuff
  void makeSpectraMap();
  void makeAxis1();
  void doneAddingEventLists();

  /// Call this method when loading event data is complete.
  void doneLoadingData();


  //------------------------------------------------------------
  /// The total number of events across all of the spectra.
  std::size_t getNumberEvents() const;

  /// Type of the events
  Mantid::API::EventType getEventType() const;

  /// Change the event type
  void switchEventType(const Mantid::API::EventType type);

  /// Returns true always - an EventWorkspace always represents histogramm-able data
  virtual bool isHistogramData() const;

  int MRUSize() const;

  void clearMRU() const;

  void clearData();

  /// Sort all event lists. Uses a parallelized algorithm
  void sortAll(EventSortType sortType, Mantid::API::Progress * prog) const;
  void sortAllOld(EventSortType sortType, Mantid::API::Progress * prog) const;

  Kernel::DateAndTime getFirstPulseTime() const;
  Kernel::DateAndTime getLastPulseTime() const;

  virtual void getIntegratedSpectra(std::vector<double> & out, const double minX, const double maxX, const bool entireRange) const;

private:
  /// NO COPY ALLOWED
  EventWorkspace(const EventWorkspace&);
  /// NO ASSIGNMENT ALLOWED
  EventWorkspace& operator=(const EventWorkspace&);

  /** A map that holds the event list for each pixel; the key is the pixelid;
   * this is used when loading data;
   * this is NOT guaranteed to be ok after DoneLoadingData; Don't use it!
   */
  EventListMap data_map;

  /// Set to true when loading data is finished.
  bool done_loading_data;

  /** A vector that holds the event list for each spectrum; the key is
   * the workspace index, which is not necessarily the pixelid.
   */
  EventListVector data;

  /// Static reference to the logger class
  static Kernel::Logger & g_log;

  /// The number of vectors in the workspace
  int m_noVectors;

  /// The most-recently-used list of dataY histograms
  mutable mru_lists m_bufferedDataY;

  /// The most-recently-used list of dataE histograms
  mutable mru_lists m_bufferedDataE;

  /// Cached copy of the # of events in the list, cached at the end of doneLoadingData()
  mutable std::size_t m_cachedNumberOfEvents;


};

///shared pointer to the EventWorkspace class
typedef boost::shared_ptr<EventWorkspace> EventWorkspace_sptr;
///shared pointer to a const Workspace2D
typedef boost::shared_ptr<const EventWorkspace> EventWorkspace_const_sptr;



} /// namespace DataObjects

} /// namespace Mantid

#endif /* MANTID_DATAOBJECTS_EVENTWORKSPACE_H_ */
