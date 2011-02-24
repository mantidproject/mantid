#ifndef MDEVENTWORKSPACE_H_
#define MDEVENTWORKSPACE_H_

#include "MantidAPI/IMDWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidMDEvents/BoxController.h"
#include "MantidMDEvents/IMDBox.h"
#include "MantidMDEvents/IMDEventWorkspace.h"
#include "MantidMDEvents/MDEvent.h"

namespace Mantid
{
namespace MDEvents
{

  /** Templated class for the multi-dimensional event workspace.
   *
   * Template parameters are the same as the MDPoint<> template params, and
   * determine the basic MDPoint type used.
   *
   * @tparam nd :: the number of dimensions that each MDPoint will be tracking.
   *               an usigned int > 0.
   *
   *
   * @author Janik Zikovsky, SNS
   * @date Dec 3, 2010
   *
   * */
  TMDE_CLASS
  class DLLExport MDEventWorkspace  : public IMDEventWorkspace
  {
  public:
    MDEventWorkspace();
    ~MDEventWorkspace();

    virtual const std::string id() const;

    /** Returns the number of dimensions in this workspace */
    virtual int getNumDims() const;

    /** Returns the total number of points (events) in this workspace */
    virtual size_t getNPoints() const;

    /** Returns the number of bytes of memory
     * used by the workspace. */
    virtual size_t getMemorySize() const;

    /** Sample function returning (a copy of) the n-th event in the workspace.
     * This may not be needed.
     *  */
    MDE getEvent(size_t n);

    /// Add a vector of MDEvents to the workspace.
    void addEvents(const std::vector<MDE> & events);

  protected:

    /** MDBox containing all of the events in the workspace. */
    IMDBox<MDE, nd> * data;


    //    /** Typedef of the basic MDEvent data type used in this MDEventWorkspace.
    //     * This is for convenience; an algorithm can declare
    //     *  MyWorkspace::Event someEvent;
    //     * without having to look up template parameters...
    //     */
    //    //typedef MDEvent<nd> Event;


  };





}//namespace MDEvents

}//namespace Mantid


#endif /* MDEVENTWORKSPACE_H_ */
