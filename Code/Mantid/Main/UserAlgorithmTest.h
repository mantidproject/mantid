#ifndef MANTID_MAIN_USERALGORITHMTEST_H_
#define MANTID_MAIN_USERALGORITHMTEST_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace1D.h" 
#include "MantidDataObjects/ManagedWorkspace2D.h" 

/** @class UserAlgorithmTest UserAlgorithmTest.h

UserAlgorithmTest is a class that performs tests of
the user algorithm examples

@author Nick Draper, Tessella Support Services plc
@date 16/01/2008

Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class UserAlgorithmTest 
{
public:
  /// No-arg Constructor
  UserAlgorithmTest() { }

  void RunPropertyAlgorithmTest();
  void RunWorkspaceAlgorithmTest();
  void RunModifyDataTest();
  void RunAllTests();

  static Mantid::DataObjects::Workspace1D_sptr Create1DWorkspace(int size);
  static Mantid::DataObjects::Workspace2D_sptr Create2DWorkspace(int xlen, int ylen);


};


#endif /*MANTID_MAIN_USERALGORITHMTEST_H_*/

