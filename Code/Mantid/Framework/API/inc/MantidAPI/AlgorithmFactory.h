#ifndef MANTID_API_ALGORITHMFACTORY_H_
#define MANTID_API_ALGORITHMFACTORY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <vector>
#include "MantidAPI/DllExport.h"
#include "MantidKernel/DynamicFactory.h"
#include "MantidKernel/SingletonHolder.h"
#include "MantidKernel/Logger.h"

namespace Mantid
{
namespace API
{

/// Structure uniquely describing an algorithm with its name, category and version.
struct Algorithm_descriptor
{
  std::string name;///< name
  std::string category;///< category
  int version;///< version
};

//----------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------
class IAlgorithm;
class Algorithm;
class CloneableAlgorithm;

/** The AlgorithmFactory class is in charge of the creation of concrete
    instances of Algorithms. It inherits most of its implementation from
    the Dynamic Factory base class.
    It is implemented as a singleton class.
    
    @author Russell Taylor, Tessella Support Services plc
    @date 21/09/2007
    
    Copyright &copy; 2007-2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
*/
class EXPORT_OPT_MANTID_API AlgorithmFactoryImpl : public Kernel::DynamicFactory<Algorithm>
{
public:
  // Unhide the base class version (to satisfy the intel compiler)
  using Kernel::DynamicFactory<Algorithm>::create;
  ///Creates an instance of an algorithm
  boost::shared_ptr<Algorithm> create(const std::string& ,const int& ) const;

  /// algorithm factory specific function to subscribe algorithms, calls the dynamic factory subscribe function internally
  template <class C>
  void subscribe()
  {
    Kernel::Instantiator<C, Algorithm>* newI = new Kernel::Instantiator<C, Algorithm>;
    boost::shared_ptr<IAlgorithm> tempAlg = newI-> createInstance();

    const int version = extractAlgVersion(tempAlg);
    const std::string className = extractAlgName(tempAlg);
    delete newI;
    typename VersionMap::const_iterator it = m_vmap.find(className);
    if (!className.empty())
    {
      if( it == m_vmap.end())
        m_vmap[className] = version;	
      else
      {
        if(version == it->second )
        {
          g_log.fatal() << "Cannot register algorithm " << className << " twice with the same version\n";
          return;
        }
        if(version > it->second)
          m_vmap[className]=version;
      }  
      Kernel::DynamicFactory<Algorithm>::subscribe<C>(createName(className,version));	
    }
  }

  ///Get the algorithm keys
  const std::vector<std::string> getKeys() const;
  /// Returns algorithm descriptors.
  std::vector<Algorithm_descriptor> getDescriptors() const;
  /// Returns the highest version number of an algorithm
  int highestVersion(const std::string & algName) const;
  /// Return the version of the algorithm where the given property is first found
  int versionForProperty(const std::string & algName, const std::string & propName) const;
	  
  /// Store a pointer to an algorithm that has alread been constructed; for instance an algorithm created in Python
  bool storeCloneableAlgorithm(CloneableAlgorithm* algorithm);

 private:
  friend struct Mantid::Kernel::CreateUsingNew<AlgorithmFactoryImpl>;

  /// Extract the name of an algorithm
  const std::string extractAlgName(const boost::shared_ptr<IAlgorithm> alg) const;
  /// Extract the version of an algorithm
  int extractAlgVersion(const boost::shared_ptr<IAlgorithm> alg) const;

  ///Create an algorithm object with the specified name
  boost::shared_ptr<Algorithm> createAlgorithm(const std::string & name, const int version) const;
	
  /// Private Constructor for singleton class
  AlgorithmFactoryImpl();	
  /// Private copy constructor - NO COPY ALLOWED
  AlgorithmFactoryImpl(const AlgorithmFactoryImpl&);
  /// Private assignment operator - NO ASSIGNMENT ALLOWED
  AlgorithmFactoryImpl& operator = (const AlgorithmFactoryImpl&);
  ///Private Destructor
  virtual ~AlgorithmFactoryImpl();
  /// creates an algorithm name convolved from an name and version
  std::string createName(const std::string&, const int&)const;
  ///static reference to the logger class
  Kernel::Logger& g_log;

  /// A typedef for the map of algorithm versions
  typedef std::map<std::string, int> VersionMap;
  /// The map holding the registered class names and their highest versions
  VersionMap m_vmap;
  /// A hash table storing clean pointers to algorithms
  std::map<std::string, CloneableAlgorithm*> m_cloneable_algs;
};
  

///Forward declaration of a specialisation of SingletonHolder for AlgorithmFactoryImpl (needed for dllexport/dllimport) and a typedef for it.
#ifdef _WIN32
// this breaks new namespace declaraion rules; need to find a better fix
template class EXPORT_OPT_MANTID_API Mantid::Kernel::SingletonHolder<AlgorithmFactoryImpl>;
#endif /* _WIN32 */

typedef EXPORT_OPT_MANTID_API Mantid::Kernel::SingletonHolder<AlgorithmFactoryImpl> AlgorithmFactory;

/// Convenient typedef for an UpdateNotification
typedef Mantid::Kernel::DynamicFactory<Algorithm>::UpdateNotification AlgorithmFactoryUpdateNotification;
typedef const Poco::AutoPtr<Mantid::Kernel::DynamicFactory<Algorithm>::UpdateNotification> & AlgorithmFactoryUpdateNotification_ptr;

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_ALGORITHMFACTORY_H_*/
