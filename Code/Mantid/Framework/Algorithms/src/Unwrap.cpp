//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/Unwrap.h"

namespace Mantid
{
namespace Algorithms
{

DECLARE_ALGORITHM(Unwrap)

using namespace Kernel;
using namespace API;
using Geometry::IInstrument_const_sptr;

/// Default constructor
Unwrap::Unwrap()
{
  this->useAlgorithm("UnwrapMonitor");
  this->deprecatedDate("2011-03-17"); // Saint Patrick's day!!!!!
}

Unwrap::~Unwrap()
{}

const std::string Unwrap::name() const
{
  return "Unwrap";
}

int Unwrap::version() const
{
  return 1;
}

} // namespace Algorithm
} // namespace Mantid
