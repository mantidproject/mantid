#ifndef MANTID_DATAHANDLING_LOADRAW_H_
#define MANTID_DATAHANDLING_LOADRAW_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "DataHandlingCommand.h"
#include "../../Kernel/inc/Logger.h"

namespace Mantid
{
namespace DataHandling
{
/** @class LoadRaw LoadRaw.h DataHandling/LoadRaw.h

    Loads an file in ISIS RAW format and stores it in a 2D workspace 
    (Workspace2D class). LoadRaw is an algorithm and as such inherits
    from the Algorithm class, via DataHandlingCommand, and overrides
    the init(), exec() & final() methods.
    
    Required Properties:
       <UL>
       <LI> Filename - The name of and path to the input RAW file </LI>
       <LI> OutputWorkspace - The name of the workspace in which to store the imported data </LI>
       </UL>

    @author Russell Taylor, Tessella Support Services plc
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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>    
*/
  class DLLExport LoadRaw : public DataHandlingCommand
  {
  public:
    /// Default constructor
    LoadRaw();

    /// Destructor
    ~LoadRaw() {}
    
  private:

    /// Overwrites Algorithm method. Does nothing at present
    Kernel::StatusCode init();
    
    /// Overwrites Algorithm method
    Kernel::StatusCode exec();
    
    /// Overwrites Algorithm method. Does nothing at present
    Kernel::StatusCode final();
    
    /// The name and path of the input file
    std::string m_filename;
    
    ///static reference to the logger class
    static Kernel::Logger& g_log;

    /// Personal wrapper for sqrt to allow msvs to compile
    static double dblSqrt(double in);
  };

} // namespace DataHandling
} // namespace Mantid

#endif /*MANTID_DATAHANDLING_LOADRAW_H_*/
