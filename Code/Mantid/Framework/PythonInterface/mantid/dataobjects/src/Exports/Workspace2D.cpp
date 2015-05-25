#include "MantidDataObjects/Workspace2D.h"
#include "MantidPythonInterface/kernel/Registry/DataItemInterface.h"

#include <boost/python/class.hpp>
#include <boost/python/object/inheritance.hpp>

using Mantid::API::MatrixWorkspace;
using Mantid::DataObjects::Workspace2D;
using namespace Mantid::PythonInterface::Registry;
using namespace boost::python;

void export_Workspace2D()
{
  class_<Workspace2D, bases<MatrixWorkspace>,
         boost::noncopyable>("Workspace2D", no_init)
    ;

  // register pointers
  DataItemInterface<Workspace2D>();
}
