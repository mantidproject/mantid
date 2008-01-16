//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <cmath>
#include "MantidAlgorithms/Divide.h"
#include "MantidAlgorithms/BinaryOpHelper.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/Exception.h" 
#include "MantidAPI/TripleIterator.h" 

// Register the class into the algorithm factory
DECLARE_NAMESPACED_ALGORITHM(Mantid::Algorithms,Divide)
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace Mantid
{
  namespace Algorithms
  {
    // Get a reference to the logger
    Logger& Divide::g_log = Logger::get("Divide");

    /** Initialisation method. 
    * Defines input and output workspaces
    * 
    */
    void Divide::init()
    {
      declareProperty(new WorkspaceProperty<Workspace>("InputWorkspace_1","",Direction::Input));
      declareProperty(new WorkspaceProperty<Workspace>("InputWorkspace_2","",Direction::Input));
      declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace","",Direction::Output));    
    }

    /** Executes the algorithm
    * 
    *  @throw runtime_error Thrown if algorithm cannot execute
    */
    void Divide::exec()
    {
      // get input workspace, dynamic cast not needed
      Workspace_sptr in_work1 = getProperty("InputWorkspace_1");
      Workspace_sptr in_work2 = getProperty("InputWorkspace_2");

      //create a BinaryOpHelper
      BinaryOpHelper boHelper;
      if (!boHelper.checkSizeCompatability(in_work1,in_work2))
      {
        throw std::invalid_argument("The size of the two workspaces are not compatible for algorithm plus"  );
      }

      if (!boHelper.checkXarrayCompatability(in_work1,in_work2))
      {
        g_log.error("The x arrays of the workspaces are not identical");
        throw std::invalid_argument("The x arrays of the workspaces are not identical");
      }

      Workspace_sptr out_work = boHelper.createOutputWorkspace(in_work1,in_work2);

      triple_iterator<Workspace> ti_out(*out_work);
      triple_iterator<Workspace> ti_in1(*in_work1);
      triple_iterator<Workspace> ti_in2(*in_work2);
      std::transform(ti_in1.begin(),ti_in1.end(),ti_in2.begin(),ti_out.begin(),Divide_fn());

      // Assign it to the output workspace property
      setProperty("OutputWorkspace",out_work);

      return;
    }


    /** Performs the Division with Gausian errors within the transform function
    * @param a The triple ref of the first workspace data item
    * @param b The triple ref of the second workspace data item
    * @returns A triple ref of the result with Gausian errors
    */
    TripleRef<double>
      Divide::Divide_fn::operator() (const TripleRef<double>& a,const TripleRef<double>& b) 
    {          
      xvalue=a[0];
      ret_sig=a[1]/b[1];
      //gaussian errors for the moment
      // (Sa/a)2 + (Sb/b)2 = (Sc/c)2 
      //  So after taking proportions, squaring, summing, 
      //  and taking the square root, you get a proportional error to the product c. Multiply that proportional error by c to get the actual standard deviation Sc. 
      ret_err=ret_sig*sqrt(pow((a[2]/a[1]),2) + pow((b[2]/b[1]),2));    
      return TripleRef<double>(xvalue,ret_sig,ret_err);      
    }
  }
}
