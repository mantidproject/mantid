#ifndef LOADRAW_H_
#define LOADRAW_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "DataHandlingCommand.h"

namespace Mantid
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
    
    Copyright &copy; 2007 ???RAL???

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
  class LoadRaw : public DataHandlingCommand
  {
  public:
    /// Default constructor
    LoadRaw();
    /// Destructor
    ~LoadRaw() {}
    
  private:
    /** Performs the initialisation task of retrieving and setting parameters
     * 
     *  @return A StatusCode object indicating whether the operation was successful
     */
    StatusCode init();
    
    /** Executes the algorithm, reading in the file and creating and populating
     *  the output workspace
     * 
     *  @return A StatusCode object indicating whether the operation was successful
     */
    StatusCode exec();
    
    /// Does nothing at present
    StatusCode final();
    
    /// The name and path of the input file
    std::string m_filename;
    
  };

}

#endif /*LOADRAW_H_*/
