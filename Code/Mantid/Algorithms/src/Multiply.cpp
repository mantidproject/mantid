//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <cmath>
#include "MantidAlgorithms/Multiply.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/Exception.h" 
#include "MantidAPI/TripleIterator.h" 
#include "MantidAPI/IErrorHelper.h" 

// Register the class into the algorithm factory
DECLARE_NAMESPACED_ALGORITHM(Mantid::Algorithms,Multiply)
using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace Mantid
{
  namespace Algorithms
  {
    // Get a reference to the logger
    Logger& Multiply::g_log = Logger::get("Multiply");

    /** Performs the multiply operation using Iterators and the std::tranform function.
    * @param it_in1 The const iterator to the lhs data item
    * @param it_in2 The const iterator to the rhs data item
    * @param it_out The output iterator to the new workspace
    */
    void Multiply::performBinaryOperation(Workspace::const_iterator it_in1, Workspace::const_iterator it_in2,
        Workspace::iterator it_out)
    {
      std::transform(it_in1.begin(),it_in1.end(),it_in2.begin(),it_out.begin(),Multiply_fn());
    }

    /** Performs the addition with Gausian errors within the transform function
    * @param a The triple ref of the first workspace data item
    * @param b The triple ref of the second workspace data item
    * @returns A triple ref of the result with Gausian errors
    */
    HistDataValue&
      Multiply::Multiply_fn::operator() (const IPointData& a,const IPointData& b) 
    {           
      //copy the values from lhs
      result = a;
      //use the error helper to correct the changed values in result
      a.ErrorHelper()->multiply(a,b,result);     
      return result;          
    }
  }
}
