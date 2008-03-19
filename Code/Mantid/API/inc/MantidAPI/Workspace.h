#ifndef MANTID_KERNEL_WORKSPACE_H_
#define MANTID_KERNEL_WORKSPACE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidAPI/Instrument.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/WorkspaceIterator.h"
#include "MantidAPI/IErrorHelper.h"
#include "MantidAPI/GaussianErrorHelper.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/Logger.h"
#include "boost/shared_ptr.hpp"
#include <string>
#include <ostream> 

namespace Mantid
{
namespace API
{
///shared pointer to the workspace base class
typedef boost::shared_ptr<Workspace> Workspace_sptr;

/** @class Workspace Workspace.h
 
 Base Workspace Abstract Class.
 Not static method create() since this base 
 object will not be registered with the factory.
 Requirement: get some kind of support for memmory 
 footprint of the data object.
 
 @author Laurent C Chapon, ISIS, RAL
 @date 26/09/2007
 
 Copyright &copy; 2007-8 STFC Rutherford Appleton Laboratories
 
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
class DLLExport Workspace
{
public:
  /// Typedef for the workspace_iterator to use with a Workspace
  typedef workspace_iterator<LocatedDataRef, Workspace> iterator;
  /// Typedef for the const workspace_iterator to use with a Workspace
  typedef workspace_iterator<const LocatedDataRef, const Workspace> const_iterator;

  /// Return the workspace typeID 
  virtual const std::string id() const = 0;

  /// Initialises the workspace. sets the size and lengths in the arrays, must be overloaded
  virtual void init(const int &NVectors, const int &XLength, const int &YLength) = 0;
  
  void setTitle(const std::string&);
  void setComment(const std::string&);

  const std::string& getComment() const;
  const std::string& getTitle() const;

  Instrument& getInstrument();
  Sample& getSample();

	/// Get the footprint in memory.
	virtual long int getMemorySize() const {return 0;}	
	virtual ~Workspace();

        
  //section required for iteration
  ///Returns the number of single indexable items in the workspace
  virtual int size() const = 0;
  ///Returns the size of each block of data returned by the dataX accessors
  virtual int blocksize() const = 0;
  ///Returns the x data
  virtual std::vector<double>& dataX(int const index) = 0;
  ///Returns the y data
  virtual std::vector<double>& dataY(int const index) =0;
  ///Returns the error data
  virtual std::vector<double>& dataE(int const index)  =0;  
  ///Returns the error data
  virtual std::vector<double>& dataE2(int const index)  =0;

  ///Returns the ErrorHelper applicable for this spectra
  virtual const IErrorHelper* errorHelper(int const index)  const=0;
  ///Sets the ErrorHelper for this spectra
  virtual void setErrorHelper(int const index,API::IErrorHelper* errorHelper) =0;
  ///Sets the ErrorHelper for this spectra
  virtual void setErrorHelper(int const index,const API::IErrorHelper* errorHelper) =0;

  ///Returns the spectra number
  virtual int spectraNo(int const index) const =0;
  ///Returns the spectra number (non const)
  virtual int& spectraNo(int const index) =0;

  //Get methods return the histogram number 
  ///Returns the x data const
  virtual const std::vector<double>& dataX(int const index) const = 0;
  ///Returns the y data const
  virtual const std::vector<double>& dataY(int const index) const =0;
  ///Returns the error const
  virtual const std::vector<double>& dataE(int const index) const =0;
  ///Returns the error const
  virtual const std::vector<double>& dataE2(int const index) const =0;

  ///Returns a reference to the WorkspaceHistory
  WorkspaceHistory& getWorkspaceHistory() { return m_history; }
  ///Returns a reference to the WorkspaceHistory const
  const WorkspaceHistory& getWorkspaceHistory() const { return m_history; }

  /// Returns the unit object
  const boost::shared_ptr<Kernel::Unit>& XUnit() const;
  boost::shared_ptr<Kernel::Unit>& XUnit();
  
  /// Are the Y-values dimensioned?
  const bool& isDistribution() const;
  bool& isDistribution(bool newValue);
  
protected:
  Workspace();

private:
  /// Private copy constructor. NO COPY ALLOWED
  Workspace(const Workspace&);
  /// Private copy assignment operator. NO ASSIGNMENT ALLOWED
  Workspace& operator=(const Workspace&);
  
  /// The title of the workspace
  std::string m_title;
  /// A user-provided comment that is attached to the workspace
  std::string m_comment;

  /// The instrument used for this experiment
  Instrument m_instrument;
  /// The information on the sample environment
  Sample m_sample;

  /// The history of the workspace, algorithm and environment
  WorkspaceHistory m_history;
  
  /// The unit of the x axis
  boost::shared_ptr<Kernel::Unit> m_xUnit;
  /// Flag indicating whether the Y-values are dimensioned? False by default
  bool m_isDistribution;
  
	/// Static reference to the logger class
	static Kernel::Logger& g_log;
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_KERNEL_WORKSPACE_H_*/
