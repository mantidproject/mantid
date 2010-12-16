#ifndef MANTID_API_LOADALGORITHMFACTORY_H_
#define MANTID_API_LOADALGORITHMFACTORY_H_

/* Used to register load algorithm  classes into the factory. creates a global object in an
 * anonymous namespace. The object itself does nothing, but the comma operator
 * is used in the call to its constructor to effect a call to the factory's
 * subscribe method.
 *
 * The second operation that this macro performs is to provide the definition
 * of the CatalogID method for the concrete Catalog.
 */
#define DECLARE_LOADALGORITHM(classname) \
  namespace { \
    Mantid::Kernel::RegistrationHelper register_loadalg_##classname( \
       ((Mantid::API::LoadAlgorithmFactory::Instance().subscribe<classname>(#classname)) \
       , 0)); \
  } 

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllExport.h"
#include "MantidKernel/DynamicFactory.h"
#include "MantidKernel/SingletonHolder.h"
#include "MantidAPI/IDataFileChecker.h"

namespace Mantid
{
namespace API
{
/** Creates instances of concrete DataFileLoder.
    The factory is a singleton that hands out shared pointers to the base DataFileLoder class.
    It overrides the base class DynamicFactory::create method so that only a single
    instance of a given datafileloader is ever created, and a pointer to that same instance
    is passed out each time the datafileloader is requested.

    @author Sofia Antony
    @date 23/11/2010

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class EXPORT_OPT_MANTID_API LoadAlgorithmFactoryImpl : public Kernel::DynamicFactory<IDataFileChecker>
{
public:
	/// create an instance of the datafileloader specified by the calssName
  virtual boost::shared_ptr<IDataFileChecker> create(const std::string& className) const;
  
private:
  friend struct Kernel::CreateUsingNew<LoadAlgorithmFactoryImpl>;

  /// Private Constructor for singleton class
  LoadAlgorithmFactoryImpl();
  /// Private copy constructor 
  LoadAlgorithmFactoryImpl(const LoadAlgorithmFactoryImpl&);
  /// Private assignment operator 
  LoadAlgorithmFactoryImpl& operator = (const LoadAlgorithmFactoryImpl&);
  ///Private Destructor
  virtual ~LoadAlgorithmFactoryImpl();

  /// Reference to the logger class
  Kernel::Logger& m_log;
};

///Forward declaration of a specialisation of SingletonHolder for AlgorithmFactoryImpl (needed for dllexport/dllimport) .
#ifdef _WIN32
// this breaks new namespace declaraion rules; need to find a better fix
  template class EXPORT_OPT_MANTID_API Mantid::Kernel::SingletonHolder<LoadAlgorithmFactoryImpl>;
#endif /* _WIN32 */
/// The specialisation of the SingletonHolder class that holds the LoadAlgorithm
typedef Mantid::Kernel::SingletonHolder<LoadAlgorithmFactoryImpl> LoadAlgorithmFactory;

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_LOADALGORITHMFACTORY_H_*/
