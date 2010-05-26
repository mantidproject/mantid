#ifndef MANTID_DATAOBJECTS_EVENTWORKSPACE_H_
#define MANTID_DATAOBJECTS_EVENTWORKSPACE_H_ 1

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

#include <string>
#include "MantidKernel/Logger.h"
#include "MantidKernel/System.h"
#include "MantidAPI/MatrixWorkspace.h"

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

/** \class EventWorkspace

    This class is intended to fulfill the design specified in 
    <https://svn.mantidproject.org/mantid/trunk/Documents/Design/Event WorkspaceDetailed Design Document.doc>
 */
#ifdef _WIN32
#ifdef IN_MANTID_DATA_OBJECTS
  #define EventWorkspace_DllExport __declspec( dllexport )
#else
  #define EventWorkspace_DllExport __declspec( dllimport )
#endif
#else
  #define EventWorkspace_DllExport
  #define EventWorkspace_DllImport
#endif

class EventWorkspace_DllExport EventWorkspace: public API::MatrixWorkspace
{
 public:
  /** The nae of the workspace type.
      \return Standard name. */
  virtual const std::string id() const {return "EventWorkspace";}

  /// Constructor
  EventWorkspace();

  /// Destructor
  virtual ~EventWorkspace();

  int size() const;

  int blocksize() const;

  const int getNumberHistograms() const;

  MantidVec& dataX(const int);

  MantidVec& dataY(const int);

  MantidVec& dataE(const int);

  MantidVec& dataX(const int) const;

  MantidVec& dataY(const int) const;

  MantidVec& dataE(const int) const;

  Kernel::cow_ptr<MantidVec> refX(const int) const;

  void setX(const int, const  Kernel::cow_ptr<MantidVec> &);

  void init(const int&, const int&, const int&);
private:
  /// NO COPY ALLOWED
  EventWorkspace(const EventWorkspace&);
  /// NO ASSIGNMENT ALLOWED
  EventWorkspace& operator=(const EventWorkspace&);

  /// Static reference to the logger class
  static Kernel::Logger & g_log;
};

}

}

#endif /* MANTID_DATAOBJECTS_EVENTWORKSPACE_H_ */
