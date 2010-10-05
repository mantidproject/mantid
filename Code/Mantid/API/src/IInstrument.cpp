//---------------------------------------
// Includes
//--------------------------------------
#include "MantidAPI/IInstrument.h"
#include "MantidGeometry/IComponent.h"
#include <deque>

using namespace Mantid::API;

/**
 * Find the first component in an Instrument Definition File (IDF) with a given name. Use this method if 
 * you know that a given component in an IDF has a unique name. Otherwise use getAllComponentsWithName().
 * @param cname The name of the component. If there are multiple matches, the first one found is returned.
 * @returns A shared pointer to the component
*/
boost::shared_ptr<Mantid::Geometry::IComponent> IInstrument::getComponentByName(const std::string & cname)
{
  using namespace Mantid::Geometry;
  boost::shared_ptr<IComponent> node = boost::shared_ptr<IComponent>(this, NoDeleting());
  // Check the instrument name first
  if( this->getName() == cname ) 
  {
    return node;
  }
  // Otherwise Search the instrument tree using a breadth-first search algorithm since most likely candidates 
  // are higher-level components
  // I found some useful info here http://www.cs.bu.edu/teaching/c/tree/breadth-first/
  std::deque<boost::shared_ptr<IComponent> > nodeQueue;
  // Need to be able to enter the while loop
  nodeQueue.push_back(node);
  while( !nodeQueue.empty() )
  {
    node = nodeQueue.front();
    nodeQueue.pop_front();
    int nchildren(0);
    boost::shared_ptr<ICompAssembly> asmb = boost::dynamic_pointer_cast<ICompAssembly>(node);
    if( asmb )
    {
      nchildren = asmb->nelements();
    }
    for( int i = 0; i < nchildren; ++i )
    {
      boost::shared_ptr<Geometry::IComponent> comp = (*asmb)[i];
      if( comp->getName() == cname )
      {
        return comp;
      }
      else
      {
        nodeQueue.push_back(comp);
      }
    } 
  }// while-end

  // If we have reached here then the search failed
  return boost::shared_ptr<IComponent>();
}


/**
 * Find all components in an Instrument Definition File (IDF) with a given name. If you know a component
 * has a unique name use instead getComponentByName(), which is as fast or faster for retrieving a uniquely
   named component.
 * @param cname The name of the component. If there are multiple matches, the first one found is returned.
 * @returns Pointers to components
*/
std::vector<boost::shared_ptr<Mantid::Geometry::IComponent> > IInstrument::getAllComponentsWithName(const std::string & cname)
{
  using namespace Mantid::Geometry;
  boost::shared_ptr<IComponent> node = boost::shared_ptr<IComponent>(this, NoDeleting());
  std::vector<boost::shared_ptr<IComponent> > retVec;
  // Check the instrument name first
  if( this->getName() == cname ) 
  {
    retVec.push_back(node);
  }
  // Same algorithm as used in getComponentByName() but searching the full tree
  std::deque<boost::shared_ptr<IComponent> > nodeQueue;
  // Need to be able to enter the while loop
  nodeQueue.push_back(node);
  while( !nodeQueue.empty() )
  {
    node = nodeQueue.front();
    nodeQueue.pop_front();
    int nchildren(0);
    boost::shared_ptr<ICompAssembly> asmb = boost::dynamic_pointer_cast<ICompAssembly>(node);
    if( asmb )
    {
      nchildren = asmb->nelements();
    }
    for( int i = 0; i < nchildren; ++i )
    {
      boost::shared_ptr<Geometry::IComponent> comp = (*asmb)[i];
      if( comp->getName() == cname )
      {
        retVec.push_back(comp);
      }
      else
      {
        nodeQueue.push_back(comp);
      }
    } 
  }// while-end

  // If we have reached here then the search failed
  return retVec;
}