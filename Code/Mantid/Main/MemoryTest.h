#ifndef MANTID_MAIN_MEMORYTEST_H_
#define MANTID_MAIN_MEMORYTEST_H_

//-----------------------------------
//Includes
//-----------------------------------
#include "MantidKernel/Logger.h"

/** 
    A class to demonstrate that in Linux there is not actually memory leak when 
    it appears that memory is not released to the operating system. See 
    <http://www.mantidproject.org/Main_Page> for more information.

    @author Martyn Gigg, Tessella Support Services plc
    @date 21/11/2008

    Copyright &copy; 2008 STFC Rutherford Appleton Laboratories

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
    Code Documentation is available at: <http://doxygen.mantidproject.org>    
*/
class MemoryTest
{
public: 
  //Compiler generated constructors are fine
  
  ///Actually run the memory tests
  void runMemoryTests() const;

private:
  
  // A static reference to the logger
  static Mantid::Kernel::Logger& g_log;
};


#endif //MANTID_MAIN_MEMORYTEST_H_

