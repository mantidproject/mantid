//-----------------------------------
// Includes
//-----------------------------------
#include "MantidAPI/WorkspaceTracer.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Algorithm.h"
#include "MantidKernel/PropertyHistory.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/ConfigService.h"

#include <Poco/Mutex.h>
#include <Poco/ScopedLock.h>
#include <Poco/Thread.h>

#include <iostream>

using namespace Mantid::API;
using namespace Mantid::Kernel;

// Initialise the logger
Logger& WorkspaceTracerImpl::g_log = Logger::get("WorkspaceTracerImpl");


//-------------------------------
// AlgorithmChain public method
//-------------------------------
/**
 * Run the algorithm chain 
 */
void WorkspaceTracerImpl::AlgorithmChain::executeChain()
{ 
  while( !m_Algorithms.empty() )
  {
    try
    {
      (*m_Algorithms.begin())->execute();
    }
    catch(std::runtime_error &)
    {
      g_log.error() << "An exception was thrown while attempting to rerun the " 
		    << (*m_Algorithms.begin())->name() 
		    << " algorithm. The chain has been terminated.";
      break;
    } 
    m_Algorithms.pop_front();
  }
}


//-----------------------------------
// WorkspaceTracerImpl Public methods
//-----------------------------------
/**
 * A handler for a Poco notification of a workspace replacement
 * @param pNf :: A pointer to the notification object
 */
void WorkspaceTracerImpl::handleAfterReplaceNotification(Mantid::API::WorkspaceAfterReplaceNotification_ptr  pNf)
{
  if( m_isSwitchedOn && !m_isRunning )
  {
    executeTrace(pNf->object_name());
  }
}

//-----------------------------------
// WorkspaceTracerImpl Private methods
//-----------------------------------
/**
 * Default constructor
 */
WorkspaceTracerImpl::WorkspaceTracerImpl() :
  m_wkspAftReplaceObserver(*this, &Mantid::API::WorkspaceTracerImpl::handleAfterReplaceNotification), 
  executeTrace(this, &Mantid::API::WorkspaceTracerImpl::executeTraceImpl),
  m_strWsName(""), m_vecAlgHistories(), m_algChain(), m_mutex(), m_isRunning(false), 
  m_isSwitchedOn(false)
{
  //Check the configure properties to see if we should switch it on
  int isOn(-1);
  int result = Kernel::ConfigService::Instance().getValue("AlgorithmChaining.SwitchedOn", isOn);
  if( result == 0 ) m_isSwitchedOn = false;
  else
  {
    if( isOn > 0 ) m_isSwitchedOn = true;
    else m_isSwitchedOn = false;
  }
  
  // M. Gigg 2010-11-09: Disabled this as it is dangerous to use with scripts. Future work
  // may see a slightly altered version of it used
  if( m_isSwitchedOn )
  {
    g_log.error() << "Algorithm chaining has been requested to switch on "
		  << "but is dangerous to use with scripts and is therefore currently unavailable.\n";
  }
  m_isSwitchedOn = false;  
  
}

/**
 * Default destructor
 */
WorkspaceTracerImpl::~WorkspaceTracerImpl()
{
}

/**
 * Executes in a separate thread
 * @param wsname :: The name of the replaced workspace
 * @return nothing
 */
Poco::Void WorkspaceTracerImpl::executeTraceImpl(const std::string & wsname)
{
  Poco::ScopedLock<Poco::Mutex> s_lock(m_mutex);
  m_isRunning = true;

  // When the workspace replace signal is received, the algorithm that
  // caused it hasn't finised yet. This waits until that occurs, however, maybe using
  // the finished notification as well would be better but I don't know how to get this to
  // listen for those events
  Poco::Thread::sleep(2000);

  m_strWsName = wsname;
  createAlgorithmList();
  
  m_algChain.executeChain();
  m_isRunning = false;
  return Poco::Void();
}

/**
 * Find the list algorithms to run given that the workspace indicated has been refreshed
 */
