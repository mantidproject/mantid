//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <cmath>
#include "MantidAlgorithms/Divide.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/Exception.h" 
#include "MantidAPI/TripleIterator.h" 

// Register the class into the algorithm factory
DECLARE_NAMESPACED_ALGORITHM(Mantid::Algorithms,Divide)
using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace Mantid
{
  namespace Algorithms
  {
    // Get a reference to the logger
    Logger& Divide::g_log = Logger::get("Divide");

    /** Performs the divide operation using Iterators and the std::tranform function.
    * @param it_in1 The const iterator to the lhs data item
    * @param it_in2 The const iterator to the rhs data item
    * @param it_out The output iterator to the new workspace
    */
    void Divide::performBinaryOperation(API::Workspace::const_iterator it_in1, API::Workspace::const_iterator it_in2,
        API::Workspace::iterator it_out)
    {
      std::transform(it_in1.begin(),it_in1.end(),it_in2.begin(),it_out.begin(),Divide_fn());
    }


    /** Performs the Division with Gausian errors within the transform function
    * @param a The triple ref of the first workspace data item
    * @param b The triple ref of the second workspace data item
    * @returns A triple ref of the result with Gausian errors
    */
    TripleRef<double>
      Divide::Divide_fn::operator() (const TripleRef<double>& a,const TripleRef<double>& b) 
    {          
      ret_x=a[0];
      ret_sig=a[1]/b[1];
      //gaussian errors for the moment
      // (Sa/a)2 + (Sb/b)2 = (Sc/c)2 
      //  So after taking proportions, squaring, summing, 
      //  and taking the square root, you get a proportional error to the product c. Multiply that proportional error by c to get the actual standard deviation Sc. 
      ret_err=ret_sig*sqrt(pow((a[2]/a[1]),2) + pow((b[2]/b[1]),2));    
      return TripleRef<double>(ret_x,ret_sig,ret_err);      
    }
  }
}
