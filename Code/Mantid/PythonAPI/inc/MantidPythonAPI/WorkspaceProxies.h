#ifndef MANTID_PYTHONAPI_WORKSPACEPROXIES_H_
#define MANTID_PYTHONAPI_WORKSPACEPROXIES_H_

//-----------------------------------
// Includes
//-----------------------------------
#include <MantidAPI/AnalysisDataService.h>
#include <MantidAPI/MatrixWorkspace.h>
#include <MantidAPI/ITableWorkspace.h>
#include <MantidAPI/WorkspaceFactory.h>
#include <boost/python/object.hpp>

namespace Mantid
{

  namespace PythonAPI
  {

    /** 
    Various structures to aid with Python's interaction with Mantid workspaces

    @author ISIS, STFC
    @date 28/02/2008

    Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>    
    */

    /**
    * A Wrapper class for MatrixWorkspace objects. These objects are actually instantiated
    * in Python. Some method calls are routed through here.
    */
    class MatrixWorkspaceWrapper: public API::MatrixWorkspace
    {
    public:
      /// Constructor
      MatrixWorkspaceWrapper(PyObject *self) : m_self(self)
      {
        Py_INCREF(m_self);
      }
      /// Destructor
      ~MatrixWorkspaceWrapper()
      {
        Py_DECREF(m_self);
      }
      /// Read X values from a spectra in a workspace and return it as a read-only numpy array
      static PyObject * readX(API::MatrixWorkspace& self, int index);
      /// Read Y values from a spectra in a workspace and return it as a read-only numpy array
      static PyObject * readY(API::MatrixWorkspace& self, int index);
      /// Read E values from a spectra in a workspace and return it as a read-only numpy array
      static PyObject * readE(API::MatrixWorkspace& self, int index);

    private:
      /// Stored Python object
      PyObject *m_self;
    };

    /**
     *  A proxy for implementing workspace algebra operator overloads.
     *
     */
    struct WorkspaceAlgebraHelper
    {
      typedef API::MatrixWorkspace wraptype; ///< Wrapper type def
      typedef boost::shared_ptr<wraptype> wraptype_ptr; ///< Shared pointer type def

      /// Binary op for two workspaces
      static wraptype_ptr performBinaryOp(const wraptype_ptr lhs, const wraptype_ptr rhs, 
					  const std::string & op, const std::string & name, bool inplace);

      /// Binary op for a workspace and a double
      static wraptype_ptr performBinaryOp(const wraptype_ptr inputWS, const double value, 
					  const std::string & op, const std::string & name, bool inplace);
    };

    /**
    * A proxy struct for the WorkspaceFactory
    */
    struct WorkspaceFactoryProxy
    {
      /**
      * Create a MatrixWorkspace object that is initialized to the required size
      */
      static API::MatrixWorkspace_sptr createMatrixWorkspace(int nvectors, int xlength, int ylength)
      {
        return API::WorkspaceFactory::Instance().create("Workspace2D", nvectors, xlength, ylength);
      }

      /**
      * Create a matrix workspace that has the same attributes as the workspace given
      */
      static API::MatrixWorkspace_sptr createMatrixWorkspaceFromTemplate(const API::MatrixWorkspace_sptr & original, int nvectors = -1, int xlength = -1, int ylength = -1)
      {
        return API::WorkspaceFactory::Instance().create(original, nvectors, xlength, ylength);
      }

      /**
      * Create a table workspace object
      */
      static API::ITableWorkspace_sptr createTableWorkspace()
      {
        return API::WorkspaceFactory::Instance().createTable();
      }

    };



  }
}

#endif //MANTID_PYTHONAPI_WORKSPACEPROXIES_H_
