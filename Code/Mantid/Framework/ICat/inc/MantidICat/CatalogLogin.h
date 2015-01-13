#ifndef MANTID_ICAT_CATALOGLOGIN_H_
#define MANTID_ICAT_CATALOGLOGIN_H_

#include "MantidAPI/Algorithm.h"
#include "MantidICat/DllConfig.h"

namespace Mantid {
namespace ICat {
/**
 This class is responsible for authentication of credentials against the
 catalog.

       Required Properties:
 <UL>
  <LI> Username - The logged in user name </LI>
  <LI> Password - The password of the logged in user </LI>
  <LI> Facility name - The name of the facility to log in to </LI>
 </UL>

 @author Sofia Antony, ISIS Rutherford Appleton Laboratory
 @date 07/07/2010
 Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_ICAT_DLL CatalogLogin : public API::Algorithm {
public:
  /// constructor
  CatalogLogin() : API::Algorithm() {}
  /// Destructor
  ~CatalogLogin() {}
  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "CatalogLogin"; }
  /// Summary of algorithms purpose.
  virtual const std::string summary() const {
    return "Authenticates credentials against a given catalog.";
  }
  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "DataHandling\\Catalog"; }

private:
  /// Overwrites Algorithm method.
  void init();
  /// Overwrites Algorithm method
  void exec();
};
}
}
#endif
