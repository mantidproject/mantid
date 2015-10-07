#ifndef MANTID_ICAT_CATALOGLOGOUT_H_
#define MANTID_ICAT_CATALOGLOGOUT_H_

#include "MantidAPI/Algorithm.h"
#include "MantidICat/DllConfig.h"

namespace Mantid {
namespace ICat {

/**
 CatalogLogout is responsible for logging out of a catalog based on session
 information provided by the user.
 If no session information is provided this algorithm will log out of all active
 catalogs.

  @author Sofia Antony, STFC Rutherford Appleton Laboratory
  @date 23/07/2010
  Copyright &copy; 2010 STFC Rutherford Appleton Laboratory & NScD Oak Ridge
 National Laboratory

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
class MANTID_ICAT_DLL CatalogLogout : public API::Algorithm {
public:
  /// Constructor
  CatalogLogout() : API::Algorithm() {}
  /// Destructor
  ~CatalogLogout() {}
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "CatalogLogout"; }
  /// Summary of algorithms purpose.
  virtual const std::string summary() const {
    return "Logs out all catalogs, or a specific catalog using the session "
           "information provided.";
  }
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "DataHandling\\Catalog"; }

private:
  void init();
  void exec();
};
}
}

#endif
