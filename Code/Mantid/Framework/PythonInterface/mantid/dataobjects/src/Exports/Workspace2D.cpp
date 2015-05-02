#include "MantidDataObjects/Workspace2D.h"
#include <boost/python/class.hpp>

using Mantid::API::MatrixWorkspace;
using Mantid::DataObjects::Workspace2D;
using namespace boost::python;

void export_Workspace2D()
{
  class_<Workspace2D, bases<MatrixWorkspace>,
         boost::noncopyable>("Workspace2D", no_init)
    ;
}
