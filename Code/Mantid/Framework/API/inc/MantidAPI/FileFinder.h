#ifndef MANTID_API_FILEFINDER_H_
#define MANTID_API_FILEFINDER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/Logger.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/SingletonHolder.h"
#include "MantidAPI/DllConfig.h"
#include "MantidAPI/IArchiveSearch.h"

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
    class MANTID_API_DLL FileFinderImpl
    {
    public:
      std::string getFullPath(const std::string& fName)const;
      std::string makeFileName(const std::string& hint, const Kernel::FacilityInfo& facility)const;
      void setCaseSensitive(const bool cs); 
      std::string findRun(const std::string& hint,const std::set<std::string> *exts)const;
      std::string findRun(const std::string& hint,const std::vector<std::string> &exts  = std::vector<std::string>())const;
      std::vector<std::string> findRuns(const std::string& hint)const;
      /// DO NOT USE! MADE PUBLIC FOR TESTING ONLY.
      const Kernel::FacilityInfo getFacility(const std::string& hint) const;

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
      std::string getPath(const std::vector<IArchiveSearch_sptr>& archs, const std::string& fName)const;
      /// glob option - set to case sensitive or insensitive
      int globOption;

      /// reference to the logger class
      Mantid::Kernel::Logger& g_log;
    };

    ///Forward declaration of a specialisation of SingletonHolder for AlgorithmFactoryImpl (needed for dllexport/dllimport) and a typedef for it.
#ifdef _WIN32
    // this breaks new namespace declaraion rules; need to find a better fix
    template class MANTID_API_DLL Mantid::Kernel::SingletonHolder<FileFinderImpl>;
#endif /* _WIN32 */

    typedef MANTID_API_DLL Mantid::Kernel::SingletonHolder<FileFinderImpl> FileFinder;

  }
}

#endif //MANTID_API_FILEFINDER_H_
