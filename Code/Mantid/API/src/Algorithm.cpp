//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AnalysisDataService.h"

using namespace Mantid::Kernel;

namespace Mantid
{
namespace API
{

// Get a reference to the logger
Kernel::Logger& Algorithm::g_log = Kernel::Logger::get("Algorithm");

/// Constructor
Algorithm::Algorithm() :
  PropertyManager(),_executeAsync(this,&Algorithm::executeAsyncImpl),m_isInitialized(false),
  m_isExecuted(false),m_isChildAlgorithm(false),m_cancel(false),m_runningAsync(false)
{}

/// Virtual destructor
Algorithm::~Algorithm()
{}

/** Initialization method invoked by the framework. This method is responsible
 *  for any bookkeeping of initialization required by the framework itself.
 *  It will in turn invoke the init() method of the derived algorithm,
 *  and of any sub-algorithms which it creates.
 *  @throw runtime_error Thrown if algorithm or sub-algorithm cannot be initialised
 *
 */
void Algorithm::initialize()
{
  // Bypass the initialization if the algorithm has already been initialized.
  if (m_isInitialized) return;

  try
  {
    // Invoke init() method of the derived class inside a try/catch clause
    try
    {
      this->init();
    }
    catch(std::runtime_error& ex)
    {
      g_log.error()<<"Error initializing main algorithm"<<ex.what();
      throw;
    }

    // Indicate that this Algorithm has been initialized to prevent duplicate attempts.
    setInitialized();
  }
  // Unpleasant catch-all! Along with this, Gaudi version catches GaudiException & std::exception
  // but doesn't really do anything except (print fatal) messages.
  catch (...)
  {
    // Gaudi: A call to the auditor service is here
    // (1) perform the printout
    g_log.fatal("UNKNOWN Exception is caught");
    throw;
  }

  // Only gets to here if everything worked normally
  return;
}

/** The actions to be performed by the algorithm on a dataset. This method is
 *  invoked for top level algorithms by the application manager.
 *  This method invokes exec() method.
 *  For sub-algorithms either the execute() method or exec() method
 *  must be EXPLICITLY invoked by  the parent algorithm.
 *
 *  @throw runtime_error Thrown if algorithm or sub-algorithm cannot be executed
 */
bool Algorithm::execute()
{
  clock_t start,end;
  time_t start_time;
  // Return a failure if the algorithm hasn't been initialized
  if ( !isInitialized() )
  {
    g_log.error("Algorithm is not initialized:" + this->name());
    throw std::runtime_error("Algorithm is not initialised:" + this->name());
  }

  // no logging of input if a child algorithm
  if (!m_isChildAlgorithm) algorithm_info();

  // Check all properties for validity
  if ( !validateProperties() )
  {
    // Reset name on input workspaces to trigger attempt at collection from ADS
    const std::vector< Property*> &props = getProperties();
    for (unsigned int i = 0; i < props.size(); ++i)
    {
      IWorkspaceProperty *wsProp = dynamic_cast<IWorkspaceProperty*>(props[i]);
      if (wsProp && !(wsProp->getWorkspace()))
      {
        // Setting it's name to the same one it alreasy had
        props[i]->setValue(props[i]->value());
      }
    }
    // Try the validation again
    if ( !validateProperties() ) throw std::runtime_error("Some invalid Properties found");
  }

  // Invoke exec() method of derived class and catch all uncaught exceptions
  try
  {
    try
    {
      time(&start_time);
      start = clock();
      // Call the concrete algorithm's exec method
      this->exec();
      end = clock();
      // need it to throw before trying to run fillhistory() on an algorithm which has failed
      // Put any output workspaces into the AnalysisDataService - if this is not a child algorithm
      if (!isChild())
      {
        fillHistory(start_time,double(end - start)/CLOCKS_PER_SEC);
        this->store();
      }

      // RJT, 19/3/08: Moved this up from below the catch blocks
      setExecuted(true);
      if (!m_isChildAlgorithm) g_log.information() << "Algorithm successful, Duration "
                                     << double(end - start)/CLOCKS_PER_SEC << " seconds" << std::endl;
      m_children.clear();
    }
    catch(std::runtime_error& ex)
    {
      m_children.clear();
      g_log.error()<< "Error in Execution of algorithm "<< this->name()<<std::endl;
      g_log.error()<< ex.what()<<std::endl;
      if (m_isChildAlgorithm || m_runningAsync) throw;
    }
    catch(std::logic_error& ex)
    {
      m_children.clear();
      g_log.error()<< "Logic Error in Execution of algorithm "<< this->name()<<std::endl;
      g_log.error()<< ex.what()<<std::endl;
      if (m_isChildAlgorithm || m_runningAsync) throw;
    }
  }
  catch(CancelException& ex)
  {
      g_log.error("Execution terminated by user.");
      throw;
  }
  // Gaudi also specifically catches GaudiException & std:exception.
  catch (...)
  {
    // Gaudi sets the executed flag to true here despite the exception
    // This allows it to move to the next command or it just loops indefinitely.
    // we will set it to false (see Nick Draper) 6/12/07
    setExecuted(false);

    g_log.error("UNKNOWN Exception is caught ");
    throw;
    // Gaudi calls exception service 'handle' method here
  }

  // Gaudi has some stuff here where it tests for failure, increments the error counter
  // and then converts to success if less than the maximum. This is clearly related to
  // having an event loop, and thus we shouldn't want it. This is the only place it's used.

  // Only gets to here if algorithm ended normally
  return isExecuted();
}

/// Has the Algorithm already been initialized
bool Algorithm::isInitialized() const
{
  return m_isInitialized;
}

/// Has the Algorithm already been executed
bool Algorithm::isExecuted() const
{
  return m_isExecuted;
}

/** Create a sub algorithm.  A call to this method creates a child algorithm object.
 *  Using this mechanism instead of creating daughter
 *  algorithms directly via the new operator is prefered since then
 *  the framework can take care of all of the necessary book-keeping.
 *
 *  @param name    The concrete algorithm class of the sub algorithm
 *  @returns Set to point to the newly created algorithm object
 */
Algorithm_sptr Algorithm::createSubAlgorithm(const std::string& name)
{
  Algorithm_sptr alg = AlgorithmManager::Instance().createUnmanaged(name);
  //set as a child
  alg->setChild(true);

  m_children.push_back(alg);

  // Initialise the sub-algorithm
  try
  {
    alg->initialize();
  }
  catch (std::runtime_error& err)
  {
    g_log.error() << "Unable to initialise sub-algorithm " << name << std::endl;
  }

  // If output workspaces are nameless, give them a temporary name to satisfy validator
  const std::vector< Property*> &props = alg->getProperties();
  for (unsigned int i = 0; i < props.size(); ++i)
  {
    if (props[i]->direction() == 1 && dynamic_cast<IWorkspaceProperty*>(props[i]) )
    {
      if ( props[i]->value().empty() ) props[i]->setValue("ChildAlgOutput");
    }
  }


  return alg;
}

// IAlgorithm property methods. Pull in PropertyManager implementation.
void Algorithm::setPropertyValue(const std::string &name, const std::string &value)
{
  PropertyManager::setPropertyValue(name, value);
}

// IAlgorithm property methods. Pull in PropertyManager implementation.
void Algorithm::setProperties(const std::string& propertiesArray)
{
  PropertyManager::setProperties(propertiesArray);
}

// IAlgorithm property methods. Pull in PropertyManager implementation.
void Algorithm::setPropertyOrdinal(const int &index, const std::string &value)
{
  PropertyManager::setPropertyOrdinal(index, value);
}

std::string Algorithm::getPropertyValue(const std::string &name) const
{
  return PropertyManager::getPropertyValue(name);
}

const std::vector< Mantid::Kernel::Property* >& Algorithm::getProperties() const
{
  return PropertyManager::getProperties();
}

//----------------------------------------------------------------------
// Protected Member Functions
//----------------------------------------------------------------------

/// Set the Algorithm initialized state
void Algorithm::setInitialized()
{
  m_isInitialized = true;
}

/// Set the executed flag to the specified state
// Public in Gaudi - don't know why and will leave here unless we find a reason otherwise
//     Also don't know reason for different return type and argument.
void Algorithm::setExecuted(bool state)
{
  m_isExecuted = state;
}

//----------------------------------------------------------------------
// Private Member Functions
//----------------------------------------------------------------------

/** Fills History, Algorithm History and Algorithm Parameters
 *  @param start a date and time defnining the start time of the algorithm
 *  @param duration a double defining the length of duration of the algorithm
 */
void Algorithm::fillHistory(AlgorithmHistory::dateAndTime start,double duration)
{
  // Create two vectors to hold a list of pointers to the input & output workspaces (InOut's go in both)
  std::vector<Workspace_sptr> inputWorkspaces, outputWorkspaces;
  findWorkspaceProperties(inputWorkspaces,outputWorkspaces);

  // Create the history object for this algorithm
  AlgorithmHistory algHistory(this,start,duration);

  std::vector<Workspace_sptr>::iterator outWS;
  std::vector<Workspace_sptr>::const_iterator inWS;
  // Loop over the output workspaces
  for (outWS = outputWorkspaces.begin(); outWS != outputWorkspaces.end(); ++outWS)
  {
    // Loop over the input workspaces, making the call that copies their history to the output ones
    // (Protection against copy to self is in WorkspaceHistory::copyAlgorithmHistory)
    for (inWS = inputWorkspaces.begin(); inWS != inputWorkspaces.end(); ++inWS)
    {
      (*outWS)->history().copyAlgorithmHistory( (*inWS)->getHistory() );
    }
    // Add the history for the current algorithm to all the output workspaces
    (*outWS)->history().addAlgorithmHistory(algHistory);
  }
}

/** Populate lists of the input & output workspace properties.
 *  (InOut workspaces go in both lists)
 *  @param inputWorkspaces  A reference to a vector for the input workspaces
 *  @param outputWorkspaces A reference to a vector for the output workspaces
 */
void Algorithm::findWorkspaceProperties(std::vector<Workspace_sptr>& inputWorkspaces,
                                        std::vector<Workspace_sptr>& outputWorkspaces) const
{
  // Loop over properties looking for the workspace properties and putting them in the right list
  const std::vector<Property*>& algProperties = getProperties();
  std::vector<Property*>::const_iterator it;
  for (it = algProperties.begin(); it != algProperties.end(); ++it)
  {
    const IWorkspaceProperty *wsProp = dynamic_cast<IWorkspaceProperty*>(*it);
    if (wsProp)
    {
      const Property *wsPropProp = dynamic_cast<Property*>(*it);
      unsigned int direction = wsPropProp->direction();
      if (direction == Direction::Input || direction == Direction::InOut)
      {
        inputWorkspaces.push_back(wsProp->getWorkspace());
      }
      if (direction == Direction::Output || direction == Direction::InOut)
      {
        outputWorkspaces.push_back(wsProp->getWorkspace());
      }
    }
  }
}

/// puts out algorithm parameter information to the logger
void Algorithm::algorithm_info() const
{
  // Make use of the AlgorithmHistory class, which holds all the info we want here
  AlgorithmHistory AH(this);
  g_log.information() << AH;
}

/** Stores any output workspaces into the AnalysisDataService
 *  @throw std::runtime_error If unable to successfully store an output workspace
 */
void Algorithm::store()
{
  const std::vector< Property*> &props = getProperties();
  for (unsigned int i = 0; i < props.size(); ++i)
  {
    IWorkspaceProperty *wsProp = dynamic_cast<IWorkspaceProperty*>(props[i]);
    if (wsProp)
    {
      try
      {
        wsProp->store();
      }
      catch (std::runtime_error& e)
      {
        g_log.error("Error storing output workspace in AnalysisDataService");
        throw;
      }
    }
  }
}

/** To query whether algorithm is a child.
 *  @returns true - the algorithm is a child algorithm.  False - this is a full managed algorithm.
 */
bool Algorithm::isChild() const
{
  return m_isChildAlgorithm;
}

/** To set whether algorithm is a child.
 *  @param isChild True - the algorithm is a child algorithm.  False - this is a full managed algorithm.
 */
void Algorithm::setChild(const bool isChild)
{
  m_isChildAlgorithm = isChild;
}

bool Algorithm::executeAsyncImpl(const int&)
{
    try
    {
        m_runningAsync = true;
        notificationCenter.postNotification(new StartedNotification(this));
        bool res = execute();
        notificationCenter.postNotification(new FinishedNotification(this,res));
        m_runningAsync = false;
        return res;
    }
    catch(std::exception& e)
    {
        notificationCenter.postNotification(new ErrorNotification(this,e.what()));
    }
    return false;
}

void Algorithm::cancel()
{
    m_cancel = true;
    for(std::vector<Algorithm_sptr>::iterator c=m_children.begin();c!=m_children.end();c++)
        (**c).cancel();
}

void Algorithm::progress(double p)
{
    notificationCenter.postNotification(new ProgressNotification(this,p));
}

void Algorithm::interruption_point()
{
    if (m_cancel) throw CancelException();
}

} // namespace API
} // namespace Mantid
