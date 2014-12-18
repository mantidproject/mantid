//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/HistoryItem.h"
#include "MantidAPI/ScriptBuilder.h"

#include <boost/utility.hpp>

namespace Mantid
{
namespace API
{

using Mantid::Kernel::PropertyHistory_sptr;
using Mantid::Kernel::PropertyHistory_const_sptr;

ScriptBuilder::ScriptBuilder(boost::shared_ptr<HistoryView> view, std::string versionSpecificity)
  : m_historyItems(view->getAlgorithmsList()), m_output(), m_versionSpecificity(versionSpecificity)
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
  auto iter = m_historyItems.begin();
  for( ; iter != m_historyItems.end(); ++iter)
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
 * @param depth :: count of how far we've recursed into the history
 */
void ScriptBuilder::writeHistoryToStream(std::ostringstream& os, std::vector<HistoryItem>::const_iterator& iter, int depth)
{
  auto algHistory = iter->getAlgorithmHistory();
  if(iter->isUnrolled())
  {
    os << "\n";
    os << std::string(depth, '#');
    os << " Child algorithms of " << algHistory->name() << "\n";

    //don't create a line for the algorithm, just output its children
    buildChildren(os, iter, depth+1);
    
    os << std::string(depth, '#');
    os << " End of child algorithms of " << algHistory->name() << "\n";
    
    if(boost::next(iter) == m_historyItems.end() 
        || !boost::next(iter)->isUnrolled())
    {
      os << "\n";
    }
  }
  else
  {
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
 * @param depth :: count of how far we've recursed into the history
 */
void ScriptBuilder::buildChildren(std::ostringstream& os, std::vector<HistoryItem>::const_iterator& iter, int depth)
{
  size_t numChildren = iter->numberOfChildren();
  ++iter; //move to first child
  for(size_t i = 0; i < numChildren && iter != m_historyItems.end(); ++i, ++iter)
  {
    writeHistoryToStream(os, iter, depth);
  }
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
  for (auto propIter = props.begin(); propIter != props.end(); ++propIter)
  {
    prop = buildPropertyString(*propIter);    
    if(prop.length() > 0)
    {
      properties << prop << ", ";
    }
  }

  //Three cases, we can either specify the version of every algorithm...
  if(m_versionSpecificity == "all")
  {
    properties << "Version=" << algHistory->version() << ", ";
  }
  else if(m_versionSpecificity == "old")
  {
    //...or only specify algorithm versions when they're not the newest version
    bool oldVersion = false;

    std::vector<Algorithm_descriptor> descriptors = AlgorithmFactory::Instance().getDescriptors();
    for(auto dit = descriptors.begin(); dit != descriptors.end(); ++dit)
    {
      //If a newer version of this algorithm exists, then this must be an old version.
      if((*dit).name == algHistory->name() && (*dit).version > algHistory->version())
      {
        oldVersion = true;
        break;
      }
    }

    if(oldVersion)
    {
      properties << "Version=" << algHistory->version() << ", ";
    }
  }
  //Third case is we never specify the version, so do nothing.


  std::string propStr = properties.str();
  if (propStr.length() > 0)
  {  
    //remove trailing comma & space
    propStr.erase(propStr.size()-1);
    propStr.erase(propStr.size()-1);
  }

  return name + "(" + propStr + ")";
}

/**
 * Build the script output for a single property
 *
 * @param propHistory :: reference to a property history object
 * @returns std::string for this property
 */
const std::string ScriptBuilder::buildPropertyString(PropertyHistory_const_sptr propHistory)
{
  std::string prop = ""; 
  if (!propHistory->isDefault())
  {
    if(propHistory->type() == "number")
    {
      prop = propHistory->name() + "=" + propHistory->value();
    }
    else if(propHistory->type() == "boolean")
    {
      std::string value = (propHistory->value() == "1" ? "True" : "False");
      prop = propHistory->name() + "=" + value;
    }    
    else
    {
      std::string opener = "='"; 
      if (propHistory->value().find('\\') != std::string::npos )
      {
        opener= "=r'";
      }

      prop = propHistory->name() + opener + propHistory->value() + "'";
    }
  }

  return prop;
}


} // namespace API
} // namespace Mantid
