#ifndef SAVECSV_H_
#define SAVECSV_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "DataHandlingCommand.h"
#include "../../Kernel/inc/Logger.h"

namespace Mantid
{
/** @class SaveCSV SaveCSV.h DataHandling/SaveCSV.h

    Saves a 1D workspace to a CSV file. SaveCSV is an algorithm and as such 
    inherits from the Algorithm class, via DataHandlingCommand, and overrides
    the init(), exec() & final() methods.
    
    Required Properties:
       <UL>
       <LI> Filename - The name of file to store the 1D workspace to </LI>
       <LI> InputWorkspace - The name of a 1D workspace </LI>
       </UL>

    Optional Properties:
       <UL>
       <LI> Seperator - defaults to "," </LI>
       <LI> LineSeperator - defaults to "\n" </LI>
       </UL>

    @author Anders Markvardsen, ISIS, RAL
    @date 15/10/2007
    
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
  class SaveCSV : public DataHandlingCommand
  {
  public:
    /// Default constructor
    SaveCSV();
    
    /// Destructor
    ~SaveCSV() {}
    
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
    
    /// The name of the file used for storing the workspace
    std::string m_filename;
    
    /// The seperator for the CSV file
    std::string m_seperator;
    
    /// The line seperator for the CSV file
    std::string m_lineSeperator;   

	///static reference to the logger class
	static Logger& g_log;
  };

}

#endif /*SAVECSV_H_*/
