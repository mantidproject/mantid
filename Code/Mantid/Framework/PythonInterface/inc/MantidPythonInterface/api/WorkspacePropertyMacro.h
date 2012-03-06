#ifndef MANTID_PYTHONINTERFACE_WORKSPACEPROPERTYMACRO_H_
#define MANTID_PYTHONINTERFACE_WORKSPACEPROPERTYMACRO_H_
/*
  Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
#include "MantidPythonInterface/kernel/PropertyWithValue.h"
#include "MantidAPI/WorkspaceProperty.h"
#include <boost/python/register_ptr_to_python.hpp>

/**
 * Defines a macro for exporting new WorkspaceProperty types.
 * This automatically exports a new PropertyWithValue class for the given type.
 *
 * @param Type :: The workspace type (not the shared_ptr wrapped type)
 * @param ClassName :: A string defining the final class name in Python

 */
#define EXPORT_WORKSPACE_PROPERTY(Type, ClassName) \
  EXPORT_PROP_W_VALUE(boost::shared_ptr<Type>, Type); \
  boost::python::register_ptr_to_python<Mantid::API::WorkspaceProperty<Type>*>(); \
  boost::python::class_<Mantid::API::WorkspaceProperty<Type>,\
         boost::python::bases<Mantid::Kernel::PropertyWithValue<boost::shared_ptr<Type> >, Mantid::API::IWorkspaceProperty>,\
         boost::noncopyable>(ClassName, boost::python::no_init) \
      ;

#endif /* MANTID_PYTHONINTERFACE_WORKSPACEPROPERTYMACRO_H_ */
