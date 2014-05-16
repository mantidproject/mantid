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
      os << "\n";
    }
    //create the string for this algorithm
    os << buildAlgorithmString(algHistory) << "\n";
  }
}

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
      properties << prop;

      if(boost::next(propIter) != props.cend() 
          && !(boost::next(propIter, 2) == props.cend() && boost::next(propIter)->isDefault()))
      {
        properties << ", ";
      } 
    }
  }

  return name + "(" + properties.str() + ")";
}

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
