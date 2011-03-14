#ifndef MANTID_API_FILEFINDER_H_
#define MANTID_API_FILEFINDER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/SingletonHolder.h"
#include "MantidAPI/DllExport.h"

#include <vector>
#include <set>

namespace Mantid
{
  namespace API
  {

    /**
    This class finds data files given an instrument name (optionally) and a run number

    @author Roman Tolchenov, Tessella plc
    @date 23/07/2010

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
    class EXPORT_OPT_MANTID_API FileFinderImpl
    {
    public:
      std::string getFullPath(const std::string& fName)const;
      std::string makeFileName(const std::string& hint)const;
      std::string findRun(const std::string& hint,const std::set<std::string> *exts = NULL)const;
      std::vector<std::string> findRuns(const std::string& hint)const;

    private:
      friend struct Mantid::Kernel::CreateUsingNew<FileFinderImpl>;

      /// a string that is allowed at the end of any run number
      static const std::string ALLOWED_SUFFIX;
      /// Default constructor
      FileFinderImpl();
      /// Copy constructor
      FileFinderImpl(const FileFinderImpl&);
      /// Assignment operator
      FileFinderImpl& operator=(const FileFinderImpl&);
      std::string extractAllowedSuffix(std::string & userString) const;
      std::pair<std::string,std::string> toInstrumentAndNumber(const std::string& hint)const;

    };

    ///Forward declaration of a specialisation of SingletonHolder for AlgorithmFactoryImpl (needed for dllexport/dllimport) and a typedef for it.
#ifdef _WIN32
    // this breaks new namespace declaraion rules; need to find a better fix
    template class EXPORT_OPT_MANTID_API Mantid::Kernel::SingletonHolder<FileFinderImpl>;
#endif /* _WIN32 */

    typedef EXPORT_OPT_MANTID_API Mantid::Kernel::SingletonHolder<FileFinderImpl> FileFinder;

  }
}

#endif //MANTID_API_FILEFINDER_H_
