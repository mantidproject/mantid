//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFunctionMD.h"
#include "MantidAPI/Expression.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/IConstraint.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Exception.h"

#include "boost/lexical_cast.hpp"

#include <sstream>
#include <iostream> 

namespace Mantid
{
namespace API
{
  using namespace Geometry;
  
  Kernel::Logger& IFunctionMD::g_log = Kernel::Logger::get("IFunctionMD");

  /** Set the workspace
    * @param ws A shared pointer to a workspace. Must be a MatrixWorkspace.
    * @param slicing A string identifying the data to be fitted. Format for IFunctionMD:
    *  "WorkspaceIndex=int,StartX=double,EndX=double". StartX and EndX are optional.
  */
  void IFunctionMD::setWorkspace(boost::shared_ptr<Workspace> ws,const std::string& slicing)
  {
    try
    {
      m_workspace = boost::dynamic_pointer_cast<IMDWorkspace>(ws);
      if (!m_workspace)
      {
        throw std::invalid_argument("Workspace has a wrong type (not a IMDWorkspace)");
      }
     
    }
    catch(std::exception& e)
    {
      g_log.error() << "IFunctionMD::setWorkspace failed with error: " << e.what() << '\n';
      throw;
    }
  }

  /// Get the workspace
  boost::shared_ptr<const Workspace> IFunctionMD::getWorkspace()const
  {
    return m_workspace;
  }

  /// Returns the size of the fitted data (number of double values returned by the function)
  int IFunctionMD::dataSize()const
  {
    return m_dataSize;
  }

  /// Returns a pointer to the fitted data. These data are taken from the workspace set by setWorkspace() method.
  const double* IFunctionMD::getData()const
  {
    return &m_data[0];
  }

  const double* IFunctionMD::getWeights()const
  {
    return &m_weights[0];
  }

  /// Function you want to fit to. 
  /// @param out The buffer for writing the calculated values. Must be big enough to accept dataSize() values
  void IFunctionMD::function(double* out)const
  {
    if (m_dataSize == 0) return;
    //function(out,m_xValues.get(),m_dataSize);
    // Add penalty factor to function if any constraint is violated

  }

  /// Derivatives of function with respect to active parameters
  void IFunctionMD::functionDeriv(Jacobian* out)
  {

  }


} // namespace API
} // namespace Mantid
