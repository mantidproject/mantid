#ifndef MANTID_API_FILELOADERREGISTRY_H_
#define MANTID_API_FILELOADERREGISTRY_H_

#include "MantidAPI/DllConfig.h"

#include <set>
#include <string>

namespace Mantid
{
  namespace API
  {

    /**


    Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
     */
    class MANTID_API_DLL FileLoaderRegistry
    {
    public:
      /// Default constructor
      FileLoaderRegistry();

      /// @returns the number of entries in the registry
      inline size_t size() const { return m_names.size(); }
      /// Adds an entry
      void subscribe(const std::string & name);
      /// Pick the best loader for the given filename
      std::string findLoader(const std::string & filename) const;

    private:
      /// The registered names
      std::set<std::string> m_names;
    };

  } // namespace API
} // namespace Mantid

#endif  /* MANTID_API_FILELOADERREGISTRY_H_ */
