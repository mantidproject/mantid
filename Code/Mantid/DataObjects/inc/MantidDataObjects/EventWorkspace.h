#ifndef MANTID_DATAOBJECTS_EVENTWORKSPACE_H_
#define MANTID_DATAOBJECTS_EVENTWORKSPACE_H_ 1

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

#include <string>
#include "MantidKernel/Logger.h"
#include "MantidKernel/System.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/EventList.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "boost/date_time/posix_time/posix_time.hpp"

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



///Map to EventList objects, with an int as the index.
typedef std::map<const int, EventList*> EventListMap;
typedef std::vector<EventList*> EventListVector;


/** \class EventWorkspace

    This class is intended to fulfill the design specified in 
    <https://svn.mantidproject.org/mantid/trunk/Documents/Design/Event WorkspaceDetailed Design Document.doc>
 */

class DLLExport EventWorkspace : public API::MatrixWorkspace
{
 public:
  /// The name of the workspace type.
  virtual const std::string id() const {return "EventWorkspace";}

  /// Constructor
  EventWorkspace();

  /// Destructor
  virtual ~EventWorkspace();

  /// Initialize the pixels */
  void init(const int&, const int&, const int&);

  //------------------------------------------------------------

  /// Returns the number of single indexable items in the workspace
  int size() const;

  /// Get the blocksize, aka the number of bins in the histogram
  int blocksize() const;

  /// Get the number of histograms. aka the number of pixels or detectors.
  const int getNumberHistograms() const;

  //------------------------------------------------------------

  /// Return the data X vector at a given workspace index
  MantidVec& dataX(const int);

  /// Return the data Y vector at a given workspace index
  MantidVec& dataY(const int);

  /// Return the data E vector at a given workspace index
  MantidVec& dataE(const int);


  /// Return the const data X vector at a given workspace index
  const MantidVec& dataX(const int) const;

  /// Return the const data Y vector at a given workspace index
  const MantidVec& dataY(const int) const;

  /// Return the const data E vector at a given workspace index
  const MantidVec& dataE(const int) const;

  /// Get a pointer to the x data at the given workspace index
  Kernel::cow_ptr<MantidVec> refX(const int) const;

  //------------------------------------------------------------

  /// Set the x-axis data for the given pixel.
  void setX(const int, const  Kernel::cow_ptr<MantidVec> &);

  /// Set the x-axis data (histogram bins) for all pixels
  void setAllX(Kernel::cow_ptr<MantidVec> &x);

  /// Get an EventList object at the given pixelid/spectrum number
  EventList& getEventList(const int pixelid);

  /// Get an EventList object at the given workspace index number
  EventList& getEventListAtWorkspaceIndex(const int spectrum_number);

  /// Call this method when loading event data is complete.
  void doneLoadingData();


  //------------------------------------------------------------
  /// Set the size of the MRU (most recently used) histogram cache
  //void setMRUSize(const int size) const;

  //------------------------------------------------------------
  /// Get the absolute time corresponding to the give frame ID
  boost::posix_time::ptime getTime(const size_t frameId);

  /// Add the absolute time corresponding to the give frame ID
  void addTime(const size_t frameId, boost::posix_time::ptime absoluteTime);

  /// The total number of events across all of the spectra.
  std::size_t getNumberEvents() const;

  /// Returns true always - an EventWorkspace always represents histogramm-able data
  virtual const bool isHistogramData() const;

private:
  /// NO COPY ALLOWED
  EventWorkspace(const EventWorkspace&);
  /// NO ASSIGNMENT ALLOWED
  EventWorkspace& operator=(const EventWorkspace&);

  /// A map that holds the event list for each pixel; the key is the pixelid.
  EventListMap data_map;

  /// Set to true when loading data is finished.
  bool done_loading_data;

  /** A vector that holds the event list for each pixel; the key is
   * the spectrum #, which is not necessarily the pixelid.
   */
  mutable EventListVector data;

  /// Static reference to the logger class
  static Kernel::Logger & g_log;

  /// The number of vectors in the workspace
  int m_noVectors;

  /// Vector where the index is the frameId and the value is the corresponding time, in Posix timing.
  std::vector<boost::posix_time::ptime> frameTime;
};

///shared pointer to the EventWorkspace class
typedef boost::shared_ptr<EventWorkspace> EventWorkspace_sptr;
///shared pointer to a const Workspace2D
typedef boost::shared_ptr<const EventWorkspace> EventWorkspace_const_sptr;



} /// namespace DataObjects

} /// namespace Mantid

#endif /* MANTID_DATAOBJECTS_EVENTWORKSPACE_H_ */
