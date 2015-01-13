#ifndef MANTID_ICAT_CATALOGKEEPALIVE_H_
#define MANTID_ICAT_CATALOGKEEPALIVE_H_

#include "MantidAPI/Algorithm.h"
#include "MantidICat/DllConfig.h"

namespace Mantid {
namespace ICat {
/**
 CatalogKeepAlive is responsible for keeping a catalog alive based on the
 session information.

 Required Properties:

 <UL>
  <LI> Session - Session information used to obtain the specific catalog to keep
 alive.</LI>
 </UL>

 @author Jay Rainey, ISIS Rutherford Appleton Laboratory
 @date 17/03/2014
 Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
 National Laboratory & European Spallation Source

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
class MANTID_ICAT_DLL CatalogKeepAlive : public API::Algorithm {
public:
  /// constructor
  CatalogKeepAlive() : API::Algorithm() {}
  /// Algorithm's name for identification.
  virtual const std::string name() const { return "CatalogKeepAlive"; }
  /// Summary of algorithms purpose.
  virtual const std::string summary() const {
    return "Refreshes the current session to the maximum amount provided by "
           "the catalog API.";
  }
  /// Algorithm's version for identification.
  virtual int version() const { return 1; }
  /// Algorithm's category for identification.
  virtual const std::string category() const { return "DataHandling\\Catalog"; }

private:
  /// Override algorithm initialisation method.
  void init();
  /// Override algorithm execute method.
  void exec();
};
}
}
#endif
