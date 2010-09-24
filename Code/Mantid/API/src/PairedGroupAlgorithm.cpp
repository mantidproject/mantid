//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/PairedGroupAlgorithm.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/FrameworkManager.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace Mantid
{
  namespace API
  {
    PairedGroupAlgorithm::PairedGroupAlgorithm() : API::Algorithm(), m_progress(NULL) {}

    PairedGroupAlgorithm::~PairedGroupAlgorithm()
    {
      if (m_progress) delete m_progress;
    }


    /** This method is called if one of the selected workspaces for binary operation is a workspacegroup
    *  @param inputWSGrp pointer to the first workspace group
    *  @param prop a vector holding properties
    *  @retval false if  selected workspace groups sizes not match
    */
    bool PairedGroupAlgorithm::processGroups(WorkspaceGroup_sptr inputWSGrp,const std::vector<Mantid::Kernel::Property*>&prop)
    {

      //getting the input workspace group names
      const std::vector<std::string> inputWSNames=inputWSGrp->getNames();
      int nSize=inputWSNames.size();
      //return if atleast one member is not there in the group to process
      if(nSize<1)
      {
        throw std::runtime_error("Input WorkspaceGroup has no members to process");
      }
      std::vector<std::string> lhsWSGrp;
      std::vector<std::string> rhsWSGrp;
      bool bgroupExecStatus=true;
      //flag used for checking failure of  all memebrs of the group
      bool bgroupFailed=false;
      WorkspaceGroup_sptr outWSGrp= WorkspaceGroup_sptr(new WorkspaceGroup);

      getGroupNames(prop,lhsWSGrp,rhsWSGrp);
      //get member from  each group and call binary execute on each member
      IAlgorithm* alg = API::FrameworkManager::Instance().createAlgorithm(this->name(), this->version());
      if(!alg)
      {
        g_log.error()<<"createAlgorithm failed for  "<<this->name()<<"("<<this->version()<<")"<<std::endl;
        throw std::runtime_error("algorithm execution failed ");
        // return false;
      }
      std::vector<std::string>::const_iterator lhsItr=lhsWSGrp.begin(); 
      std::vector<std::string>::const_iterator rhsItr=rhsWSGrp.begin(); 
      int nPeriod=0;
      bool bStatus=0;

      if(lhsWSGrp.size()>1 && rhsWSGrp.size()>1)
      {
        if(isCompatibleSizes(lhsWSGrp,rhsWSGrp))
        {
          for (;lhsItr!=lhsWSGrp.end();++lhsItr,++rhsItr)
          {
            ++nPeriod;
            // Create a new instance of the algorithm for each group member (needed if execute creates new properties)
            alg = API::FrameworkManager::Instance().createAlgorithm(this->name(), this->version());
            setProperties(alg,prop,(*lhsItr),(*rhsItr),nPeriod,outWSGrp);
            bStatus=alg->execute();
            if(!bStatus)
            {
              throw std::runtime_error("Execution failed for the algorithm "+this->name());
            }
            bgroupExecStatus=bgroupExecStatus&bStatus;
            bgroupFailed=bgroupFailed || bStatus;
          }
        }
      }
      else if(lhsWSGrp.size()==1)//if LHS is not a group workspace and RHS is group workspace
      {				
        for (;rhsItr!=rhsWSGrp.end();rhsItr++)
        {	++nPeriod;
        setProperties(alg,prop,(*lhsItr),(*rhsItr),nPeriod,outWSGrp);
        bStatus=alg->execute();
        if(!bStatus)
        {
          throw std::runtime_error("Execution failed for the algorithm "+this->name());
        }
        bgroupExecStatus=bgroupExecStatus&bStatus;
        bgroupFailed=bgroupFailed || bStatus;
        }
      }
      else if (rhsWSGrp.size()==1)//if RHS is not a group workspace and LHS is a group workspace
      {				
        for (;lhsItr!=lhsWSGrp.end();lhsItr++)
        {	++nPeriod;
        setProperties(alg,prop,(*lhsItr),(*rhsItr),nPeriod,outWSGrp);
        bStatus=alg->execute();
        if(!bStatus)
        {
          throw std::runtime_error("Execution failed for the algorithm "+this->name());
        }
        bgroupExecStatus=bgroupExecStatus && bStatus;
        bgroupFailed=bgroupFailed || bStatus;
        }
      }

      if(bgroupExecStatus)
	  {
        setExecuted(true);
	  }
     

      m_notificationCenter.postNotification(new FinishedNotification(this,this->isExecuted()));
      return bStatus;
    }

    /** This method sets properties for the algorithm
    *  @param alg pointer to the algorithm
    *  @param prop a vector holding properties
    *  @param lhsWSName name of the LHS workspace
    *  @param rhsWSName name of the RHS workspace
    *  @param nPeriod period number
    *  @param outWSGrp shared pointer to output workspace
    */
    void PairedGroupAlgorithm::setProperties(IAlgorithm* alg,const std::vector<Kernel::Property*>&prop,
      const std::string& lhsWSName,const std::string& rhsWSName,int nPeriod,WorkspaceGroup_sptr outWSGrp)
    {
      std::string prevPropName("");
      std::string currentPropName("");

      std::vector<Mantid::Kernel::Property*>::const_iterator propItr=prop.begin();
      for (propItr=prop.begin();propItr!=prop.end();propItr++)
      {
        if(isWorkspaceProperty(*propItr))
        {
          if(isInputWorkspaceProperty(*propItr))
          {
            currentPropName=(*propItr)->name();
            if(prevPropName.empty())
            {	//LHS workspace 
              alg->setPropertyValue(currentPropName,lhsWSName);
            }
            else
            {	// RHS workspace 
              if(currentPropName.compare(prevPropName))
              { alg->setPropertyValue(currentPropName,rhsWSName);
              }
            }
            prevPropName=currentPropName;
          }//end of if loop for input workspace property
          if(isOutputWorkspaceProperty(*propItr))
          {
            std::string str=(*propItr)->value();
            std::stringstream speriodNum;
            speriodNum<<nPeriod;
            std::string outWSName=str+"_"+speriodNum.str();
            alg->setPropertyValue((*propItr)->name(),outWSName);
            if(nPeriod==1){
              AnalysisDataService::Instance().addOrReplace(str,outWSGrp );
            }
            if(outWSGrp)
              outWSGrp->add(outWSName);
          }//end of if loop for output workspace property
        }//end of if loop for workspace property
        else
        {
          alg->setPropertyValue((*propItr)->name(),(*propItr)->name());
        }
      }//end of for loop for property vector

    }

    /** This method checks both LHS and RHS workspaces are of same size
    *  @param lhsWSGrpNames a vector holding names of LHS Workspace
    *  @param rhsWSGrpNames a vector holding names of RHS Workspace
    *  @retval true if  selected workspace groups are of same size
    *  @retval false if  selected workspace groups sizes not match
    */
    bool PairedGroupAlgorithm::isCompatibleSizes(const std::vector<std::string> &lhsWSGrpNames, const std::vector<std::string> &rhsWSGrpNames) const
    {
      //if both lhs and rhs workspaces are group workspaces ,check it's size
      if(lhsWSGrpNames.size()!=rhsWSGrpNames.size())
      {
        g_log.error("Selected workspace groups are not of same size.");
        return false;
      }
      return true;
    }

    /** This method iterates through property vector and returns  LHS and RHS workspaces group names vectors
    *  @param prop  vector holding the properties
    *  @param lhsWSGrpNames a vector holding names of LHS Workspace
    *  @param rhsWSGrpNames a vector holding names of RHS Workspace
    */
    void PairedGroupAlgorithm::getGroupNames(const std::vector<Kernel::Property*>&prop, 
      std::vector<std::string> &lhsWSGrpNames, std::vector<std::string> &rhsWSGrpNames) const
    {
      std::vector<Mantid::Kernel::Property*>::const_iterator itr;
      std::string prevPropName("");
      std::string currentPropName("");
      for (itr=prop.begin();itr!=prop.end();itr++)
      {	
        if(isWorkspaceProperty(*itr))
        {
          if(isInputWorkspaceProperty(*itr))
          {
            currentPropName=(*itr)->name();
            std::string wsName=(*itr)->value();
			Workspace_sptr wsPtr;
			try
			{
             wsPtr=AnalysisDataService::Instance().retrieve(wsName);
			}
			catch(Kernel::Exception::NotFoundError&)
			{
				throw std::runtime_error("Workspace "+ wsName+ "not loaded");
			}
            WorkspaceGroup_sptr wsGrpSptr=boost::dynamic_pointer_cast<WorkspaceGroup>(wsPtr);
            if(prevPropName.empty())
            {	//LHSWorkspace 
              if(wsGrpSptr)
                lhsWSGrpNames=wsGrpSptr->getNames();
              else
                lhsWSGrpNames.push_back(wsName);
            }
            else
            {	
              if(currentPropName.compare(prevPropName))
              { //second workspace("RHSWorkSpace") 
                if(wsGrpSptr)
                  rhsWSGrpNames=wsGrpSptr->getNames();
                else
                  rhsWSGrpNames.push_back(wsName);
              }
            }
            prevPropName=currentPropName;
          }//end of if loop for input workspace property iteration
        }//end of if loop for workspace property iteration
      } //end of for loop for property iteration
    }

  } // namespace API
} // namespace Mantid
