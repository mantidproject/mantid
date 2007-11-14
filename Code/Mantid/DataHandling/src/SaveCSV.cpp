/* @class SaveCSV SaveCSV.h DataHandling/SaveCSV.h

    @author Anders J. Markvardsen, ISIS, RAL
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

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "../inc/SaveCSV.h"
#include "../../DataObjects/inc/Workspace1D.h"

#include <fstream>  // used to get ofstream
#include <iomanip>  // setw() used below

DECLARE_NAMESPACED_ALGORITHM(Mantid::DataHandling, SaveCSV)

namespace Mantid
{
namespace DataHandling
{

  using namespace Kernel;
  using DataObjects::Workspace1D;

  Logger& SaveCSV::g_log = Logger::get("SaveCSV");


  /// Empty default constructor
  SaveCSV::SaveCSV() {}


  /** Initialisation method. Does nothing at present.
   * 
   *  @return A StatusCode object indicating whether the operation was successful
   */
  StatusCode SaveCSV::init()
  {	
    return StatusCode::SUCCESS;
  }
  
  
  /** Executes the algorithm. Retrieve the Filename, Seperator and LineSeperator
   *  properties and save 1D workspace to Filename. 
   * 
   *  @return A StatusCode object indicating whether the operation was successful
   */
  StatusCode SaveCSV::exec()
  {
    // Gets the name of the file to save the 1D workspace to; and the
    // Seperator and LineSeperator properties if they are provided by the user.

    // Retrieve the filename from the properties

    StatusCode status = getProperty("Filename", m_filename);


    // Check that this property has been set and retrieved successfully

    if ( status.isFailure() )
    {     
      g_log.error("Filename property has not been set.");
      return status;
    }
    
    
    // Check if the user has specified the optional Seperator property 
    
    status = getProperty("Seperator", m_seperator);


    // If Seperator not specified then use default Seperator

    if ( status.isFailure() )
    {     
      m_seperator = ",";
    }    
        

    // Check if the user has specified the optional LineSeperator property 
    
    status = getProperty("LineSeperator", m_lineSeperator);


    // If LineSeperator not specified then use default LineSeperator

    if ( status.isFailure() )
    {     
      m_lineSeperator = "\n";
    } 


    const Workspace1D *localworkspace = dynamic_cast<Workspace1D*>(m_inputWorkspace);


    // Get info from 1D workspace

    const std::vector<double>& xValue = localworkspace->getX();
    const std::vector<double>& yValue = localworkspace->getY();
    const std::vector<double>& eValue = localworkspace->getE();

    
    // prepare to save to file
    
    std::ofstream outCSV_File(m_filename.c_str());
  
  
    if (!outCSV_File)
    {
      g_log.error("Failed to open file:" + m_filename);
      return StatusCode::FAILURE;
    }
    
    
    // write to file
    
    for (int i = 0; i < (int)xValue.size(); i++)
    {
      outCSV_File << std::setw(15) << xValue[i] << m_seperator << std::setw(15) << yValue[i] 
        << m_seperator << std::setw(15) << eValue[i] << m_lineSeperator;
    }    
    
    
    outCSV_File.close();
    

    return StatusCode::SUCCESS;
  }

  
  /** Finalisation method. Does nothing at present.
   *
   *  @return A StatusCode object indicating whether the operation was successful
   */  
  StatusCode SaveCSV::final()
  {
    return StatusCode::SUCCESS;
  }
  
} // namespace DataHandling
} // namespace Mantid
