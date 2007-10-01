/* @class LoadRaw LoadRaw.h DataHandling/LoadRaw.h

    Loads an file in ISIS RAW format.

    @author Russell Taylor, Tessella Support Services plc
    @date 26/09/2007
    
    Copyright © 2007 ???RAL???

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
#include "../inc/LoadRaw.h"

extern "C" void open_file__(const char* fname, int* found, unsigned fname_len);
extern "C" void getpari_(const char* fname, const char* item, int* val, 
    int* len_in, int* len_out, int* errcode, unsigned len_fname, unsigned len_item);
extern "C" void getdat_(const char* fname, const int& spec_no, const int& nspec, 
    int* idata, int& length, int& errcode, unsigned len_fname);
extern "C" void close_data_file__();

namespace Mantid
{
  // Constructor
  LoadRaw::LoadRaw()
  {
  }

  StatusCode LoadRaw::init()
  {
    MsgStream log(0,"");

    StatusCode status = getProperty("Filename", m_filename);
    // Check that property has been set and retrieved successfully
    if ( status.isFailure() )
    {     
      log << "Filename property has not been set." << endreq;
      return status;
    }
    
    return StatusCode::SUCCESS;
  }
  
  StatusCode LoadRaw::exec()
  {
    MsgStream log(0,"");
    
    int found = 0;  
    // Call the FORTRAN function to open the RAW file
    open_file__( m_filename.c_str(), &found, strlen( m_filename.c_str() ) );
    if ( ! found )
    {
      // Unable to open file
      log << "Unable to open file " << m_filename << endreq;
      return StatusCode::FAILURE;
    }
    
    int channelsPerSpectrum, lengthIn, lengthOut, errorCode;
    lengthIn = lengthOut = 1;
    getpari_(m_filename.c_str(), "NTC1", &channelsPerSpectrum, &lengthIn, &lengthOut,
       &errorCode, strlen( m_filename.c_str() ), strlen("NTC1"));
    if (errorCode) return StatusCode::FAILURE;

    int numberOfSpectra;
    getpari_(m_filename.c_str(), "NSP1", &numberOfSpectra, &lengthIn, &lengthOut,
       &errorCode, strlen( m_filename.c_str() ), strlen("NSP1"));
    if (errorCode) return StatusCode::FAILURE;
    
    lengthIn = channelsPerSpectrum + 1;
    
    int* iData = new int[lengthIn * numberOfSpectra];
    int* iLocation = iData;     // Position pointer for stepping through memory
    
    for (int i = 1; i < numberOfSpectra; i++)
    {
      getdat_(m_filename.c_str(), i, 1, iLocation, lengthIn, errorCode, strlen( m_filename.c_str() ));
      iLocation += lengthIn;
    }
    
    // Close the input data file
    close_data_file__();
    
    delete[] iData;
    return StatusCode::SUCCESS;
  }
  
  StatusCode LoadRaw::final()
  {
    return StatusCode::SUCCESS;
  }
  
}
