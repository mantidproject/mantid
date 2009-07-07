#ifndef MANTID_DATAHANDLING_LOADASCII_H_
#define MANTID_DATAHANDLING_LOADASCII_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"

//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------

namespace Mantid
{
  namespace DataHandling
  {
    /** @class LoadAscii LoadAscii.h DataHandling/LoadAscii.h

    Loads a workspace from a coma-separated ascii file. Spectra must be stored in columns.
    Properties:
    <ul>
          <li>Filename - the name of the file to read from.  <li/>
          <li>Workspace - the workspace name that will be created and hold the loaded data.<li/>
    <ul/>

    @author Roman Tolchenov, Tessella plc
    @date 3/07/09

    Copyright &copy; 2007-9 STFC Rutherford Appleton Laboratory

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
    class DLLExport LoadAscii : public API::Algorithm
    {
    public:
      /// Destructor
      ~LoadAscii() {}
      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "LoadAscii"; }
      /// Algorithm's version for identification overriding a virtual method
      virtual const int version() const { return 1; }
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "DataHandling"; }

    private:
      /// Overwrites Algorithm method.
      void init();
      /// Overwrites Algorithm method
      void exec();
      ///static reference to the logger class
      static Kernel::Logger& g_log;
    };

  } // namespace DataHandling
} // namespace Mantid

#endif  /*  MANTID_DATAHANDLING_LOADASCII_H_  */
