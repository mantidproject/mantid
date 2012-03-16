#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidPythonInterface/kernel/Policies/upcast_returned_value.h"

#include <boost/python/class.hpp>
#include <boost/python/overloads.hpp>

using namespace boost::python;
using namespace Mantid::API;
namespace Policies = Mantid::PythonInterface::Policies;

namespace
{
  /// Overload generator for create
  BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(createFromParent_Overload, create, 1, 4);
  BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(createTable_Overload, createTable, 0, 1);
  BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(createPeaks_Overload, createPeaks, 0, 1);
}

void export_WorkspaceFactory()
{
  const char * createFromParentDoc = "Create a workspace based on the given one. The meta-data, instrumen etc are copied from the input"
      "If the size parameters are passed then the workspace will be a differenet size.";
  typedef MatrixWorkspace_sptr (WorkspaceFactoryImpl::*createFromParentPtr)
       (const MatrixWorkspace_const_sptr&,size_t, size_t, size_t) const;

  const char * createFromScratchDoc = "Create a clean new worksapce of the given size.";
  typedef MatrixWorkspace_sptr (WorkspaceFactoryImpl::*createFromScratchPtr)
       (const std::string&,const size_t&, const size_t&, const size_t&) const;


  class_<WorkspaceFactoryImpl,boost::noncopyable>("WorkspaceFactoryImpl", no_init)
    .def("create", (createFromParentPtr)&WorkspaceFactoryImpl::create,
         createFromParent_Overload(createFromParentDoc,
                                   (arg("parent"), arg("NVectors")=-1, arg("XLength")=-1, arg("YLength")=-1))[return_value_policy<Policies::upcast_returned_value>()])

    .def("create", (createFromScratchPtr)&WorkspaceFactoryImpl::create, return_value_policy<Policies::upcast_returned_value>(),
        createFromScratchDoc, (arg("className"), arg("NVectors"), arg("XLength"), arg("YLength")))

    .def("createTable", &WorkspaceFactoryImpl::createTable,
         createTable_Overload("Creates an empty TableWorkspace", (arg("className")="TableWorkspace")))

    .def("createPeaks", &WorkspaceFactoryImpl::createPeaks,
          createPeaks_Overload("Creates an empty PeaksWorkspace", (arg("className")="PeaksWorkspace")))

    .def("Instance", &WorkspaceFactory::Instance, return_value_policy<reference_existing_object>(),
         "Returns the single instance of this class.")
    .staticmethod("Instance")
  ;
}

