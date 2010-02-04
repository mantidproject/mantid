#ifndef MANTID_PYTHONAPI_WORKSPACEPROXIES_H_
#define MANTID_PYTHONAPI_WORKSPACEPROXIES_H_

//-----------------------------------
// Includes
//-----------------------------------
#include <MantidAPI/AnalysisDataService.h>
#include <MantidAPI/MatrixWorkspace.h>
#include <MantidAPI/ITableWorkspace.h>
#include <MantidAPI/WorkspaceFactory.h>
#include <boost/python.hpp>

namespace Mantid
{

namespace PythonAPI
{

/** 
    Various structures to aid with Python's interaction with Mantid workspaces

    @author ISIS, STFC
    @date 28/02/2008

    Copyright &copy; 2007 STFC Rutherford Appleton Laboratories

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
  //@cond NODOC
  /**
   * A callback class for MatrixWorkspace objects so that their setX methods are proxied through here.
   */
  class MatrixWorkspaceCallback : public API::MatrixWorkspace
  {
  public:
    ///Default constructor
    MatrixWorkspaceCallback(PyObject *self) : m_self(self)
    {
      Py_INCREF(m_self);
    }
    /// Destructor
    ~MatrixWorkspaceCallback()
    {
      Py_DECREF(m_self);
    }

  private:
    /// Stored Python object
    PyObject *m_self;
  };

  /**
   *  A proxy for implementing workspace algebra operator overloads
   */
  struct WorkspaceAlgebraProxy
  {
    typedef API::MatrixWorkspace wraptype;
    typedef boost::shared_ptr<wraptype> wraptype_ptr;
    
    /** 
     * Perform the given binary operation on two workspaces
     * @param lhs The left-hand side of the operation
     * @param rhs The right-hand side of the operation
     * @param op One of 'p', 'm', 't', 'd' to denote the required operation
     * @param inplace If true, then the lhs argument is replaced by the result of the operation.
     */
    static wraptype_ptr performBinaryOp(const wraptype_ptr lhs, const wraptype_ptr rhs, char op, bool inplace);

    //Plus workspace
    static wraptype_ptr plus(const wraptype_ptr lhs, const wraptype_ptr rhs)
    {
      return performBinaryOp(lhs, rhs, 'p', false);
    }
    /// Inplace Plus workspace
    static wraptype_ptr inplace_plus(const wraptype_ptr lhs, const wraptype_ptr rhs)
    {
      return performBinaryOp(lhs, rhs, 'p', true);
    }
    /// Minus workspace
    static wraptype_ptr minus(const wraptype_ptr lhs, const wraptype_ptr rhs)
    {
      return performBinaryOp(lhs, rhs, 'm', false);
    }
    /// Inplace Minus workspace
    static wraptype_ptr inplace_minus(const wraptype_ptr lhs, const wraptype_ptr rhs)
    {
      return performBinaryOp(lhs, rhs, 'm', true);
    }
    /// Multiply workspace
    static wraptype_ptr times(const wraptype_ptr lhs, const wraptype_ptr rhs)
    {
      return performBinaryOp(lhs, rhs, 't', false);
    }
    /// Inplace Multiply workspace
    static wraptype_ptr inplace_times(const wraptype_ptr lhs, const wraptype_ptr rhs)
    {
      return performBinaryOp(lhs, rhs, 't', true);
    }
    /// Divide workspace
    static wraptype_ptr divide(const wraptype_ptr lhs, const wraptype_ptr rhs)
    {
      return performBinaryOp(lhs, rhs, 'd', false);
    }
    /// Divide workspace
    static wraptype_ptr inplace_divide(const wraptype_ptr lhs, const wraptype_ptr rhs)
    {
      return performBinaryOp(lhs, rhs, 'd', true);
    }

    /** 
    * Perform the given binary operation on a workspace and a double
    * @param lhs The left-hand side of the operation
    * @param rhs The right-hand side of the operation
    * @param op One of 'p', 'm', 't', 'd' to denote the required operation
    * @param inplace If true, then the lhs argument is replaced by the result of the operation.
    */
    static wraptype_ptr performBinaryOp(const wraptype_ptr lhs, double rhs, char op, bool inplace);

    /** 
    * Perform the given binary operation on a double and a workspace
    * @param lhs The left-hand side of the operation
    * @param rhs The right-hand side of the operation
    * @param op One of 'p', 'm', 't', 'd' to denote the required operation
    * @param inplace If true, then the lhs argument is replaced by the result of the operation.
    */ 
    static wraptype_ptr performBinaryOp(double lhs, const wraptype_ptr rhs, char op);
    /// Plus
    static wraptype_ptr plus(const wraptype_ptr lhs, double rhs)
    {
      return performBinaryOp(lhs, rhs, 'p', false);
    }
    /// Inplace Plus
    static wraptype_ptr inplace_plus(const wraptype_ptr lhs, double rhs)
    {
      return performBinaryOp(lhs, rhs, 'p', true);
    }
    /// Reverse Plus
    static wraptype_ptr rplus(const wraptype_ptr rhs, double lhs)
    {
      return performBinaryOp(lhs, rhs, 'p');
    }
    /// Minus
    static wraptype_ptr minus(const wraptype_ptr lhs, double rhs)
    {
      return performBinaryOp(lhs, rhs, 'm', false);
    }
    /// Inplace Minus
    static wraptype_ptr inplace_minus(const wraptype_ptr lhs, double rhs)
    {
      return performBinaryOp(lhs, rhs, 'm', true);
    }
    /// Reverse Minus
    static wraptype_ptr rminus(const wraptype_ptr rhs, double lhs)
    {
      return performBinaryOp(lhs, rhs, 'm');
    }
    /// Multiply
    static wraptype_ptr times(const wraptype_ptr lhs, double rhs)
    {
      return performBinaryOp(lhs, rhs, 't', false);
    }
    /// Inplace Multiply
    static wraptype_ptr inplace_times(const wraptype_ptr lhs, double rhs)
    {
      return performBinaryOp(lhs, rhs, 't', true);
    }
    /// Multiply
    static wraptype_ptr rtimes(const wraptype_ptr rhs, double lhs)
    {
      return performBinaryOp(lhs, rhs, 't');
    }
    /// Divide
    static wraptype_ptr divide(const wraptype_ptr lhs, double rhs)
    {
      return performBinaryOp(lhs, rhs, 'd', false);
    }
    /// Reverse Divide
    static wraptype_ptr rdivide(const wraptype_ptr rhs, double lhs)
    {
      return performBinaryOp(lhs, rhs, 'd');
    }
    /// Inplace Divide
    static wraptype_ptr inplace_divide(const wraptype_ptr lhs, double rhs)
    {
      return performBinaryOp(lhs, rhs, 'd', true);
    }

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

  //@endcond

}
}

#endif //MANTID_PYTHONAPI_WORKSPACEPROXIES_H_
