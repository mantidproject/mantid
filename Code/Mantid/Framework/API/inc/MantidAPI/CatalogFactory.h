#ifndef MANTID_API_CATALOGFACTORYIMPL_H_
#define MANTID_API_CATALOGFACTORYIMPL_H_

/* Used to register Catalog classes into the factory. creates a global object in
 *an
 * anonymous namespace. The object itself does nothing, but the comma operator
 * is used in the call to its constructor to effect a call to the factory's
 * subscribe method.
 *
 * The second operation that this macro performs is to provide the definition
 * of the CatalogID method for the concrete Catalog.
 */
#define DECLARE_CATALOG(classname)                                             \
  namespace {                                                                  \
  Mantid::Kernel::RegistrationHelper                                           \
      register_alg_##classname(((Mantid::API::CatalogFactory::Instance()       \
                                     .subscribe<classname>(#classname)),       \
                                0));                                           \
  }

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include "MantidKernel/DynamicFactory.h"
#include "MantidKernel/SingletonHolder.h"

namespace Mantid {
namespace API {
//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
class ICatalog;

/**
 The factory is a singleton that hands out shared pointers to the base Catalog
 class.

 @author Sofia Antony, ISIS Rutherford Appleton Laboratory
 @date 01/10/2010
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
class MANTID_API_DLL CatalogFactoryImpl
    : public Kernel::DynamicFactory<ICatalog> {
private:
  friend struct Kernel::CreateUsingNew<CatalogFactoryImpl>;
  /// Private Constructor for singleton class
  CatalogFactoryImpl();
  /// Private copy constructor
  CatalogFactoryImpl(const CatalogFactoryImpl &);
  /// Private assignment operator
  CatalogFactoryImpl &operator=(const CatalogFactoryImpl &);
  /// Private Destructor
  virtual ~CatalogFactoryImpl();
  /// Stores pointers to already created Catalog instances, with their name as
  /// the key
  mutable std::map<std::string, boost::shared_ptr<ICatalog>> m_createdCatalogs;
};

/// Forward declaration of a specialisation of SingletonHolder for
/// CatalogFactoryImpl (needed for dllexport/dllimport) .
#ifdef _WIN32
// this breaks new namespace declaraion rules; need to find a better fix
template class MANTID_API_DLL
    Mantid::Kernel::SingletonHolder<CatalogFactoryImpl>;
#endif /* _WIN32 */
/// The specialisation of the SingletonHolder class that holds the
/// CatalogFactory
typedef MANTID_API_DLL Mantid::Kernel::SingletonHolder<CatalogFactoryImpl>
    CatalogFactory;

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_CATALOGFACTORYIMPL_H_*/
