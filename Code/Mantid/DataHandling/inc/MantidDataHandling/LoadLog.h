#ifndef MANTID_DATAHANDLING_LOADLOG_H_
#define MANTID_DATAHANDLING_LOADLOG_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/DataHandlingCommand.h"
#include "MantidKernel/Logger.h"

namespace Mantid
{
namespace DataHandling
{
/** @class LoadLog LoadLog.h DataHandling/LoadLog.h

    Loads an ISIS log file. Assumes this log file originates from a 
    PC (not VMS) environment, i.e. the log file to be loaded is assumed
    to have the extension .txt. Its filename is assumed to starts with the raw data 
    file identifier and the .txt itself is assumed to contain two columns, where 
    the first column consists of data-time strings of the form ISO 8601 
    and the second column consists either of numbers (integers or floating)
    or strings that may contain spaces (e.g. "CHANGE RUNTABLE").
    
    LoadLog is an algorithm and as such inherits from the Algorithm class, 
    via DataHandlingCommand, and overrides the init(), exec() & final() methods.
    
    Required Properties:
       <UL>
       <LI> Filename - The full name of and path of the input ISIS log file </LI>
       </UL>

    @author Anders Markvardsen, ISIS, RAL
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
  class DLLExport LoadLog : public DataHandlingCommand
  {
  public:
    /// Default constructor
    LoadLog();

    /// Destructor
    ~LoadLog() {}
    
  private:

    /// Overwrites Algorithm method.
    Kernel::StatusCode init();
    
    /// Overwrites Algorithm method
    Kernel::StatusCode exec();
    
    /// Overwrites Algorithm method. Does nothing at present
    Kernel::StatusCode final();
    
    /// The name and path of an input file. This may be the filename of a
    /// raw datafile or the name of a specific log file. 
    std::string m_filename;
    
    /// type returned by classify
    enum kind { empty, string, number };

    /// Takes as input a string and try to determine what type it is
    kind classify(const std::string& s);

    std::string stringToLower(std::string strToConvert);

    bool isLogFile(const std::string& filenamePart);
    /// Read a single log file into a Sample object
 //   void addToSample(API::Sample& sample, std::string &filename);

    /// static reference to the logger class
    static Kernel::Logger& g_log;
  };

} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADLOG_H_*/
