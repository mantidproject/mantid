#ifndef MANTID_MDEVENTS_MDEVENTFACTORY_H_
#define MANTID_MDEVENTS_MDEVENTFACTORY_H_

#include "MantidKernel/System.h"
#include <boost/shared_ptr.hpp>
#include <MantidMDEvents/MDEventFactory.h>
#include <MantidAPI/IMDEventWorkspace.h>
#include <MantidMDEvents/MDEventWorkspace.h>
#include <MantidMDEvents/MDBin.h>


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

  //### BEGIN AUTO-GENERATED CODE #################################################################
  
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
  



  // ------------- Typedefs for MDBox ------------------

  /// Typedef for a MDBox with 1 dimension 
  typedef MDBox<MDEvent<1>, 1> MDBox1;
  /// Typedef for a MDBox with 2 dimensions 
  typedef MDBox<MDEvent<2>, 2> MDBox2;
  /// Typedef for a MDBox with 3 dimensions 
  typedef MDBox<MDEvent<3>, 3> MDBox3;
  /// Typedef for a MDBox with 4 dimensions 
  typedef MDBox<MDEvent<4>, 4> MDBox4;
  /// Typedef for a MDBox with 5 dimensions 
  typedef MDBox<MDEvent<5>, 5> MDBox5;
  /// Typedef for a MDBox with 6 dimensions 
  typedef MDBox<MDEvent<6>, 6> MDBox6;
  /// Typedef for a MDBox with 7 dimensions 
  typedef MDBox<MDEvent<7>, 7> MDBox7;
  /// Typedef for a MDBox with 8 dimensions 
  typedef MDBox<MDEvent<8>, 8> MDBox8;
  /// Typedef for a MDBox with 9 dimensions 
  typedef MDBox<MDEvent<9>, 9> MDBox9;



  // ------------- Typedefs for IMDBox ------------------

  /// Typedef for a IMDBox with 1 dimension 
  typedef IMDBox<MDEvent<1>, 1> IMDBox1;
  /// Typedef for a IMDBox with 2 dimensions 
  typedef IMDBox<MDEvent<2>, 2> IMDBox2;
  /// Typedef for a IMDBox with 3 dimensions 
  typedef IMDBox<MDEvent<3>, 3> IMDBox3;
  /// Typedef for a IMDBox with 4 dimensions 
  typedef IMDBox<MDEvent<4>, 4> IMDBox4;
  /// Typedef for a IMDBox with 5 dimensions 
  typedef IMDBox<MDEvent<5>, 5> IMDBox5;
  /// Typedef for a IMDBox with 6 dimensions 
  typedef IMDBox<MDEvent<6>, 6> IMDBox6;
  /// Typedef for a IMDBox with 7 dimensions 
  typedef IMDBox<MDEvent<7>, 7> IMDBox7;
  /// Typedef for a IMDBox with 8 dimensions 
  typedef IMDBox<MDEvent<8>, 8> IMDBox8;
  /// Typedef for a IMDBox with 9 dimensions 
  typedef IMDBox<MDEvent<9>, 9> IMDBox9;



  // ------------- Typedefs for MDGridBox ------------------

  /// Typedef for a MDGridBox with 1 dimension 
  typedef MDGridBox<MDEvent<1>, 1> MDGridBox1;
  /// Typedef for a MDGridBox with 2 dimensions 
  typedef MDGridBox<MDEvent<2>, 2> MDGridBox2;
  /// Typedef for a MDGridBox with 3 dimensions 
  typedef MDGridBox<MDEvent<3>, 3> MDGridBox3;
  /// Typedef for a MDGridBox with 4 dimensions 
  typedef MDGridBox<MDEvent<4>, 4> MDGridBox4;
  /// Typedef for a MDGridBox with 5 dimensions 
  typedef MDGridBox<MDEvent<5>, 5> MDGridBox5;
  /// Typedef for a MDGridBox with 6 dimensions 
  typedef MDGridBox<MDEvent<6>, 6> MDGridBox6;
  /// Typedef for a MDGridBox with 7 dimensions 
  typedef MDGridBox<MDEvent<7>, 7> MDGridBox7;
  /// Typedef for a MDGridBox with 8 dimensions 
  typedef MDGridBox<MDEvent<8>, 8> MDGridBox8;
  /// Typedef for a MDGridBox with 9 dimensions 
  typedef MDGridBox<MDEvent<9>, 9> MDGridBox9;



  // ------------- Typedefs for MDEventWorkspace ------------------

  /// Typedef for a MDEventWorkspace with 1 dimension 
  typedef MDEventWorkspace<MDEvent<1>, 1> MDEventWorkspace1;
  /// Typedef for a MDEventWorkspace with 2 dimensions 
  typedef MDEventWorkspace<MDEvent<2>, 2> MDEventWorkspace2;
  /// Typedef for a MDEventWorkspace with 3 dimensions 
  typedef MDEventWorkspace<MDEvent<3>, 3> MDEventWorkspace3;
  /// Typedef for a MDEventWorkspace with 4 dimensions 
  typedef MDEventWorkspace<MDEvent<4>, 4> MDEventWorkspace4;
  /// Typedef for a MDEventWorkspace with 5 dimensions 
  typedef MDEventWorkspace<MDEvent<5>, 5> MDEventWorkspace5;
  /// Typedef for a MDEventWorkspace with 6 dimensions 
  typedef MDEventWorkspace<MDEvent<6>, 6> MDEventWorkspace6;
  /// Typedef for a MDEventWorkspace with 7 dimensions 
  typedef MDEventWorkspace<MDEvent<7>, 7> MDEventWorkspace7;
  /// Typedef for a MDEventWorkspace with 8 dimensions 
  typedef MDEventWorkspace<MDEvent<8>, 8> MDEventWorkspace8;
  /// Typedef for a MDEventWorkspace with 9 dimensions 
  typedef MDEventWorkspace<MDEvent<9>, 9> MDEventWorkspace9;



  // ------------- Typedefs for MDBin ------------------

  /// Typedef for a MDBin with 1 dimension 
  typedef MDBin<MDEvent<1>, 1> MDBin1;
  /// Typedef for a MDBin with 2 dimensions 
  typedef MDBin<MDEvent<2>, 2> MDBin2;
  /// Typedef for a MDBin with 3 dimensions 
  typedef MDBin<MDEvent<3>, 3> MDBin3;
  /// Typedef for a MDBin with 4 dimensions 
  typedef MDBin<MDEvent<4>, 4> MDBin4;
  /// Typedef for a MDBin with 5 dimensions 
  typedef MDBin<MDEvent<5>, 5> MDBin5;
  /// Typedef for a MDBin with 6 dimensions 
  typedef MDBin<MDEvent<6>, 6> MDBin6;
  /// Typedef for a MDBin with 7 dimensions 
  typedef MDBin<MDEvent<7>, 7> MDBin7;
  /// Typedef for a MDBin with 8 dimensions 
  typedef MDBin<MDEvent<8>, 8> MDBin8;
  /// Typedef for a MDBin with 9 dimensions 
  typedef MDBin<MDEvent<9>, 9> MDBin9;


  //### END AUTO-GENERATED CODE ##################################################################

} // namespace Mantid
} // namespace MDEvents

#endif  /* MANTID_MDEVENTS_MDEVENTFACTORY_H_ */








