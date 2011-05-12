#ifndef LOAD_MD_WORKSPACE_H
#define LOAD_MD_WORKSPACE_H


#include "MantidGeometry/MDGeometry/MDGeometryDescription.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MDDataObjects/MDWorkspace.h"
#include "MDDataObjects/MD_FileFormatFactory.h"

    /** Algorithm loads main part of existing multidimensional workspace and initate workspace for future operations

    
        Another prospective feature would be initate a workspace from a swap file (probably not by this algorithm)


        @author  Alex Buts,  ISIS RAL 
        @date 23/12/2010

        Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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



namespace Mantid
{
namespace MDAlgorithms
{

class DLLExport Load_MDWorkspace: public API::Algorithm
{
    
public:

    Load_MDWorkspace(void);

    virtual ~Load_MDWorkspace(void);

  /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "LoadMDworkspace";}
      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return 1;}
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "MD-Algorithms";}
     /** The function sets load algorithm into test mode, namely it conects the algorithm to the test file reader,
	     which provides test workspace (generated and not from a data file)* The test mode is dropped immidiately after
		 the algorithm has been executed, so if you need a test mode for next load operation, you should set it again */
	  void set_test_mode(){test_mode=true;}
private:
    // Overridden Algorithm methods
      void init();
      void exec();

	  bool test_mode;

protected:
   /// logger -> to provide logging, for MD dataset file operations
    static Mantid::Kernel::Logger& ldmdws_log;
};
} // end namespaces
}

#endif
