#ifndef MANTID_DATAHANDLING_LOADREFLTBL_H_
#define MANTID_DATAHANDLING_LOADREFLTBL_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFileLoader.h"
#include "MantidDataObjects/TableWorkspace.h"

namespace Mantid
{
  namespace DataHandling
  {
    /**
    Loads a table workspace from an ascii file in reflectometry tbl format. Rows must be no longer than 17 cells.

    Properties:
    <ul>
    <li>Filename  - the name of the file to read from.</li>
    <li>Workspace - the workspace name that will be created and hold the loaded data.</li>
    </ul>

    @author Keith Brown, ISIS, Placement student from the University of Derby
    @date 27/03/14

    Copyright &copy; 2007-2014 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://github.com/mantidproject/mantid>. 
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class DLLExport LoadReflTBL :public API::IFileLoader<Kernel::FileDescriptor>
    {
    public:
      /// Default constructor
      LoadReflTBL();
      /// The name of the algorithm
      virtual const std::string name() const { return "LoadReflTBL"; }
      /// The version number
      virtual int version() const { return 1; }
      /// The category
      virtual const std::string category() const { return "DataHandling\\Text"; }
      /// Returns a confidence value that this algorithm can load a file
      virtual int confidence(Kernel::FileDescriptor & descriptor) const;

    private:
      /// Sets documentation strings for this algorithm
      virtual void initDocs();
      /// Declare properties
      void init();
      /// Execute the algorithm
      void exec();
    };

  } // namespace DataHandling
} // namespace Mantid

#endif  /*  MANTID_DATAHANDLING_LOADREFLTBL_H_  */
