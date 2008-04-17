#ifndef MANTID_API_ALGORITHMFACTORY_H_
#define MANTID_API_ALGORITHMFACTORY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllExport.h"
#include "MantidKernel/DynamicFactory.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/SingletonHolder.h"

namespace Mantid
{
namespace API
{

//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
class Algorithm;

/** The AlgorithmFactory class is in charge of the creation of concrete
    instances of Algorithms. It inherits most of its implementation from
    the Dynamic Factory base class.
    It is implemented as a singleton class.
    
    @author Russell Taylor, Tessella Support Services plc
    @date 21/09/2007
    
    Copyright &copy; 2007 STFC Rutherford Appleton Laboratories

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
	  boost::shared_ptr<Algorithm> create(const std::string& ,const int& ) const;

	  template <class C>
	  void subscribe(const std::string& className)
	  {
		  Kernel::Instantiator<C, Algorithm>* newI = new Kernel::Instantiator<C, Algorithm>;
		  boost::shared_ptr<Algorithm> tempAlg = newI-> createInstance();
		  int version = tempAlg->version();
		  delete newI;
		  typename versionMap::iterator it = _vmap.find(className);
		  if (!className.empty())
		  {
			  if( it == _vmap.end())
				  _vmap[className] = version;	
			  else
			  {
				  if(version == it->second )
					  throw std::runtime_error("cannot register algorithm "+ className + " twice with the same version\n");
				  if(version > it->second)
					  _vmap[className]=version;
			  }  
			  DynamicFactory<Algorithm>::subscribe<C>(createName(className,version));	
		  }
	  }

  private:
	friend struct Mantid::Kernel::CreateUsingNew<AlgorithmFactoryImpl>;
	
	/// Private Constructor for singleton class
	AlgorithmFactoryImpl();	
	/// Private copy constructor - NO COPY ALLOWED
	AlgorithmFactoryImpl(const AlgorithmFactoryImpl&);
	/// Private assignment operator - NO ASSIGNMENT ALLOWED
	AlgorithmFactoryImpl& operator = (const AlgorithmFactoryImpl&);
	///Private Destructor
	virtual ~AlgorithmFactoryImpl();
	std::string createName(const std::string&, const int&)const;
	///static reference to the logger class
	Kernel::Logger& g_log;
  
	/// A typedef for the map of algorithm versions
	 typedef std::map<std::string, int> versionMap;
	 /// The map holding the registered class names and their highest versions
	 versionMap _vmap;

  };
  
	///Forward declaration of a specialisation of SingletonHolder for AlgorithmFactoryImpl (needed for dllexport/dllimport) and a typedef for it.
#ifdef _WIN32
// this breaks new namespace declaraion rules; need to find a better fix
	template class EXPORT_OPT_MANTID_API Mantid::Kernel::SingletonHolder<AlgorithmFactoryImpl>;
#endif /* _WIN32 */
	typedef EXPORT_OPT_MANTID_API Mantid::Kernel::SingletonHolder<AlgorithmFactoryImpl> AlgorithmFactory;
	
} // namespace API
} // namespace Mantid

#endif /*MANTID_API_ALGORITHMFACTORY_H_*/
