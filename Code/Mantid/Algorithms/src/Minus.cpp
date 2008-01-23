//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <cmath>
#include "MantidAlgorithms/Minus.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/Exception.h" 
#include "MantidAPI/TripleIterator.h" 

// Register the class into the algorithm factory
DECLARE_NAMESPACED_ALGORITHM(Mantid::Algorithms,Minus)
using namespace Mantid::API;
using namespace Mantid::Kernel;
namespace Mantid
{
  namespace Algorithms
  {
    // Get a reference to the logger
    Logger& Minus::g_log = Logger::get("Minus");

    void Minus::performBinaryOperation(API::Workspace::const_iterator it_in1, API::Workspace::const_iterator it_in2,
        API::Workspace::iterator it_out)
    {
      std::transform(it_in1.begin(),it_in1.end(),it_in2.begin(),it_out.begin(),Minus_fn());
    }


    /** Performs the addition with Gausian errors within the transform function
    * @param a The triple ref of the first workspace data item
    * @param b The triple ref of the second workspace data item
    * @returns A triple ref of the result with Gausian errors
    */
    TripleRef<double>
      Minus::Minus_fn::operator() (const TripleRef<double>& a,const TripleRef<double>& b) 
    {     
      ret_x=a[0];
      ret_sig=a[1]-b[1];
      //gaussian errors for the moment
      ret_err=sqrt((a[2]*a[2])+(b[2]*b[2]));     
      return TripleRef<double>(ret_x,ret_sig,ret_err);      
    }
  }
}
