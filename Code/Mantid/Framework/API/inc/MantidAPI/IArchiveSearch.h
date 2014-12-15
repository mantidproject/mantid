#ifndef MANTID_API_IARCHIVESEARCH_H_
#define MANTID_API_IARCHIVESEARCH_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"

#ifndef Q_MOC_RUN
# include <boost/shared_ptr.hpp>
#endif
#include <string>
#include <vector>
#include <set>


#define DECLARE_ARCHIVESEARCH(classname,facility) \
namespace { \
  Mantid::Kernel::RegistrationHelper register_archive_##facility( \
  ((Mantid::API::ArchiveSearchFactory::Instance().subscribe<classname>(#facility)) \
  , 0)); \
}

namespace Mantid
{
  namespace API
  {

    /**
    This class is an archive searching interface.

    @author Roman Tolchenov, Tessella plc
    @date 27/07/2010

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
    class MANTID_API_DLL IArchiveSearch
    {
    public:
      /// Virtual destructor
      virtual ~IArchiveSearch(){}
      /**
        * Return the full path to a data file in an archive. The first match is returned
        * @param filenames :: A list of filenames (without extensions) to pass to the archive
        * @param exts :: A list of extensions to check for in turn against each file
        */
      virtual std::string getArchivePath(const std::set<std::string>& filenames, const std::vector<std::string>& exts)const = 0;
    };

    ///Typedef for a shared pointer to an IArchiveSearch
    typedef boost::shared_ptr<IArchiveSearch> IArchiveSearch_sptr;

  }
}

#endif //MANTID_API_IARCHIVESEARCH_H_