void WorkspaceTracerImpl::createAlgorithmList()
{
  m_vecAlgHistories.clear();
  //Need to create a temp copy of object names as the vector gets dynamically created by the
  //function call
  std::set<std::string> currentWorkspaces = AnalysisDataService::Instance().getObjectNames();
  std::set<std::string>::const_iterator sIter = currentWorkspaces.begin();
  std::vector<std::string> updateWorkspaces;
  for( ; sIter != currentWorkspaces.end(); ++sIter )
  {
    //Examine history for base workspace as input
    if( propertyExists(*sIter, getBaseWorkspace(), Direction::Input) )
    {
      updateWorkspaces.push_back(*sIter);
    }
  }

  //Now subtract the base algorithm chain from the end point algorithm chains so that the
  // algorithms before the refreshed workspace are not run
  std::vector<AlgorithmHistory> baseChain;
  getBaseAlgorithmChain(baseChain);
  std::vector<std::string>::const_iterator wIter = updateWorkspaces.begin();
  for( ; wIter != updateWorkspaces.end(); ++wIter )
  {
    const WorkspaceHistory* hist = &FrameworkManager::Instance().getWorkspace(*wIter)->getHistory();
    std::vector<AlgorithmHistory>::const_iterator aIter = hist->getAlgorithmHistories().begin();
    std::vector<AlgorithmHistory>::const_iterator aEnd = hist->getAlgorithmHistories().end();
    for(; aIter != aEnd; ++aIter )
    {
      if( std::find(baseChain.begin(), baseChain.end(), *aIter) == baseChain.end() &&
	  std::find(m_vecAlgHistories.begin(), m_vecAlgHistories.end(), *aIter) == m_vecAlgHistories.end() &&
	  hasWorkspaceInput(*aIter) )
      {
	m_vecAlgHistories.push_back(*aIter);
	Algorithm* alg = getAlgorithm(*aIter);
	m_algChain.addToEnd(alg);
      }
    }
  }
 
}

/**
 * Examine the history of the given workspace and determine whether the property value specified
 * exists in any of the algorithms present with the specified direction can also be specifed
 * @param wsName :: The workspace to test
 * @param pvalue :: The property value to search for
 * @param dir :: Whether it should be an input or output property
 * @returns bool Indicating whether a match was found
 */
bool WorkspaceTracerImpl::propertyExists(const std::string & wsName, const std::string & pvalue, 
				     unsigned int dir)
{
  const WorkspaceHistory* hist = &FrameworkManager::Instance().getWorkspace(wsName)->getHistory();
  std::vector<AlgorithmHistory>::const_iterator aIter = hist->getAlgorithmHistories().begin();
  std::vector<AlgorithmHistory>::const_iterator aEnd = hist->getAlgorithmHistories().end();
  for(; aIter != aEnd; ++aIter )
  {
    if( propertyExists(*aIter, pvalue, dir) ) return true;
  }
  return false;
}

/**
 * Examine the given algorithm history and find whether the given property value exists and whether it has the correct
 * direction
 * @param algHist :: The AlorithmHistory object to be tested
 * @param pvalue :: The property value to test
 * @param dir :: Whether it should be an input or output property
 * @returns bool Indicating whether a match was found
 */
bool WorkspaceTracerImpl::propertyExists(const AlgorithmHistory & algHist, const std::string & pvalue, 
				     unsigned int dir)
{
  //Look at property history of each
  std::vector<PropertyHistory>::const_iterator pIter = algHist.getProperties().begin();
  std::vector<PropertyHistory>::const_iterator pEnd = algHist.getProperties().end();
  for( ; pIter != pEnd; ++pIter )
  {
    if( pIter->value() == pvalue )
    {
      if( dir == Direction::None ) return true;
      if( pIter->direction() == dir || pIter->direction() == Direction::InOut ) return true;
    }
  }
  return false;
}

/**
 * Retrieve the algorithm history for the workspace named in getBaseWorkspace but with
 * checks for things such as workspaces replacing themselves and do not have them in the list
 * @param baseChain :: A reference for a vector of AlgorithmHistory objects 
 */
void WorkspaceTracerImpl::getBaseAlgorithmChain(std::vector<Mantid::API::AlgorithmHistory> & baseChain)
{
  Workspace* ws = FrameworkManager::Instance().getWorkspace(getBaseWorkspace());
  std::vector<AlgorithmHistory>::const_iterator aIter = ws->getHistory().getAlgorithmHistories().begin();
  std::vector<AlgorithmHistory>::const_iterator aEnd = ws->getHistory().getAlgorithmHistories().end();
  for( ; aIter != aEnd; ++aIter )
  {
    if( !propertyExists(*aIter, getBaseWorkspace(), Direction::Input) ) baseChain.push_back(*aIter);
  }
}

