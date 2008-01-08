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

    Load ISIS log file(s). Assumes that a log file originates from a 
    PC (not VMS) environment, i.e. the log file to be loaded is assumed
    to have the extension .txt. Its filename is assumed to starts with the raw data 
    file identifier followed by the character '_', and the .txt itself is assumed to 
    contain two columns, where the first column consists of data-time strings of the 
    form ISO 8601 and the second column consists either of numbers or strings that may 
    contain spaces (e.g. "CHANGE RUNTABLE").

    The algoritm requires an input filename. If this filename is the name of a
    raw datafile the algorithm will attempt to read in all the log files associated
    with that log file. Otherwise it will assume the filename specified is the 
    filename of a specific log file. 
    
    LoadLog is an algorithm and as such inherits from the Algorithm class, 
    via DataHandlingCommand, and overrides the init(), exec() & final() methods.
    
    Required Properties:
       <UL>
       <LI> Filename - The full name of and path of the input ISIS log file </LI>
       <LI> OutputWorkspace - A name specified for the output storage space </LI>
       </UL>

    @author Anders Markvardsen, ISIS, RAL
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
  class DLLExport LoadLog : public DataHandlingCommand
  {
  public:
    /// Default constructor
    LoadLog();

    /// Destructor
    ~LoadLog() {}
    
  private:

    /// Overwrites Algorithm method.
    void init();
    
    /// Overwrites Algorithm method
    void exec();
    
    /// Overwrites Algorithm method. Does nothing at present
    void final();
    
    /// The name and path of an input file. This may be the filename of a
    /// raw datafile or the name of a specific log file. 
    std::string m_filename;
    
    /// type returned by classify
    enum kind { empty, string, number };

    /// Takes as input a string and try to determine what type it is
    kind classify(const std::string& s);

    /// convert string to lower case
    std::string stringToLower(std::string strToConvert);

    /// look at whether filename has the .txt extension and contain a '_'
    bool isLogFile(const std::string& filenamePart);

    /// check if first 19 characters of a string is data-time string according to yyyy-mm-ddThh:mm:ss
    bool isDateTimeString(const std::string& str);

    /// static reference to the logger class
    static Kernel::Logger& g_log;
  };

} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADLOG_H_*/
