#ifndef MANTID_DATAOBJECTS_WORKSPACESINGLEVALUE_H_
#define MANTID_DATAOBJECTS_WORKSPACESINGLEVALUE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid
{

//----------------------------------------------------------------------
// Forward Declaration
//----------------------------------------------------------------------
namespace Kernel
{
  class Logger;
}

namespace DataObjects
{
/** Concrete workspace implementation. Data is a single double value
    @author Nicholas Draper
    @date 19/05/2008

    Copyright &copy; 2007-9 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class DLLExport WorkspaceSingleValue : public API::MatrixWorkspace
{
public:
  /// Typedef for the workspace_iterator to use with a WorkspaceSingleValue
  typedef API::workspace_iterator<API::LocatedDataRef, WorkspaceSingleValue> iterator;
  /// Typedef for the const workspace_iterator to use with a WorkspaceSingleValue
  typedef API::workspace_iterator<const API::LocatedDataRef, const WorkspaceSingleValue> const_iterator;

  /**
  	Gets the name of the workspace type
  	@return Standard string name
  */
  virtual const std::string id() const {return "WorkspaceSingleValue";}

  WorkspaceSingleValue(double value=0.0,double error=0.0);

  virtual ~WorkspaceSingleValue();

  //section required for iteration
  ///Returns the number of single indexable items in the workspace
  virtual std::size_t size() const
  { return 1; }

  ///Returns the size of each block of data returned by the dataX accessors
  virtual std::size_t blocksize() const
  { return 1; }

  std::size_t getNumberHistograms() const
  { return 1; }

  //inheritance redirections
  ///Returns the x data
  virtual MantidVec& dataX(const std::size_t index) { (void) index; return _X; }
  ///Returns the y data
  virtual MantidVec& dataY(const std::size_t index) { (void) index; return _Y; }
  ///Returns the error data
  virtual MantidVec& dataE(const std::size_t index) { (void) index; return _E; }
  ///Returns the x error data
  virtual MantidVec& dataDx(const std::size_t index) { (void) index; return _Dx; }
  /// Returns the x data const
  virtual const MantidVec& dataX(const std::size_t index) const { (void) index; return _X;}
  /// Returns the y data const
  virtual const MantidVec& dataY(const std::size_t index) const { (void) index; return _Y;}
  /// Returns the error const
  virtual const MantidVec& dataE(const std::size_t index) const { (void) index; return _E;}
  /// Returns the x error const
  virtual const MantidVec& dataDx(const std::size_t index) const { (void) index; return _Dx;}
  
  /// Returns a pointer to the x data
  virtual Kernel::cow_ptr<MantidVec> refX(const std::size_t index) const;
  /// Set the specified X array to point to the given existing array
  virtual void setX(const std::size_t index, const Kernel::cow_ptr<MantidVec>& X) { (void) index; _X = *X; }
  /// Set the specified X array to point to the given existing array, with error
  virtual void setX(const int index, const Kernel::cow_ptr<MantidVec>& X,
      const Kernel::cow_ptr<MantidVec>& dX) { (void) index; _X = *X; _Dx = *dX;}

  ///Returns non-const vector of the x data
  virtual MantidVec& dataX() { return _X; }
  ///Returns non-const vector of the y data
  virtual MantidVec& dataY() { return _Y; }
  ///Returns non-const vector of the error data
  virtual MantidVec& dataE() { return _E; }
  ///Returns non-const vector of the x error data
  virtual MantidVec& dataDx() { return _Dx; }
  /// Returns the x data const
  virtual const MantidVec& dataX() const { return _X; }
  /// Returns the y data const
  virtual const MantidVec& dataY() const { return _Y; }
  /// Returns the error data const
  virtual const MantidVec& dataE() const { return _E; }
  /// Returns the x error data const
  virtual const MantidVec& dataDx() const { return _Dx; }

private:
  /// Private copy constructor. NO COPY ALLOWED
  WorkspaceSingleValue(const WorkspaceSingleValue&);
  /// Private copy assignment operator. NO ASSIGNMENT ALLOWED
  WorkspaceSingleValue& operator=(const WorkspaceSingleValue&);

  // allocates space in a new workspace - does nothing in this case
  virtual void init(const std::size_t &NVectors, const std::size_t &XLength, const std::size_t &YLength);

  ///Internal cache of X data
  MantidVec _X;
  ///Internal cache of Y data
  MantidVec _Y;
  ///Internal cache of E data
  MantidVec _E;
  ///Internal cache of x error data
  MantidVec _Dx;

  /// Static reference to the logger class
  static Kernel::Logger &g_log;
};

  ///shared pointer to the WorkspaceSingleValue class
typedef boost::shared_ptr<WorkspaceSingleValue> WorkspaceSingleValue_sptr;
typedef boost::shared_ptr<const WorkspaceSingleValue> WorkspaceSingleValue_const_sptr;

} // namespace DataObjects
} // namespace Mantid

#endif /*MANTID_DATAOBJECTS_WORKSPACESINGLEVALUE_H_*/