/**
 * Examine the given algorithm history and find whether it takes a workspace for input
 * @param algHist :: The AlorithmHistory object to be tested
 * @returns bool Indicating whether there is at least one workspace as input
 */
bool WorkspaceTracerImpl::hasWorkspaceInput(const AlgorithmHistory & algHist)
{
  //Look at property history of each
  std::vector<PropertyHistory>::const_iterator pIter = algHist.getProperties().begin();
  std::vector<PropertyHistory>::const_iterator pEnd = algHist.getProperties().end();
  for( ; pIter != pEnd; ++pIter )
  {
    if( (pIter->direction() == Direction::Input ||  pIter->direction() == Direction::InOut ) &&
	isWorkspaceProperty(*pIter) )
    {
      return true;
    }
  }
  return false;
}

/**
 * Get or create an algorithm based upon the given AlgorithmHistory object. It checks whether
 * the parameter values now make sense given that the input is different
 * @param algHist :: The algorithm history to base the algorithm on
 * @returns An algorithm pointer with its properties set correctly
 */
Algorithm* WorkspaceTracerImpl::getAlgorithm(const AlgorithmHistory & algHist)
{
  Algorithm* alg = dynamic_cast<Algorithm*>(FrameworkManager::Instance().createAlgorithm(algHist.name()));
  //Set properties
  std::vector<PropertyHistory>::const_iterator pIter = algHist.getProperties().begin();
  std::vector<PropertyHistory>::const_iterator pEnd = algHist.getProperties().end();
  for( ; pIter != pEnd; ++pIter )
  {
    //If the new algorithm doesn't have it yet then skip it
    if( !alg->existsProperty(pIter->name()) ) continue;

    if( !pIter->isDefault() )
      alg->setPropertyValue(pIter->name(), pIter->value());
    else continue;

    if( pIter->direction() == Direction::Input && isWorkspaceProperty(*pIter) )
    {
      if( !AnalysisDataService::Instance().doesExist(pIter->value()) )
      {
	g_log.warning() << "The input workspace \"" << pIter->value() << "\""
			<< " for the " << algHist.name() << " algorithm does not exist, remaking it.\n";
	//Needs to be remade since it has been removed
	Algorithm* subalg = remakeWorkspace(pIter->value());
	m_algChain.addToEnd(subalg);
      }
    }
  }  
  return alg;
}

/**
 * Test whether the given PropertyHistory object relates to a workspace
 * @param prop :: The PropertyHistory object to test
 * @returns True if this PropertyHistory relates to a workspace, false otherwise
 */
bool WorkspaceTracerImpl::isWorkspaceProperty(const PropertyHistory & prop)
{
  std::string name = prop.name();
  std::transform(name.begin(), name.end(), name.begin(), toupper);
  if( name.find("WORKSPACE") != std::string::npos ) return true;
  else return false;
}

/**
 * Remake a workspace that has been deleted from the service
 * @param wsName :: The workspace to remake
 * @returns A pointer to the Algorithm object that created this workspace
 */
Algorithm* WorkspaceTracerImpl::remakeWorkspace(const std::string & wsName)
{
  std::set<std::string> currentWorkspaces = AnalysisDataService::Instance().getObjectNames();
  std::set<std::string>::const_iterator sIter = currentWorkspaces.begin();
  Algorithm* alg(NULL);
  for( ; sIter != currentWorkspaces.end(); ++sIter )
  {
    const WorkspaceHistory* hist = &FrameworkManager::Instance().getWorkspace(*sIter)->getHistory();
    std::vector<AlgorithmHistory>::const_iterator aIter = hist->getAlgorithmHistories().begin();
    std::vector<AlgorithmHistory>::const_iterator aEnd = hist->getAlgorithmHistories().end();
    for(; aIter != aEnd; ++aIter )
    {
      if( propertyExists(*aIter, wsName, Direction::Output) )
      {
	alg = getAlgorithm(*aIter);
	break;
      } 
    }
    if( alg ) break;
  }
  return alg;
}
