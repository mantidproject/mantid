#ifndef MANTID_API_DOMAINCREATORFACTORY_H_
#define MANTID_API_DOMAINCREATORFACTORY_H_

#include "MantidAPI/DllConfig.h"
#include "MantidKernel/ClassMacros.h"
#include "MantidKernel/DynamicFactory.h"
#include "MantidKernel/SingletonHolder.h"

namespace Mantid
{
  //
  // Forward declarations
  //
  namespace Kernel
  {
    class IPropertyManager;
  }
  namespace API
  {
    //
    // Forward declarations
    //
    class IDomainCreator;

    /**

    Constructs a DomainCreator object from a string

    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
    class MANTID_API_DLL DomainCreatorFactoryImpl : public Kernel::DynamicFactory<IDomainCreator>
    {
    public:
      /// Returns an initialized domain creator
      IDomainCreator * createDomainCreator(const std::string & id, Kernel::IPropertyManager* pm,
                                           const std::string& workspacePropertyName,
                                           const unsigned int domainType) const;

    private:
      friend struct Mantid::Kernel::CreateUsingNew<DomainCreatorFactoryImpl>;

      /// Private Constructor for singleton class
      DomainCreatorFactoryImpl();
      /// No copying
      DISABLE_COPY_AND_ASSIGN(DomainCreatorFactoryImpl);
      ///Private Destructor for singleton
      virtual ~DomainCreatorFactoryImpl();

      // Do not use default methods
      using Kernel::DynamicFactory<IDomainCreator>::create;
      using Kernel::DynamicFactory<IDomainCreator>::createUnwrapped;
    };

    ///Forward declaration of a specialisation of SingletonHolder for DomainCreatorFactoryImpl (needed for dllexport/dllimport) and a typedef for it.
#ifdef _WIN32
// this breaks new namespace declaraion rules; need to find a better fix
    template class MANTID_API_DLL Mantid::Kernel::SingletonHolder<DomainCreatorFactoryImpl>;
#endif /* _WIN32 */
    typedef MANTID_API_DLL Mantid::Kernel::SingletonHolder<DomainCreatorFactoryImpl> DomainCreatorFactory;


  } // namespace API
} // namespace Mantid

#endif  /* MANTID_API_DOMAINCREATORFACTORY_H_ */
