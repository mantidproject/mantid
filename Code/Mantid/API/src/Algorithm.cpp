//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmProxy.h"
#include "MantidAPI/AnalysisDataService.h"

#include <iostream>

using namespace Mantid::Kernel;


namespace Mantid
{
namespace API
{

// Get a reference to the logger
Kernel::Logger& Algorithm::g_log = Kernel::Logger::get("Algorithm");

unsigned int Algorithm::g_execCount=0;

/// Constructor
Algorithm::Algorithm() :
  PropertyManagerOwner(),m_progressObserver(*this, &Algorithm::handleChildProgressNotification),
  m_executeAsync(this,&Algorithm::executeAsyncImpl),m_isInitialized(false),
  m_isExecuted(false),m_isChildAlgorithm(false),m_cancel(false),m_runningAsync(false),m_running(false),
  m_algorithmID(0)
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
  m_notificationCenter.postNotification(new StartedNotification(this));
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
// get properties and check one of the input properties is a work space group
// if it's a group call process group 
 // if not normal execution

  std::vector<Property*> Prop=getProperties();
  std::vector<Property*>::const_iterator itr;
  for (itr=Prop.begin();itr!=Prop.end();itr++)
  {
	  const IWorkspaceProperty *wsProp = dynamic_cast<IWorkspaceProperty*>(*itr);
	  if (wsProp)
	  { 
		  const Property *wsPropProp = dynamic_cast<Property*>(*itr);
		  unsigned int direction = wsPropProp->direction();
		  if (direction == Kernel::Direction::Input ||direction==Kernel::Direction::InOut)
		  { 
			  std::string wsName=wsPropProp->value();
			  try
			  {					
				  //checking the input is a group
				  try
				  {
					//check if the pointer is valid, it won't be if it is a group
					 Workspace_sptr wsSptr=wsProp->getWorkspace();
					 if(!wsSptr)
					 {
						 boost::shared_ptr<WorkspaceGroup> wsGrpSptr =
							 boost::dynamic_pointer_cast<WorkspaceGroup>(AnalysisDataService::Instance().retrieve(wsName));
						 if(wsGrpSptr)
						 {	 //this must be a group - test for that
							 g_log.debug()<<"input is workspace group-processGroups called "<<std::endl;
							 return(Algorithm::processGroups(wsGrpSptr,Prop));
					     }
					 }

				  }
				  catch (std::exception &ex)
				  {					 
					  g_log.debug()<<ex.what()<<std::endl; 
					
				  }
				 
			  }
			  catch(Mantid::Kernel::Exception::NotFoundError &e)//if not a valid object in analysis data service
			  {
				g_log.debug()<<" Failing to cast the workspace pointer of workspace  "<< wsName<<" Workspace group"<<std::endl;

			  }
		  }//end of if loop checking the direction
	  }//end of if loop for checking workspace properties

  }// end of for loop for checking the properties for workspace groups

  // Invoke exec() method of derived class and catch all uncaught exceptions
  try
  {
    try
    {
      if (!m_isChildAlgorithm) m_running = true;
      time(&start_time);
      start = clock();
      //count used to know how many times an algorithm executed
      ++Algorithm::g_execCount; 
      // Call the concrete algorithm's exec method
      this->exec();
      end = clock();
      // need it to throw before trying to run fillhistory() on an algorithm which has failed
      // Put any output workspaces into the AnalysisDataService - if this is not a child algorithm
      if (!isChild())
      {
        fillHistory(start_time,double(end - start)/CLOCKS_PER_SEC,Algorithm::g_execCount);
        this->store();
      }

      // RJT, 19/3/08: Moved this up from below the catch blocks
      setExecuted(true);
      if (!m_isChildAlgorithm) g_log.information() << "Algorithm successful, Duration "
                                     << double(end - start)/CLOCKS_PER_SEC << " seconds" << std::endl;
      m_running = false;
    }
    catch(std::runtime_error& ex)
    {
      if (m_isChildAlgorithm || m_runningAsync) throw;
      else
      {
          g_log.error()<< "Error in Execution of algorithm "<< this->name()<<std::endl;
          g_log.error()<< ex.what()<<std::endl;
      }
      m_notificationCenter.postNotification(new ErrorNotification(this,ex.what()));
      m_running = false;
    }
    catch(std::logic_error& ex)
    {
      if (m_isChildAlgorithm || m_runningAsync) throw;
      else
      {
          g_log.error()<< "Logic Error in Execution of algorithm "<< this->name()<<std::endl;
          g_log.error()<< ex.what()<<std::endl;
      }
      m_notificationCenter.postNotification(new ErrorNotification(this,ex.what()));
      m_running = false;
    }
  }
  catch(CancelException& ex)
  {
      m_runningAsync = false;
      m_running = false;
      g_log.error("Execution terminated by user.");
      m_notificationCenter.postNotification(new ErrorNotification(this,ex.what()));
      throw;
  }
  // Gaudi also specifically catches GaudiException & std:exception.
  catch (std::exception& ex)
  {
    setExecuted(false);
    m_runningAsync = false;
    m_running = false;

    m_notificationCenter.postNotification(new ErrorNotification(this,ex.what()));
    g_log.error(ex.what());
    throw;
  }

