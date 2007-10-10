#ifndef ALGORITHMFACTORY_H_
#define ALGORITHMFACTORY_H_

#define DECLARE_ALGORITHM(algorithmclass) 

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "IAlgorithm.h"
#include <map>

namespace Mantid
{
/** @class AlgorithmFactory AlgorithmFactory.h Kernel/AlgorithmFactory.h

    The AlgorithmFactory class is in charge of the creation of concrete
    instances of Algorithms. It is implemented as a singleton class.
    
    @author Russell Taylor, Tessella Support Services plc
    @date 21/09/2007
    
    Copyright � 2007 ???RAL???

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
  class AlgorithmFactory 
  {
  public:
    
    /** A static method which retrieves the single instance of the Algorithm Factory
     * 
     *  @returns A pointer to the factory instance
     */
    static AlgorithmFactory* Instance()
    {
      if (!m_instance) m_instance = new AlgorithmFactory;
      return m_instance;
    }

    typedef IAlgorithm* (*AlgCreator)();
    
    /** Register the name and creator function of an algorithm
     * 
     *  @param algtype The algorithm type name
     *  @param creator A pointer to the creator function for the algorithm
     *  @return A StatusCode object indicating whether the operation was successful
     */
    StatusCode subscribe( const std::string algtype, AlgCreator creator )
    {
      if (m_algs.insert(Associations::value_type(algtype,creator)).second)
      {
        return StatusCode::SUCCESS;
      }
      return StatusCode::FAILURE;
    }
    
    /** Removes a registered algorithm
     * 
     *  @param algtype The algorithm type name
     *  @return A StatusCode object indicating whether the operation was successful
     */
    StatusCode unsubscribe( const std::string algtype )
    {
      if (m_algs.erase(algtype)) return StatusCode::SUCCESS;
      return StatusCode::FAILURE;
    }
    
    /** Implementation of IAlgorithmFactory::createAlgorithm
     *  Create an instance of an algorithm type that has been previously 
     *  registered and assign it to a name.
     * 
     *  @param algtype Algorithm type name
     *  @param alg Returns a pointer to the newly created algorithm
     *  @returns A StatusCode object indicating whether the operation was successful
     */    
    virtual StatusCode createAlgorithm( const std::string algtype, IAlgorithm*& alg )
    {
      Associations::const_iterator it = m_algs.find(algtype);
      if (m_algs.end() != it)
      {
        alg = (it->second)();
        return StatusCode::SUCCESS;
      }
      return StatusCode::FAILURE;
    }   
    
    /** Implementation of IAlgorithmFactory::existsAlgorithm
     *  Check the existence of a given algorithm in the list of known algorithms
     *  @param algtype The type name of the algorithm to test for
     *  @returns Whether the algorithm is known or not
     */    
    virtual bool existsAlgorithm( const std::string& algtype ) const
    {
      Associations::const_iterator it = m_algs.find(algtype);
      if (m_algs.end() != it)
      {
        return true;
      }
      return false;
    }    
    
       
  private:

    /// Private Constructor for singleton class
    AlgorithmFactory() {}
    
    /** Private copy constructor
     *  Prevents singleton being copied
     */
    AlgorithmFactory(const AlgorithmFactory&) {}
    
    /** Private destructor
     *  Prevents client from calling 'delete' on the pointer handed 
     *  out by Instance
     */
    virtual ~AlgorithmFactory()
    {
      delete m_instance;
    }

    
    /// Pointer to the factory instance
    static AlgorithmFactory* m_instance;
    
    typedef std::map<std::string, AlgCreator> Associations;
    /// Map holding the subscribed algorithms
    Associations m_algs;
    
  };

  // Initialise the instance pointer to zero
  AlgorithmFactory* AlgorithmFactory::m_instance = 0;
  
  template <class T>
  class ConcreteAlgorithmCreator
  {
  public:
    static IAlgorithm* createInstance()
    {
      return new T();
    }
  };
}

#endif /*ALGORITHMFACTORY_H_*/
