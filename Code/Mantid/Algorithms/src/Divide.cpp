//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <cmath>
#include "MantidAlgorithms/Divide.h"
#include "MantidAPI/WorkspaceIterator.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace Mantid
{
  namespace Algorithms
  {
    // Register the class into the algorithm factory
    DECLARE_ALGORITHM(Divide)

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
      int count = it_in1.end() - it_in1.begin();
      std::transform(it_in1.begin(),it_in1.end(),it_in2.begin(),it_out.begin(),Divide_fn(this,count));
    }


    /** Performs the Division with Gausian errors within the transform function
    * @param a The LocatedData ref of the first workspace data item
    * @param b The LocatedData ref of the second workspace data item
    * @returns A LocatedData ref of the result with Gausian errors
    */
    LocatedDataValue&
      Divide::Divide_fn::operator() (const ILocatedData& a,const ILocatedData& b)
    {
      //copy the values from lhs
      result = a;
      //use the error helper to correct the changed values in result
      a.ErrorHelper()->divide(a,b,result);
      if (m_progress++ % m_progress_step == 0) report_progress();
      return result;
    }
  }
}
