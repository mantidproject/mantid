//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/FakeMinimizer.h"
#include "MantidCurveFitting/CostFuncFitting.h"

#include "MantidAPI/FuncMinimizerFactory.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/Unit.h"

#include "MantidKernel/Logger.h"

namespace Mantid
{
namespace CurveFitting
{
  namespace
  {
    /// static logger
    Kernel::Logger g_log("FakeMinimizer");
  }

DECLARE_FUNCMINIMIZER(FakeMinimizer,Fake)


FakeMinimizer::FakeMinimizer()
{
    declareProperty(new API::WorkspaceProperty<API::MatrixWorkspace>("SomeOutput","abc",Kernel::Direction::Output),
        "Name of the output Workspace holding some output.");
    declareProperty("SomeInt",0,"Some integer value");
    declareProperty("SomeDouble",0.0,"Some double value");
    declareProperty("SomeString","Some units","Some string value");
}

FakeMinimizer::~FakeMinimizer()
{
}

void FakeMinimizer::initialize(API::ICostFunction_sptr, size_t maxIters)
{
    m_maxIters = maxIters;
    m_data.resize(m_maxIters);
    m_someInt = getProperty("SomeInt");
    m_someDouble = getProperty("SomeDouble");
    m_someString = getPropertyValue("SomeString");
}

/**
 * Do one iteration.
 * @param iter :: Current iteration number.
 * @return :: true if iterations to be continued, false if they can stop
 */
bool FakeMinimizer::iterate(size_t iter)
{
    m_data[iter] = static_cast<double>(iter) / static_cast<double>(m_maxIters - 1);

    if ( iter >= m_maxIters - 1 )
    {
      API::MatrixWorkspace_sptr ws = API::WorkspaceFactory::Instance().create("Workspace2D",1,m_maxIters,m_maxIters);
      auto & X = ws->dataX(0);
      for(size_t i = 0; i < X.size(); ++i)
      {
          X[i] = static_cast<double>(i + 1);
      }
      auto & Y = ws->dataY(0);
      Y.assign( m_data.begin(), m_data.end() );

      auto &E = ws->dataE(0);
      if ( m_maxIters > 0 )
        E[0] = static_cast<double>( m_someInt );
      if ( m_maxIters > 1 )
        E[1] = m_someDouble;

      auto ilabel = Kernel::UnitFactory::Instance().create("Label");
      auto label = boost::dynamic_pointer_cast<Kernel::Units::Label>(ilabel);
      label->setLabel( m_someString );
      auto axis = ws->getAxis(0);
      axis->unit() = ilabel;

      setProperty("SomeOutput",ws);
      return false;
    }

    return true;
}

double FakeMinimizer::costFunctionVal()
{
  return 0.0;
}

} // namespace CurveFitting
} // namespace Mantid
