#ifndef MANTID_DATAOBJECTS_EVENTWORKSPACE_H_
#define MANTID_DATAOBJECTS_EVENTWORKSPACE_H_ 1

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

#include "MantidAPI/IEventWorkspace.h"
#include "MantidAPI/ISpectrum.h"
#include "MantidDataObjects/EventList.h"
#include "MantidKernel/System.h"
#include <boost/date_time/posix_time/posix_time.hpp>
#include <string>

namespace Mantid
{
//----------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------
namespace API
{
  class Progress;
}

namespace DataObjects
{
class EventWorkspaceMRU;

///EventList objects, with the detector ID as the index.
typedef std::vector<EventList*> EventListVector;


//============================================================================
//============================================================================
/** \class EventWorkspace

    This class is intended to fulfill the design specified in 
    <https://github.com/mantidproject/documents/tree/master/Design/Event WorkspaceDetailed Design Document.doc>
 */

class DLLExport EventWorkspace : public API::IEventWorkspace
{

 public:
  // The name of the workspace type.
  virtual const std::string id() const {return "EventWorkspace";}

  // Constructor
  EventWorkspace();

  // Destructor
  virtual ~EventWorkspace();

  // Initialize the pixels
  void init(const std::size_t&, const std::size_t&, const std::size_t&);

  void copyDataFrom(const EventWorkspace& source,
                    std::size_t sourceStartWorkspaceIndex=0, std::size_t sourceEndWorkspaceIndex=size_t(-1));

  virtual bool threadSafe() const;

  //------------------------------------------------------------

  // Returns the number of single indexable items in the workspace
  std::size_t size() const;

  // Get the blocksize, aka the number of bins in the histogram
  std::size_t blocksize() const;

  size_t getMemorySize() const;

  // Get the number of histograms. aka the number of pixels or detectors.
  std::size_t getNumberHistograms() const;

  //------------------------------------------------------------
  // Return the underlying ISpectrum ptr at the given workspace index.
  virtual Mantid::API::ISpectrum * getSpectrum(const size_t index);

  // Return the underlying ISpectrum ptr (const version) at the given workspace index.
  virtual const Mantid::API::ISpectrum * getSpectrum(const size_t index) const;

  //------------------------------------------------------------

  double getTofMin() const;

  double getTofMax() const;

  Mantid::Kernel::DateAndTime getPulseTimeMin() const;

  Mantid::Kernel::DateAndTime getPulseTimeMax() const;

  Mantid::Kernel::DateAndTime getTimeAtSampleMin(double tofOffset = 0) const;

  Mantid::Kernel::DateAndTime getTimeAtSampleMax(double tofOffset = 0) const;

  double getEventXMin() const;
  double getEventXMax() const;
  void getEventXMinMax(double &xmin, double &xmax) const;

  //------------------------------------------------------------
  // Return the data X vector at a given workspace index
  MantidVec& dataX(const std::size_t);

  // Return the data Y vector at a given workspace index
  MantidVec& dataY(const std::size_t);

  // Return the data E vector at a given workspace index
  MantidVec& dataE(const std::size_t);

  // Return the X data erro vector at a given workspace index
  MantidVec& dataDx(const std::size_t);


  // Return the const data X vector at a given workspace index
  const MantidVec& dataX(const std::size_t) const;

  // Return the const data Y vector at a given workspace index
  const MantidVec& dataY(const std::size_t) const;

  // Return the const data E vector at a given workspace index
  const MantidVec& dataE(const std::size_t) const;

  // Return the const X data error vector at a given workspace index
  const MantidVec& dataDx(const std::size_t) const;

  // Get a pointer to the x data at the given workspace index
  Kernel::cow_ptr<MantidVec> refX(const std::size_t) const;

  /// Generate a new histogram from specified event list at the given index.
  void generateHistogram(const std::size_t index, const MantidVec& X, MantidVec& Y, MantidVec& E, bool skipError = false) const;

  /// Generate a new histogram from specified event list at the given index.
  void generateHistogramPulseTime(const std::size_t index, const MantidVec& X, MantidVec& Y, MantidVec& E, bool skipError = false) const;

  //------------------------------------------------------------
  // Set the x-axis data (histogram bins) for all pixels
  void setAllX(Kernel::cow_ptr<MantidVec> &x);

  // Get an EventList object at the given workspace index number
  EventList& getEventList(const std::size_t workspace_index);

  // Get a const EventList object at the given workspace index number
  const EventList& getEventList(const std::size_t workspace_index) const;

  // Get an EventList pointer at the given workspace index number
  EventList * getEventListPtr(const std::size_t workspace_index);

  // Get or add an EventList
  EventList& getOrAddEventList(const std::size_t workspace_index);

  // Resizes the workspace to contain the number of spectra/event lists given
  void resizeTo(const std::size_t numSpectra);
  // Pad pixels in the workspace using the loaded spectra. Requires a non-empty spectra-detector map
  void padSpectra();
  // Remove pixels in the workspace that do not contain events.
  void deleteEmptyLists();
  
  //------------------------------------------------------------
  // The total number of events across all of the spectra.
  std::size_t getNumberEvents() const;

  // Type of the events
  Mantid::API::EventType getEventType() const;

  // Change the event type
  void switchEventType(const Mantid::API::EventType type);

  // Returns true always - an EventWorkspace always represents histogramm-able data
  virtual bool isHistogramData() const;

  std::size_t MRUSize() const;

  void clearMRU() const;

  void clearData();

  EventSortType getSortType() const;

  // Sort all event lists. Uses a parallelized algorithm
  void sortAll(EventSortType sortType, Mantid::API::Progress * prog) const;
  void sortAllOld(EventSortType sortType, Mantid::API::Progress * prog) const;

  virtual void getIntegratedSpectra(std::vector<double> & out, const double minX, const double maxX, const bool entireRange) const;

private:
  /// NO COPY ALLOWED
  EventWorkspace(const EventWorkspace&);
  /// NO ASSIGNMENT ALLOWED
  EventWorkspace& operator=(const EventWorkspace&);

  /** A vector that holds the event list for each spectrum; the key is
   * the workspace index, which is not necessarily the pixelid.
   */
  EventListVector data;

  /// The number of vectors in the workspace
  std::size_t m_noVectors;

  /// Container for the MRU lists of the event lists contained.
  mutable EventWorkspaceMRU * mru;
};

///shared pointer to the EventWorkspace class
typedef boost::shared_ptr<EventWorkspace> EventWorkspace_sptr;
///shared pointer to a const Workspace2D
typedef boost::shared_ptr<const EventWorkspace> EventWorkspace_const_sptr;



} /// namespace DataObjects

} /// namespace Mantid

#endif /* MANTID_DATAOBJECTS_EVENTWORKSPACE_H_ */
