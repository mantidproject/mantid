//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <cmath>
#include "MantidAlgorithms/Minus.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/Exception.h" 
#include "MantidAPI/TripleIterator.h" 
#include "MantidAPI/IErrorHelper.h" 

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

    /** Performs the minus operation using Iterators and the std::tranform function.
    * @param it_in1 The const iterator to the lhs data item
    * @param it_in2 The const iterator to the rhs data item
    * @param it_out The output iterator to the new workspace
    */
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
    LocatedDataValue&
      Minus::Minus_fn::operator() (const ILocatedData& a,const ILocatedData& b) 
    {     
      //copy the values from lhs
      result = a;
      //use the error helper to correct the changed values in result
      a.ErrorHelper()->minus(a,b,result);     
      return result;          
    }
  }
}
