/******************************************************************************
 * Python module wrapper for the WorkspaceCreationHelper methods
 ******************************************************************************/

#include <boost/python/module.hpp>
#include <boost/python/docstring_options.hpp>
#include <boost/python/def.hpp>

#include "MantidTestHelpers/WorkspaceCreationHelper.h"

BOOST_PYTHON_MODULE(WorkspaceCreationHelper)
{
  using namespace boost::python;

  // Doc string options - User defined, python arguments, C++ call signatures
  docstring_options docstrings(true, true, false);
}
