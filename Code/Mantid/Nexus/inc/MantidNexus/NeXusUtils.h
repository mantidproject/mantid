#ifndef NEXUSUTILS_H
#define NEXUSUTILS_H
#include <napi.h>
#include "MantidDataObjects/Workspace2D.h"
namespace Mantid
{
  namespace NeXus
  {
    /** @class NeXusUtils NeXusUtils.h NeXus/NeXusUtils.h

    Utility method for saving NeXus format of Mantid Workspace

    Copyright &copy; 2007-8 STFC Rutherford Appleton Laboratory

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
    class DLLExport NeXusUtils
    {
    public:
      /// Default constructor
      NeXusUtils();

      /// Destructor
      ~NeXusUtils() {}

      /// obsolete function to write a simple 1D workspace to a Nexus file
      int writeEntry1D(const std::string& filename, const std::string& entryName, const std::string& dataName,
				 const std::vector<double>& x, const std::vector<double>& y, const std::vector<double>& e);
      /// search for the data item "dataName" at the top level of the named nexus file and retunr value
      int getNexusDataValue(const std::string& fileName, const std::string& dataName, std::string& value );

	private:
      /// Nexus file handle
      NXhandle fileID;

      /// obsolete function to write a nexus data array
      void write_data(NXhandle h, const char* name, const std::vector<double>& v);
      /// find number of MantidWorkspace_<n> entries in open nexus file
      int findMantidWSEntries();
      ///static reference to the logger class
      static Kernel::Logger& g_log;

    };

  } // namespace NeXus
} // namespace Mantid

#endif /* NEXUSUTILS_H */
