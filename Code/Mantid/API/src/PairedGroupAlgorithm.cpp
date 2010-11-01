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
    *  @param props a vector holding properties
    *  @retval false if  selected workspace groups sizes not match
    */
    bool PairedGroupAlgorithm::processGroups(WorkspaceGroup_sptr inputWSGrp,const std::vector<Mantid::Kernel::Property*>&props)
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
      //get the lhs and rhs group vectors from properties
      getGroupNames(props,lhsWSGrp,rhsWSGrp);
      //check lhs and o/p workspaces are same
      bool blhsEqual=isOutputequaltoLHS(props);
      //check rhs and o/p workspace are same
      bool brhsEqual=isOutputequaltoRHS(props);
   
      std::string lhswsName;
      std::string rhswsName;
      std::string outputwsName;
      getlhsandrhsworkspace(props,lhswsName,rhswsName,outputwsName);
      //check the workspaces are of similar names (similar if they are of names like  group_1,group_2)
      bool blhsSimilar=isGroupWorkspacesofSimilarNames(lhswsName,lhsWSGrp);
      bool brhsSimilar=isGroupWorkspacesofSimilarNames(rhswsName,rhsWSGrp);
      bool bSimilarNames(false);
          
      //get member from  each group and call binary execute on each member
      IAlgorithm* alg = API::FrameworkManager::Instance().createAlgorithm(this->name(), this->version());
      if(!alg)
      {
        g_log.error()<<"createAlgorithm failed for  "<<this->name()<<"("<<this->version()<<")"<<std::endl;
        throw std::runtime_error("algorithm execution failed ");
      }
      std::vector<std::string>::const_iterator lhsItr=lhsWSGrp.begin(); 
      std::vector<std::string>::const_iterator rhsItr=rhsWSGrp.begin(); 
      int nPeriod=0;
      bool bStatus=0;

      if(lhsWSGrp.size()>1 && rhsWSGrp.size()>1)
      {
        if(isCompatibleSizes(lhsWSGrp,rhsWSGrp))
        {
          bSimilarNames=blhsSimilar&&brhsSimilar; 
          for (;lhsItr!=lhsWSGrp.end();++lhsItr,++rhsItr)
          {
            ++nPeriod;
            // Create a new instance of the algorithm for each group member (needed if execute creates new properties)
           // alg = API::FrameworkManager::Instance().createAlgorithm(this->name(), this->version());
            setProperties(alg,props,(*lhsItr),(*rhsItr),nPeriod,outWSGrp,blhsEqual,brhsEqual,bSimilarNames);
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
        if(brhsSimilar)
        {         
          if(std::find(rhsWSGrp.begin(),rhsWSGrp.end(),lhswsName)!=rhsWSGrp.end())
          {
            bSimilarNames=true;
          }
          
        }
      
        for (;rhsItr!=rhsWSGrp.end();rhsItr++)
        {	++nPeriod;
        setProperties(alg,props,(*lhsItr),(*rhsItr),nPeriod,outWSGrp,blhsEqual,brhsEqual,bSimilarNames);
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
        if(blhsSimilar)
        {         
          if(std::find(lhsWSGrp.begin(),lhsWSGrp.end(),rhswsName)!=lhsWSGrp.end())
          {
            bSimilarNames=true;
          }
         
        }
        for (;lhsItr!=lhsWSGrp.end();lhsItr++)
        {	++nPeriod;
        setProperties(alg,props,(*lhsItr),(*rhsItr),nPeriod,outWSGrp,blhsEqual,brhsEqual,bSimilarNames);
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
    *  @param props a vector holding properties
    *  @param lhswsName name of the LHS workspace
    *  @param rhswsName name of the RHS workspace
    *  @param nPeriod period number
    *  @param outWSGrp shared pointer to output workspace
    *  @param lhsEqual true if lhs and output ws are of same names
    *  @param rhsEqual true if rhs and output ws are of same names
    *  @param bSimilarNames  if workspaces are of similar names
    */
    void PairedGroupAlgorithm::setProperties(IAlgorithm* alg,const std::vector<Kernel::Property*>&props,
      const std::string& lhswsName,const std::string& rhswsName,int nPeriod,WorkspaceGroup_sptr outWSGrp,bool lhsEqual,bool rhsEqual,bool bSimilarNames)
    {
      std::string prevPropName("");
      std::string currentPropName("");

      std::vector<Mantid::Kernel::Property*>::const_iterator propItr=props.begin();
      for (propItr=props.begin();propItr!=props.end();propItr++)
      {
        if(isWorkspaceProperty(*propItr))
        {
          if(isInputWorkspaceProperty(*propItr))
          {
            currentPropName=(*propItr)->name();
            if(prevPropName.empty())
            {	
              try
              {//LHS workspace 
                alg->setPropertyValue(currentPropName,lhswsName);
              }
              catch(std::invalid_argument& )
              {
                throw std::runtime_error("Workspace named \"" + lhswsName+ "\"can not be found");
              }
            }
            else
            {	// RHS workspace 
              if(currentPropName.compare(prevPropName))
              {  
                try
                {
                  alg->setPropertyValue(currentPropName,rhswsName);
                }
                catch(std::invalid_argument& )
                {
                  throw std::runtime_error("Workspace named \"" + rhswsName+ "\"can not be found");
                }
              }
            }
            prevPropName=currentPropName;
          }//end of if loop for input workspace property
          if(isOutputWorkspaceProperty(*propItr))
          {
            std::string outmemberwsName;
            std::string outgroupwsName=(*propItr)->value();
            if(lhsEqual)
            {
              outmemberwsName=lhswsName;
            }
            else if(rhsEqual)
            {
              outmemberwsName=rhswsName;
            }
            else
            {
              if(bSimilarNames)
              { 
                std::stringstream speriodNum;
                speriodNum<<nPeriod;
                outmemberwsName=outgroupwsName+"_"+speriodNum.str();
              }
              else
              {
                outmemberwsName=lhswsName+"_"+rhswsName+"_"+outgroupwsName;
              }
             
            }
            try
            {
            alg->setPropertyValue((*propItr)->name(),outmemberwsName);
            }
            catch(std::invalid_argument& )
            {
              throw std::runtime_error("Workspace named \"" + outmemberwsName+ "\"can not be found");
            }
            if(nPeriod==1){
              AnalysisDataService::Instance().addOrReplace(outgroupwsName,outWSGrp );
            }
            if(outWSGrp)
            {
              outWSGrp->add(outmemberwsName);
            }
          }//end of if loop for output workspace property
        }//end of if loop for workspace property
        else
        {
          try
          {
          alg->setPropertyValue((*propItr)->name(),(*propItr)->name());
          }
          catch(std::invalid_argument& )
          {
            throw std::runtime_error("Workspace named \"" + (*propItr)->name()+ "\"can not be found");
          }
        }
      }//end of for loop for property vector

    }

    /**This method checks input and output groupworkspace for an algorithm is of same name.
    *  @param props a list of properties for the algorithm
    *  @returns true if the input and output groupworkspaces are of same names
    */ 
    bool PairedGroupAlgorithm::isOutputequaltoLHS(const std::vector<Mantid::Kernel::Property*>& props)
    {
      std::string outputwsName;
      std::string rhswsName;
      std::string lhswsName;
      getlhsandrhsworkspace(props,lhswsName,rhswsName,outputwsName);
      return(!lhswsName.compare(outputwsName) ? true:false);
    }
    /**This method checks input and output groupworkspace for an algorithm is of same name.
    *  @param props a list of properties for the algorithm
    *  @returns true if the input and output groupworkspaces are of same names
    */ 
    bool PairedGroupAlgorithm::isOutputequaltoRHS(const std::vector<Mantid::Kernel::Property*>& props)
    {     
      std::string outputwsName;
      std::string rhswsName;
      std::string lhswsName;
      getlhsandrhsworkspace(props,lhswsName,rhswsName,outputwsName);
      return(!rhswsName.compare(outputwsName) ? true:false);
    }
  /**This method checks input and output groupworkspace for an algorithm is of same name.
    *  @param props a list of properties for the algorithm
    *  @param lhswsName name of lhs workspace
    *  @param rhswsName name of rhs workspace
    *  @param outputwsName name of output workspace
    */ 
    void PairedGroupAlgorithm::getlhsandrhsworkspace(const std::vector<Mantid::Kernel::Property*>& props,std::string& lhswsName,
                                          std::string& rhswsName,std::string& outputwsName )
    {
      std::vector<Mantid::Kernel::Property*>::const_iterator citr;
      for(citr=props.begin();citr!=props.end();++citr)
      {

        if(isInputWorkspaceProperty(*citr))
        {
          if(lhswsName.empty())
          {
            lhswsName=(*citr)->value();
          }
          else
          {
            rhswsName=(*citr)->value();
          }
           
        }
        if(isOutputWorkspaceProperty(*citr))
        {
          outputwsName=(*citr)->value();
        }

      }
    }
/**This method checks the member workspace are of similar names in a group workspace .
  *  @param ingroupwsName input group workspace name
  *  @param grpmembersNames a list of group member names
  *  @returns true if workspaces are of similar names
 */ 
bool PairedGroupAlgorithm::isGroupWorkspacesofSimilarNames(const std::string& ingroupwsName,const std::vector<std::string>& grpmembersNames)
{
   if(grpmembersNames.empty()) return false;
   
   bool bsimilar(true);
   //check all the members are of similar names
   std::vector<std::string>::const_iterator citr;
   for(citr=grpmembersNames.begin();citr!=grpmembersNames.end();++citr)
   {
       bool b;
       std::size_t pos=(*citr).find_last_of("_");
       if(pos==std::string::npos)
       {
         b=false;
       }
       else
       {
         //get the common part of member ws names i.e "group_1","group_2" are two group members common part is "group"
         std::string memmberwscommonpart((*citr).substr(0,pos));
         if(!ingroupwsName.compare(memmberwscommonpart))
         {
           b=true;

         }
         else
         {
           b=false;
         }
       }
       bsimilar= bsimilar&&b;
   }
   return(bsimilar?true:false);
   
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
              {
                lhsWSGrpNames=wsGrpSptr->getNames();
              }
              else
              {
                lhsWSGrpNames.push_back(wsName);
              }
            }
            else
            {	
              if(currentPropName.compare(prevPropName))
              { //second workspace("RHSWorkSpace") 
                if(wsGrpSptr)
                {
                  rhsWSGrpNames=wsGrpSptr->getNames();
                }
                else
                {
                  rhsWSGrpNames.push_back(wsName);
                }
              }
            }
            prevPropName=currentPropName;
          }//end of if loop for input workspace property iteration
        }//end of if loop for workspace property iteration
      } //end of for loop for property iteration
    }

  } // namespace API
} // namespace Mantid
