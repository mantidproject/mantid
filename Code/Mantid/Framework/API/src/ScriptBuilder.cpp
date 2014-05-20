//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/HistoryItem.h"
#include "MantidAPI/ScriptBuilder.h"

#include <boost/utility.hpp>

namespace Mantid
{
namespace API
{


ScriptBuilder::ScriptBuilder(const HistoryView& view)
  : m_historyItems(view.getAlgorithmsList()), m_output()
{
}

/**
 * Build a python script for each algorithm included in the history view.
 *
 * @return a formatted python string of the history
 */
const std::string ScriptBuilder::build()
{
  std::ostringstream os;
  auto iter = m_historyItems.cbegin();
  for( ; iter != m_historyItems.cend(); ++iter)
  {
    writeHistoryToStream(os, iter);
  }
  return os.str();
}

/**
 * Write out an algorithm to the script.
 *
 * If the entry is unrolled this will recurse and output the children of
 * that entry instead. If not, it will just output the algorithm to the stream.
 *
 * @param os :: output string stream to append algorithms to.
 * @param iter :: reference to the iterator pointing to the vector of history items
 */
void ScriptBuilder::writeHistoryToStream(std::ostringstream& os, std::vector<HistoryItem>::const_iterator& iter, int depth)
{
  auto algHistory = iter->getAlgorithmHistory();
  if(iter->isUnrolled())
  {
    os << std::string(depth, '#');
    os << " Child algorithms of " << algHistory->name() << "\n";
    
    //don't create a line for the algorithm, just output its children
    buildChildren(os, iter, depth+1);
    
    os << std::string(depth, '#');
    os << " End of child algorithms of " << algHistory->name() << "\n";

    if(boost::next(iter) != m_historyItems.cend() 
        && !boost::next(iter)->isUnrolled())
    {
      os << "\n";
    }
  }
  else
  {
    if(boost::prior(iter) != m_historyItems.cbegin() 
        && boost::prior(iter)->isUnrolled())
    {
      // os << "\n";
    }
    //create the string for this algorithm
    os << buildAlgorithmString(algHistory) << "\n";
  }
}

/**
 * Iterate over each of the items children and output them to the script.
 *
 * This moves the iterator forward over each of the child records and writes them to 
 * the stream.
 *
 * @param os :: output string stream to append algorithms to.
 * @param iter :: reference to the iterator pointing to the vector of history items
 */
void ScriptBuilder::buildChildren(std::ostringstream& os, std::vector<HistoryItem>::const_iterator& iter, int depth)
{
  size_t numChildren = iter->numberOfChildren();
  ++iter; //move to first child
  for(size_t i = 0; i < numChildren && iter != m_historyItems.cend(); ++i, ++iter)
  {
    writeHistoryToStream(os, iter, depth);
  }
  os << "\n";
  --iter;
}

/**
 * Build the script output for a single algorithm
 *
 * @param algHistory :: pointer to an algorithm history object
 * @returns std::string to run this algorithm
 */
const std::string ScriptBuilder::buildAlgorithmString(AlgorithmHistory_const_sptr algHistory)
{
  std::ostringstream properties;
  const std::string name = algHistory->name();
  std::string prop = "";

  auto props = algHistory->getProperties();
  for (auto propIter = props.cbegin(); propIter != props.cend(); ++propIter)
  {
    prop = buildPropertyString(*propIter);    
    if(prop.length() > 0)
    {
      properties << prop << ", ";
    }
  }

  std::string propStr = properties.str();
  //remove trailing comma & space
  propStr.erase(propStr.size()-1);
  propStr.erase(propStr.size()-1);

  return name + "(" + propStr + ")";
}

/**
 * Build the script output for a single property
 *
 * @param propHistory :: reference to a property history object
 * @returns std::string for this property
 */
const std::string ScriptBuilder::buildPropertyString(const Mantid::Kernel::PropertyHistory& propHistory)
{
  std::string prop = ""; 
  
  if (!propHistory.isDefault())
  {
    prop = propHistory.name() + "='" + propHistory.value() + "'";
  }

  return prop;
}


} // namespace API
} // namespace Mantid