  catch (...)
  {
    // Gaudi sets the executed flag to true here despite the exception
    // This allows it to move to the next command or it just loops indefinitely.
    // we will set it to false (see Nick Draper) 6/12/07
    setExecuted(false);
    m_runningAsync = false;
    m_running = false;

    m_notificationCenter.postNotification(new ErrorNotification(this,"UNKNOWN Exception is caught "));
    g_log.error("UNKNOWN Exception is caught ");
    throw;
    // Gaudi calls exception service 'handle' method here
  }

  // Gaudi has some stuff here where it tests for failure, increments the error counter
  // and then converts to success if less than the maximum. This is clearly related to
  // having an event loop, and thus we shouldn't want it. This is the only place it's used.

   m_notificationCenter.postNotification(new FinishedNotification(this,isExecuted()));
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
 *  @param startProgress  The percentage progress value of the overall algorithm where this child algorithm starts
 *  @param endProgress    The percentage progress value of the overall algorithm where this child algorithm ends
 *  @returns Set to point to the newly created algorithm object
 */
IAlgorithm_sptr Algorithm::createSubAlgorithm(const std::string& name, double startProgress, double endProgress)
{
  IAlgorithm_sptr alg = AlgorithmManager::Instance().createUnmanaged(name);
  //set as a child
  alg->setChild(true);

  // Initialise the sub-algorithm
  try
  {
    alg->initialize();
  }
  catch (std::runtime_error&)
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

  if (startProgress >= 0 && endProgress > startProgress && endProgress <= 1.)
  {
      alg->addObserver(m_progressObserver);
      m_startChildProgress = startProgress;
      m_endChildProgress = endProgress;
  }

  return alg;
}

/**  Add an observer to a notification
     @param observer Reference to the observer to add
 */
void Algorithm::addObserver(const Poco::AbstractObserver& observer)const
{
    m_notificationCenter.addObserver(observer);
}

/**  Remove an observer
     @param observer Reference to the observer to remove
 */
void Algorithm::removeObserver(const Poco::AbstractObserver& observer)const
{
    m_notificationCenter.removeObserver(observer);
}

void Algorithm::cancel()const
{
    m_cancel = true;
}

//----------------------------------------------------------------------
// Protected Member Functions
//----------------------------------------------------------------------

/** Initialize using proxy algorithm.
    proxy calls this algorithm's init() method and keeps the declared properties.
    initialize(const AlgorithmProxy*) copies the properties from the proxy to this algorithm.
    @param proxy Initialising proxy algorithm
  */
void Algorithm::initializeFromProxy(const AlgorithmProxy& proxy)
{
    init();
    copyPropertiesFrom(proxy);
    setInitialized();
    m_algorithmID = proxy.getAlgorithmID();
}

/// Set the Algorithm initialized state
void Algorithm::setInitialized()
{
  m_isInitialized = true;
}

/** Set the executed flag to the specified state
// Public in Gaudi - don't know why and will leave here unless we find a reason otherwise
//     Also don't know reason for different return type and argument.
    @param state New executed state
*/
void Algorithm::setExecuted(bool state)
{
  m_isExecuted = state;
}

/** Sends ProgressNotification. 
    @param p Reported progress,  must be between 0 (just started) and 1 (finished)
    @param msg Optional message string
 */
void Algorithm::progress(double p, const std::string& msg)
{
    m_notificationCenter.postNotification(new ProgressNotification(this,p,msg));
}

void Algorithm::interruption_point()
{
    if (m_cancel) throw CancelException();
}

//----------------------------------------------------------------------
// Private Member Functions
//----------------------------------------------------------------------

/** Fills History, Algorithm History and Algorithm Parameters
 *  @param start a date and time defnining the start time of the algorithm
 *  @param duration a double defining the length of duration of the algorithm
 */
void Algorithm::fillHistory(AlgorithmHistory::dateAndTime start,double duration,unsigned int uexecCount)
{
  // Create two vectors to hold a list of pointers to the input & output workspaces (InOut's go in both)
  std::vector<Workspace_sptr> inputWorkspaces, outputWorkspaces;
  findWorkspaceProperties(inputWorkspaces,outputWorkspaces);

  // Create the history object for this algorithm
  AlgorithmHistory algHistory(this,start,duration,uexecCount);

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
      catch (std::runtime_error&)
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

/** To Process workspace groups.
 *  @param input workspacegroup pointer to iterate through all members
 *  @param  a vector holding the input properties
 *  @returns true - if all the workspace members are executed.
 */

bool Algorithm::processGroups(WorkspaceGroup_sptr inputwsPtr,const std::vector<Mantid::Kernel::Property*>&prop)
{	
	int nPeriod=1;
	int execPercentage=0;
	bool bgroupExecStatus=true;
	bool bgroupFailed=false;
	std::string outWSParentName("");
	WorkspaceGroup_sptr sptrWSGrp1; 
	WorkspaceGroup_sptr sptrWSGrp2;
	bool bnewGoup1=true;
	bool bnewGoup2=true;
	bool bStatus=false;
	
	//getting the input workspace group names
	std::vector<std::string> inputWSNames=inputwsPtr->getNames();
	int nSize=inputWSNames.size();
	//size is one if only group header.
	//return if atleast one meber is not there in group to process
	if(nSize<2)
	{	g_log.error()<<"Input WorkspaceGruop has no child workspaces  "<<std::endl;
		return false;
	}
	std::vector<std::string>::const_iterator wsItr=inputWSNames.begin();
	int execTotal=0;
	//removing the header count from the totalsize
	execTotal=(nSize-1)*10;
	m_notificationCenter.postNotification(new StartedNotification(this));
	
	//for each member in the input workspace group
	//starts from the 2nd item in the group as 1st item is group header
	for(++wsItr;wsItr!=inputWSNames.end();wsItr++)
	{		
		IAlgorithm* alg = API::FrameworkManager::Instance().createAlgorithm(this->name() ,"",1);
		//set  properties
		std::vector<Mantid::Kernel::Property*>::const_iterator itr;
		for (itr=prop.begin();itr!=prop.end();itr++)
		{	
			int outWSCount=0;		
			if(isWorkspaceProperty(*itr) )
			{
				if(isInputWorkspaceProperty(*itr))
				{setInputWSProperties(alg,*itr,*wsItr);
				}
				if(isOutputWorkspaceProperty(*itr))
				{
					++outWSCount;
					//create a group and pass that to setOutputWSProperties properties
					if(outWSCount==1)
					{ 	if( bnewGoup1)
							{	sptrWSGrp1= WorkspaceGroup_sptr(new WorkspaceGroup);
								bnewGoup1=false;
							}
						setOutputWSProperties(alg,*itr,nPeriod,sptrWSGrp1,outWSParentName);
					}
					if(outWSCount==2)
					{	if( bnewGoup2){sptrWSGrp2= WorkspaceGroup_sptr(new WorkspaceGroup);
						bnewGoup2=false;
					}
						setOutputWSProperties(alg,*itr,nPeriod,sptrWSGrp2,outWSParentName);
					}
																		
				}

			}
			else
			{
				alg->setPropertyValue((*itr)->name(),(*itr)->value());
			}
		}
		// execute the algorithm 
		bStatus=alg->execute();
		// status of eac execution is checking 
		bgroupExecStatus=bgroupExecStatus&&bStatus;
		bgroupFailed=bgroupFailed||bStatus;
		execPercentage+=10;
		progress(double((execPercentage)/execTotal));
		//if a workspace execution fails
		if(!bStatus)
		{  	g_log.error()<<"Algorithm execution failed for the input workspace "<<(*wsItr)<<std::endl;
		}
		//increment count for outworkpsace name
		nPeriod++;

	}//end of for loop for input workspace group
	//if all passed 
	if(bgroupExecStatus)
	{setExecuted(true);
	}
	//if all failed
	if(!bgroupFailed)
	{
		// remove the group parent  from the ADS - bcoz only group parent will come in mantid plot
		AnalysisDataService::Instance().remove(outWSParentName);
	}

	m_notificationCenter.postNotification(new FinishedNotification(this,isExecuted()));
	return bStatus;
}
 
void Algorithm::setInputWSProperties(IAlgorithm* pAlg,Mantid::Kernel::Property* prop,const std::string&inputWS )
{
	std::string wsname=prop->value();
	try
	{
		boost::shared_ptr<WorkspaceGroup> wsGrpSptr =
			boost::dynamic_pointer_cast<WorkspaceGroup>(AnalysisDataService::Instance().retrieve(wsname));
		if(wsGrpSptr)
		{
			pAlg->setPropertyValue(prop->name(), inputWS);
		}
		else
		{
			pAlg->setPropertyValue(prop->name(), wsname);
		}
	}
	catch(Mantid::Kernel::Exception::NotFoundError &e)//if not a valid object in analysis data service
	{
		g_log.information()<<" Trying to retrieve  Object "<< wsname<<" which is not there in ADS"<<std::endl;

	}
}
void Algorithm::setOutputWSProperties(IAlgorithm* pAlg,Mantid::Kernel::Property*prop,const int nPeriod,WorkspaceGroup_sptr sptrWSGrp,std::string &outWSParentName)
{
	std::string outWSChildName("");
	outWSParentName=prop->value();
	//std::string outWSParentName=prop->value();
	std::stringstream suffix;
	suffix<<nPeriod;
	outWSChildName=outWSParentName+"_"+suffix.str();
	pAlg->setPropertyValue(prop->name(), outWSChildName);
	if(nPeriod==1){
		if(sptrWSGrp)sptrWSGrp->add(outWSParentName);
		AnalysisDataService::Instance().addOrReplace(outWSParentName,sptrWSGrp );
	}
	//adding to wsgroup vector
	if(sptrWSGrp)
	{
		g_log.information()<< outWSChildName<<" adding to group"<<std::endl;
		sptrWSGrp->add(outWSChildName);
	}
}
bool Algorithm::isWorkspaceProperty( Mantid::Kernel::Property* prop)
{
	const IWorkspaceProperty *wsProp = dynamic_cast<IWorkspaceProperty*>(prop);
	bool bStatus(false);
	(wsProp)? (bStatus=true) :( bStatus= false);
	return bStatus;
	
}

bool Algorithm::isInputWorkspaceProperty( Mantid::Kernel::Property* prop)
{
	const Property *wsPropProp = dynamic_cast<Property*>(prop);
	unsigned int direction = wsPropProp->direction();
	if (direction == Kernel::Direction::Input || direction==Kernel::Direction::InOut)
	{
		return true;
	}
	else return false;
}
bool Algorithm::isOutputWorkspaceProperty( Mantid::Kernel::Property* prop)
{
	const Property *wsPropProp = dynamic_cast<Property*>(prop);
	unsigned int direction = wsPropProp->direction();
	if (direction == Kernel::Direction::Output || direction==Kernel::Direction::InOut)
	{
		return true;
	}
	else return false;
}
bool Algorithm::executeAsyncImpl(const int&)
{	try
	{
		m_runningAsync = true;
		bool res = execute();
		m_runningAsync = false;

		return res;
	}
    catch(...)
    { }
    return false;
}

/** Handles and rescales child algorithm progress notifications.
 *  @param pNf The progress notification from the child algorithm.
 */
void Algorithm::handleChildProgressNotification(const Poco::AutoPtr<ProgressNotification>& pNf)
{
    double p = m_startChildProgress + (m_endChildProgress - m_startChildProgress)*pNf->progress;
    progress(p,pNf->message);
}

} // namespace API
} // namespace Mantid
