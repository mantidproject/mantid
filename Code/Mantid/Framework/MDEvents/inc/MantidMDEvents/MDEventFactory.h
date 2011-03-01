#ifndef MANTID_MDEVENTS_MDEVENTFACTORY_H_
#define MANTID_MDEVENTS_MDEVENTFACTORY_H_
    
#include "MantidKernel/System.h"
#include <boost/shared_ptr.hpp>
#include <MantidMDEvents/MDEventFactory.h>
#include <MantidAPI/IMDEventWorkspace.h>
#include <MantidMDEvents/MDEventWorkspace.h>


namespace Mantid
{
namespace MDEvents
{

  /** MDEventFactory : collection of methods
   * to create MDEvent* instances, by specifying the number
   * of dimensions as a parameter.
   * 
   * @author Janik Zikovsky
   * @date 2011-02-24 15:08:43.105134
   */
  class DLLExport MDEventFactory
  {
  public:
    MDEventFactory() {}
    ~MDEventFactory() {}
    static API::IMDEventWorkspace_sptr CreateMDEventWorkspace(size_t nd, std::string eventType="MDEvent");
  };



/** Macro that makes it possible to call a templated method for
 * a MDEventWorkspace using a IMDEventWorkspace_sptr as the input.
 * @param funcname :: name of the function that will be called.
 * @param workspace :: IMDEventWorkspace_sptr input workspace.
 */
#define CALL_MDEVENT_FUNCTION(funcname, workspace) \
{ \
MDEventWorkspace<MDEvent<1>, 1>::sptr MDEW1 = boost::dynamic_pointer_cast<MDEventWorkspace<MDEvent<1>, 1> >(workspace); \
if (MDEW1) funcname<MDEvent<1>, 1>(MDEW1); \
MDEventWorkspace<MDEvent<2>, 2>::sptr MDEW2 = boost::dynamic_pointer_cast<MDEventWorkspace<MDEvent<2>, 2> >(workspace); \
if (MDEW2) funcname<MDEvent<2>, 2>(MDEW2); \
MDEventWorkspace<MDEvent<3>, 3>::sptr MDEW3 = boost::dynamic_pointer_cast<MDEventWorkspace<MDEvent<3>, 3> >(workspace); \
if (MDEW3) funcname<MDEvent<3>, 3>(MDEW3); \
MDEventWorkspace<MDEvent<4>, 4>::sptr MDEW4 = boost::dynamic_pointer_cast<MDEventWorkspace<MDEvent<4>, 4> >(workspace); \
if (MDEW4) funcname<MDEvent<4>, 4>(MDEW4); \
MDEventWorkspace<MDEvent<5>, 5>::sptr MDEW5 = boost::dynamic_pointer_cast<MDEventWorkspace<MDEvent<5>, 5> >(workspace); \
if (MDEW5) funcname<MDEvent<5>, 5>(MDEW5); \
MDEventWorkspace<MDEvent<6>, 6>::sptr MDEW6 = boost::dynamic_pointer_cast<MDEventWorkspace<MDEvent<6>, 6> >(workspace); \
if (MDEW6) funcname<MDEvent<6>, 6>(MDEW6); \
MDEventWorkspace<MDEvent<7>, 7>::sptr MDEW7 = boost::dynamic_pointer_cast<MDEventWorkspace<MDEvent<7>, 7> >(workspace); \
if (MDEW7) funcname<MDEvent<7>, 7>(MDEW7); \
MDEventWorkspace<MDEvent<8>, 8>::sptr MDEW8 = boost::dynamic_pointer_cast<MDEventWorkspace<MDEvent<8>, 8> >(workspace); \
if (MDEW8) funcname<MDEvent<8>, 8>(MDEW8); \
MDEventWorkspace<MDEvent<9>, 9>::sptr MDEW9 = boost::dynamic_pointer_cast<MDEventWorkspace<MDEvent<9>, 9> >(workspace); \
if (MDEW9) funcname<MDEvent<9>, 9>(MDEW9); \
}


} // namespace Mantid
} // namespace MDEvents

#endif  /* MANTID_MDEVENTS_MDEVENTFACTORY_H_ */
