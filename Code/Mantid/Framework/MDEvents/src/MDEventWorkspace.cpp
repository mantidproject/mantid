#include "MantidMDEvents/IMDBox.h"
#include "MantidMDEvents/MDBox.h"
#include "MantidMDEvents/MDEvent.h"
#include "MantidMDEvents/MDEventWorkspace.h"

namespace Mantid
{
namespace MDEvents
{

//-----------------------------------------------------------------------------------------------
  /** Default constructor
   */
  TMDE(
  MDEventWorkspace)::MDEventWorkspace()
  {
    data = new MDBox<MDE, nd>();
  }

  //-----------------------------------------------------------------------------------------------
  /** Destructor
   */
  TMDE(
  MDEventWorkspace)::~MDEventWorkspace()
  {
    delete data;
  }

  TMDE(
  const std::string MDEventWorkspace)::id() const
  {
    std::ostringstream out;
    out << "MDEventWorkspace<MDEvent," << getNumDims() << ">";
    return out.str();
  }


  //-----------------------------------------------------------------------------------------------
  /** Returns the number of dimensions in this workspace */
  TMDE(
  int MDEventWorkspace)::getNumDims() const
  {
    return nd;
  }

  //-----------------------------------------------------------------------------------------------
  /** Returns the total number of points (events) in this workspace */
  TMDE(
  size_t MDEventWorkspace)::getNPoints() const
  {
    //return data.size();
    return 0;
  }

  //-----------------------------------------------------------------------------------------------
  /** Returns the number of bytes of memory
   * used by the workspace. */
  TMDE(
  size_t MDEventWorkspace)::getMemorySize() const
  {
    return this->getNPoints() * sizeof(MDE);
  }


  //-----------------------------------------------------------------------------------------------
  /** Add a vector of MDEvents to the workspace.
   *
   * @param events :: const ref. to a vector of events; they will be copied.
   */
  TMDE(
  void MDEventWorkspace)::addEvents(const std::vector<MDE> & events)
  {
  }





  template DLLExport class MDEventWorkspace<MDEvent<1>, 1>;
  template DLLExport class MDEventWorkspace<MDEvent<2>, 2>;
  template DLLExport class MDEventWorkspace<MDEvent<3>, 3>;
  template DLLExport class MDEventWorkspace<MDEvent<4>, 4>;
  template DLLExport class MDEventWorkspace<MDEvent<5>, 5>;
  template DLLExport class MDEventWorkspace<MDEvent<6>, 6>;
  template DLLExport class MDEventWorkspace<MDEvent<7>, 7>;
  template DLLExport class MDEventWorkspace<MDEvent<8>, 8>;
  template DLLExport class MDEventWorkspace<MDEvent<9>, 9>;


}//namespace MDEvents

}//namespace Mantid

