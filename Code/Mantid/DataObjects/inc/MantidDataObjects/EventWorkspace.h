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
typedef std::map<const int, EventList> EventListMap;


/** \class EventWorkspace

    This class is intended to fulfill the design specified in 
    <https://svn.mantidproject.org/mantid/trunk/Documents/Design/Event WorkspaceDetailed Design Document.doc>
 */

class DLLExport EventWorkspace : public API::MatrixWorkspace
{
 public:
  /** The name of the workspace type.
      \return Standard name. */
  virtual const std::string id() const {return "EventWorkspace";}

  /// Constructor
  EventWorkspace();

  /// Destructor
  virtual ~EventWorkspace();

  /// Returns the number of single indexable items in the workspace
  int size() const;

  /** Get the blocksize, aka the number of bins in the histogram */
  int blocksize() const;

  /** Get the number of histograms. aka the number of pixels or detectors. */
  const int getNumberHistograms() const;

  /** Return the data X vector at a given pixel. */
  MantidVec& dataX(const int);

  /** Return the data Y vector at a given pixel. */
  MantidVec& dataY(const int);

  /** Return the data E vector at a given pixel. */
  MantidVec& dataE(const int);


  /** Return the const data X vector at a given pixel. */
  const MantidVec& dataX(const int) const;

  /** Return the const data Y vector at a given pixel. */
  const MantidVec& dataY(const int) const;

  /** Return the const data E vector at a given pixel. */
  const MantidVec& dataE(const int) const;


  /** Get a pointer to the x data */
  Kernel::cow_ptr<MantidVec> refX(const int) const;

  /** Set the x-axis data for the given pixel. */
  void setX(const int, const  Kernel::cow_ptr<MantidVec> &);

  /** Set the x-axis data (histogram bins) for all pixels */
  void setAllX(Kernel::cow_ptr<MantidVec> &x);

  /** Initialize the pixels */
  void init(const int&, const int&, const int&);


  /** Get an EventList object at the given pixel number */
  EventList& getEventList(const int index);

private:
  /// NO COPY ALLOWED
  EventWorkspace(const EventWorkspace&);
  /// NO ASSIGNMENT ALLOWED
  EventWorkspace& operator=(const EventWorkspace&);

  /// A vector that holds the event list for each pixel.
  mutable EventListMap data;
  //std::vector<EventList> data;

  /// Static reference to the logger class
  static Kernel::Logger & g_log;

  /// The number of vectors in the workspace
  int m_noVectors;
};

///shared pointer to the EventWorkspace class
typedef boost::shared_ptr<EventWorkspace> EventWorkspace_sptr;
///shared pointer to a const Workspace2D
typedef boost::shared_ptr<const EventWorkspace> EventWorkspace_const_sptr;



} /// namespace DataObjects

} /// namespace Mantid

#endif /* MANTID_DATAOBJECTS_EVENTWORKSPACE_H_ */
