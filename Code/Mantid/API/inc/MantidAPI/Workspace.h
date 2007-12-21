#ifndef MANTID_KERNEL_WORKSPACE_H_
#define MANTID_KERNEL_WORKSPACE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/Instrument.h"
#include "MantidAPI/Sample.h"
#include "MantidKernel/Logger.h"
#include <string>
#include <ostream> 

namespace Mantid
{
namespace API
{
/** @class Workspace Workspace.h
 	
 	  Base Workspace Abstract Class
 		Not static method create() since this base 
 		object will not be registered with the factory.
		Requirement: get some kind of support for memmory 
		footprint of the data object.
 			    	
    @author Laurent C Chapon, ISIS, RAL
    @date 26/09/2007
 	    
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
 	
    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport Workspace
{
public:
  /// Return the workspace typeID 
	virtual const std::string id() const = 0;

	void setTitle(const std::string&);
	void setComment(const std::string&);

	const std::string& getComment() const;
	const std::string& getTitle() const;

	Instrument& getInstrument() ;
	Sample& getSample();  

        
  //section required for iteration
  ///Returns the number of single indexable items in the workspace
  virtual int size() const = 0;
  ///Returns the size of each block of data returned by the dataX accessors
  virtual int blocksize() const  = 0;
  ///Returns the x data
  virtual std::vector<double>& dataX(int const index) =0;
  ///Returns the y data
  virtual std::vector<double>& dataY(int const index)  =0;
  ///Returns the error data
  virtual std::vector<double>& dataE(int const index)  =0;  
	/// Get the footprint in memory.
	virtual long int getMemorySize() const {return 0;}	
	virtual ~Workspace();

  //Get methods return the histogram number 
  virtual const std::vector<double>& dataX(int const index) const =0;
  virtual const std::vector<double>& dataY(int const index) const =0;
  virtual const std::vector<double>& dataE(int const index) const =0;


protected:
	Workspace();
	Workspace(const Workspace&);
	Workspace& operator=(const Workspace&);

private:

	/// The title of the workspace
	std::string _title;
	/// A user-provided comment that is attached to the workspace
	std::string _comment;

	/// The instrument used for this experiment
	Instrument _instrument;
	/// The information on the sample environment
	Sample _sample;
  
	/// Static reference to the logger class
	static Kernel::Logger& g_log;
};

} // namespace API
} //Namespace Mantid
#endif /*MANTID_KERNEL_WORKSPACE_H_*/
