#ifndef MANTID_DATAHANDLING_ISISDATAARCHIVE_H_
#define MANTID_DATAHANDLING_ISISDATAARCHIVE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include "MantidAPI/IArchiveSearch.h"

#include <string>

namespace Mantid
{
  namespace DataHandling
  {

    /**
    This class is for searching the ISIS data archive

    @author Roman Tolchenov, Tessella plc
    @date 27/07/2010

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class DLLExport ISISDataArchive: public API::IArchiveSearch
    {
    public:
      std::string getPath(const std::string& fName)const;
    };

  }
}

#endif //MANTID_DATAHANDLING_ISISDATAARCHIVE_H_
