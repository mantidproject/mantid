//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <limits>
#include "MantidAlgorithms/BinaryOpHelper.h"
#include "MantidDataObjects/Workspace1D.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/WorkspaceFactory.h" 
#include "MantidKernel/Exception.h" 

using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;

namespace Mantid
{
  namespace Algorithms
  {
    // Get a reference to the logger
    Logger& BinaryOpHelper::g_log = Logger::get("binaryOpHelper");

    /** Performs a simple check to see if the sizes of two workspaces are compatible for a binary operation
    * In order to be size compatible then the larger workspace 
    * must divide be the size of the smaller workspace leaving no remainder
    * @param ws1 the first workspace to compare
    * @param ws2 the second workspace to compare
    * @retval true The two workspaces are size compatible
    * @retval false The two workspaces are NOT size compatible
    */
    const bool BinaryOpHelper::checkSizeCompatability(const API::Workspace_sptr ws1,const API::Workspace_sptr ws2) const
    {
      //get the largest workspace
      API::Workspace_sptr wsLarger;
      API::Workspace_sptr wsSmaller;
      if (ws1->size() > ws2->size())
      {
        wsLarger = ws1;
        wsSmaller = ws2;
      }
      else
      {
        wsLarger = ws2;
        wsSmaller = ws1;
      }
      //in order to be size compatible then the larger workspace 
      //must divide be the size of the smaller workspace leaving no remainder
      if (wsSmaller->size() ==0) return false;
      return ((wsLarger->size() % wsSmaller->size()) == 0);
    }

    /** Performs a simple check to see if the X arrays of two workspaces are compatible for a binary operation
    * The X arrays of two workspaces must be identical to allow a binary operation to be performed
    * @param ws1 the first workspace to compare
    * @param ws2 the second workspace to compare
    * @retval true The two workspaces are size compatible
    * @retval false The two workspaces are NOT size compatible
    */
    const bool BinaryOpHelper::checkXarrayCompatability(const API::Workspace_sptr ws1,const API::Workspace_sptr ws2) const
    {
      const std::vector<double>& w1x = ws1->dataX(1);
      const std::vector<double>& w2x = ws2->dataX(1);

      double sum;
      sum=0.0;
      for (int i=0; i < w1x.size(); i++) sum += fabs(w1x[i]-w2x[i]);
      if( sum < 0.0000001)
        return true;
      else
        return false;
    }


    /** Creates a suitable output workspace for a binary operatiion based on the two input workspaces
    * @param ws1 the first workspace to compare
    * @param ws2 the second workspace to compare
    * @returns a pointer to a new zero filled workspace the same type and size as the larger of the two input workspaces.
    */
    API::Workspace_sptr BinaryOpHelper::createOutputWorkspace(const API::Workspace_sptr ws1, const API::Workspace_sptr ws2) const
    {
      const double initialValue=  std::numeric_limits<double>::epsilon();
          
      //get the largest workspace
      const API::Workspace_sptr wsLarger = (ws1->size() > ws2->size()) ? ws1 : ws2;
      //create a new workspace
      API::Workspace_sptr retVal = (API::WorkspaceFactory::Instance()->create(wsLarger->id()));
      //this needs to be set to the size of the larger workspace and 0 filled
      Workspace1D* ws1D = dynamic_cast<Workspace1D*>(retVal.get());
      if (ws1D != 0)
      {    
        //do ws1d things
        std::vector<double> x(wsLarger->size(),initialValue),sig(wsLarger->size(),initialValue),err(wsLarger->size(),initialValue);
        ws1D->setData(sig,err);
        ws1D->setX(x);
      }
      else
      {
        Workspace2D* ws2D = dynamic_cast<Workspace2D*>(retVal.get());   
        if (ws2D != 0)
        {
          //do ws2d things
          std::vector<double> x(wsLarger->blocksize(),initialValue),y(wsLarger->blocksize(),initialValue),e(wsLarger->blocksize(),initialValue);
          int len=wsLarger->size()/wsLarger->blocksize();
          ws2D->setHistogramNumber(len);

          for (int i = 0; i < len; i++)
          {
            ws2D->setX(i,x);
            ws2D->setData(i,y,e);
          }
        }
      }
      return retVal;
    }
  }
}
