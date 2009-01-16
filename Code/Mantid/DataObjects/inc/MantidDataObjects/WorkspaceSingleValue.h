#ifndef MANTID_DATAOBJECTS_WORKSPACESINGLEVALUE_H_
#define MANTID_DATAOBJECTS_WORKSPACESINGLEVALUE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/IErrorHelper.h"

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

    Copyright &copy; 2007-8 STFC Rutherford Appleton Laboratory

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
  	\return Standard string name
  */
  virtual const std::string id() const {return "WorkspaceSingleValue";}

  WorkspaceSingleValue(double value=0.0,double error=0.0);

  virtual ~WorkspaceSingleValue();

  //section required for iteration
  ///Returns the number of single indexable items in the workspace
  virtual int size() const
  { return 1; }

  ///Returns the size of each block of data returned by the dataX accessors
  virtual int blocksize() const
  { return 1; }

  const int getNumberHistograms() const
  { return 1; }

  //inheritance redirections
  ///Returns the x data
  virtual std::vector<double>& dataX(int const index) { return _X; }
  ///Returns the y data
  virtual std::vector<double>& dataY(int const index) { return _Y; }
  ///Returns the error data
  virtual std::vector<double>& dataE(int const index) { return _E; }
  /// Returns the x data const
  virtual const std::vector<double>& dataX(int const index) const {return _X;}
  /// Returns the y data const
  virtual const std::vector<double>& dataY(int const index) const {return _Y;}
  /// Returns the error const
  virtual const std::vector<double>& dataE(int const index) const {return _E;}

  ///Returns non-const vector of the x data
  virtual std::vector<double>& dataX() { return _X; }
  ///Returns non-const vector of the y data
  virtual std::vector<double>& dataY() { return _Y; }
  ///Returns non-const vector of the error data
  virtual std::vector<double>& dataE() { return _E; }
  /// Returns the x data const
  virtual const std::vector<double>& dataX() const { return _X; }
  /// Returns the y data const
  virtual const std::vector<double>& dataY() const { return _Y; }
  /// Returns the error data const
  virtual const std::vector<double>& dataE() const { return _E; }

  ///Returns the ErrorHelper applicable for this detector
  virtual const API::IErrorHelper* errorHelper(int const index) const { return _ErrorHelper; }
  ///Returns the ErrorHelper applicable for this detector
  virtual const API::IErrorHelper* errorHelper() const { return _ErrorHelper; }
  ///Sets the ErrorHelper for this spectra
  virtual void setErrorHelper(int const index,API::IErrorHelper* errorHelper) { _ErrorHelper=errorHelper; }
   ///Sets the ErrorHelper for this spectra
  virtual void setErrorHelper(int const index,const API::IErrorHelper* errorHelper) { _ErrorHelper=const_cast<API::IErrorHelper*>(errorHelper); }
  ///Sets the ErrorHelper for this spectra
  virtual void setErrorHelper(API::IErrorHelper* errorHelper) { _ErrorHelper=errorHelper; }
   ///Sets the ErrorHelper for this spectra
  virtual void setErrorHelper(const API::IErrorHelper* errorHelper) { _ErrorHelper=const_cast<API::IErrorHelper*>(errorHelper); }

private:
  /// Private copy constructor. NO COPY ALLOWED
  WorkspaceSingleValue(const WorkspaceSingleValue&);
  /// Private copy assignment operator. NO ASSIGNMENT ALLOWED
  WorkspaceSingleValue& operator=(const WorkspaceSingleValue&);

  // allocates space in a new workspace - does nothing in this case
  virtual void init(const int &NVectors, const int &XLength, const int &YLength);

  ///Internal cache of X data
  std::vector<double> _X;
  ///Internal cache of Y data
  std::vector<double> _Y;
  ///Internal cache of E data
  std::vector<double> _E;

  ///Internal cache of applicable errorhelper
  API::IErrorHelper* _ErrorHelper;

  /// Static reference to the logger class
  static Kernel::Logger &g_log;
};

  ///shared pointer to the WorkspaceSingleValue class
  typedef boost::shared_ptr<WorkspaceSingleValue> WorkspaceSingleValue_sptr;

} // namespace DataObjects
} // namespace Mantid

#endif /*MANTID_DATAOBJECTS_WORKSPACESINGLEVALUE_H_*/
