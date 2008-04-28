//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <cmath>
#include "MantidAlgorithms/BinaryOperation.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceIterator.h" 

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace Mantid
{
  namespace Algorithms
  {
    // Get a reference to the logger
    Logger& BinaryOperation::g_log = Logger::get("BinaryOperation");

    /** Initialisation method. 
    * Defines input and output workspaces
    * 
    */
    void BinaryOperation::init()
    {
      declareProperty(new WorkspaceProperty<Workspace>("InputWorkspace_1","",Direction::Input));
      declareProperty(new WorkspaceProperty<Workspace>("InputWorkspace_2","",Direction::Input));
      declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace","",Direction::Output));    
    }

    /** Executes the algorithm
    * 
    *  @throw runtime_error Thrown if algorithm cannot execute
    */
    void BinaryOperation::exec()
    {
      // get input workspace, dynamic cast not needed
      Workspace_sptr in_work1 = getProperty("InputWorkspace_1");
      Workspace_sptr in_work2 = getProperty("InputWorkspace_2");

      if (!checkSizeCompatability(in_work1,in_work2))
      {
        throw std::invalid_argument("The size of the two workspaces are not compatible for algorithm plus"  );
      }

      Workspace::const_iterator ti_in1 = createConstIterator(in_work1,in_work2);
      Workspace::const_iterator ti_in2 = createConstIterator(in_work2,in_work1);
      
      Workspace_sptr out_work = createOutputWorkspace(in_work1,in_work2);
      Workspace::iterator ti_out(*out_work);

      //perform the operation through an abstract call
      performBinaryOperation(ti_in1,ti_in2,ti_out);

      // Assign it to the output workspace property
      setProperty("OutputWorkspace",out_work);

      return;
    }

    /** Performs a simple check to see if the sizes of two workspaces are compatible for a binary operation
    * In order to be size compatible then the larger workspace 
    * must divide be the size of the smaller workspace leaving no remainder
    * @param lhs the first workspace to compare
    * @param rhs the second workspace to compare
    * @retval true The two workspaces are size compatible
    * @retval false The two workspaces are NOT size compatible
    */
    const bool BinaryOperation::checkSizeCompatability(const API::Workspace_sptr lhs,const API::Workspace_sptr rhs) const
    {
      //in order to be size compatible then the larger workspace 
      //must divide be the size of the smaller workspace leaving no remainder
      if (rhs->size() ==0) return false;
      return ((lhs->size() % rhs->size()) == 0);
    }

    /** Performs a simple check to see if the X arrays of two workspaces are compatible for a binary operation
    * The X arrays of two workspaces must be identical to allow a binary operation to be performed
    * @param lhs the first workspace to compare
    * @param rhs the second workspace to compare
    * @retval true The two workspaces are size compatible
    * @retval false The two workspaces are NOT size compatible
    */
    const bool BinaryOperation::checkXarrayCompatability(const API::Workspace_sptr lhs,const API::Workspace_sptr rhs) const
    {
      const std::vector<double>& w1x = lhs->dataX(0);
      const std::vector<double>& w2x = rhs->dataX(0);

      double sum;
      sum=0.0;
      for (unsigned int i=0; i < w1x.size(); i++) sum += fabs(w1x[i]-w2x[i]);
      if( sum < 0.0000001)
        return true;
      else
        return false;
    }

    /** Gets the number of time an iterator over the first workspace would have to loop to perform a full iteration of the second workspace
    * @param lhs the first workspace to compare
    * @param rhs the second workspace to compare
    * @returns Integer division of rhs.size()/lhs.size() with a minimum of 1
    */
    const int BinaryOperation::getRelativeLoopCount(const API::Workspace_sptr lhs, const API::Workspace_sptr rhs) const
    {
      int lhsSize = lhs->size();
      if (lhsSize == 0) return 1;
      int retVal = rhs->size()/lhsSize;
      return (retVal == 0)?1:retVal;
    }
  


    /** Creates a suitable output workspace for a binary operatiion based on the two input workspaces
    * @param lhs the first workspace to compare
    * @param rhs the second workspace to compare
    * @returns a pointer to a new zero filled workspace the same type and size as the larger of the two input workspaces.
    */
    API::Workspace_sptr BinaryOperation::createOutputWorkspace(const API::Workspace_sptr lhs, const API::Workspace_sptr rhs) const
    {       
      //get the largest workspace
      const API::Workspace_sptr wsLarger = (lhs->size() > rhs->size()) ? lhs : rhs;
      //create a new workspace
      API::Workspace_sptr retVal = API::WorkspaceFactory::Instance().create(wsLarger);

      return retVal;
    }

    /** Creates a const iterator taking into account loop
    * @param wsMain The workspace theat the iterator will be created for
    * @param wsComparison The workspace to be used for axes comparisons
    * @returns a const iterator to wsMain, with loop count and orientation set appropriately
    */
    Workspace::const_iterator BinaryOperation::createConstIterator(const API::Workspace_sptr wsMain, const API::Workspace_sptr wsComparison) const
    {
     //get loop count
      unsigned int loopDirection = LoopOrientation::Vertical;
      int loopCount = getRelativeLoopCount(wsMain,wsComparison);
      if (loopCount > 1)
      {
        loopDirection = getLoopDirection(wsMain,wsComparison);
      }
      else
      {
        if (!checkXarrayCompatability(wsMain,wsComparison))
        {
          g_log.error("The x arrays of the workspaces are not identical");
          throw std::invalid_argument("The x arrays of the workspaces are not identical");
        }
      }
      Workspace::const_iterator it(*wsMain,loopCount,loopDirection);
      return it;
    }

    /** Determines the required loop direction for a looping iterator
    * @param wsMain The workspace theat the iterator will be created for
    * @param wsComparison The workspace to be used for axes comparisons
    * @returns An value describing the orientation of the 1D workspace to be looped
    * @retval 0 Horizontal - The number and contents of the X axis bins match
    * @retval 1 Vertical - The number of detector elements match
    */
    unsigned int BinaryOperation::getLoopDirection(const API::Workspace_sptr wsMain, const API::Workspace_sptr wsComparison) const
    {
      unsigned int retVal = LoopOrientation::Horizontal;
      
      //check if the vertical sizes match
      int wsMainArraySize = wsMain->size(); //this must be a 1D array for this to work
      int wsComparisonArraySize = wsComparison->size()/wsComparison->blocksize();
      if (wsMainArraySize == wsComparisonArraySize)
      {
        retVal = LoopOrientation::Vertical;
      }
      //check if Horizontial looping matches in length
      if (wsMain->blocksize() == wsComparison->blocksize())
      {
        //it does, now check if the X arrays are compatible
        if (!checkXarrayCompatability(wsMain,wsComparison))
        {
          if(retVal == LoopOrientation::Horizontal)
          {
            //this is a problem, the lengths only match Horizontally but the data does not match
            g_log.error("The x arrays of the workspaces are not identical");
            throw std::invalid_argument("The x arrays of the workspaces are not identical");
          }
        }
        else 
        {
          //all is good in the world
          retVal = LoopOrientation::Horizontal;
        }
      }

      return retVal;
    }
    
  }
}
