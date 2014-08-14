#include "MantidAlgorithms/RebinByTimeBase.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidKernel/Unit.h"

#include <boost/make_shared.hpp>

namespace Mantid
{
namespace Algorithms
{

  using namespace Mantid::Kernel;
  using namespace Mantid::API;

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  RebinByTimeBase::RebinByTimeBase()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  RebinByTimeBase::~RebinByTimeBase()
  {
  }
  
  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void RebinByTimeBase::init()
  {
    declareProperty(new API::WorkspaceProperty<API::IEventWorkspace>("InputWorkspace","",Direction::Input), "An input workspace containing TOF events.");

    declareProperty(new ArrayProperty<double>("Params", boost::make_shared<RebinParamsValidator>()),
      "A comma separated list of first bin boundary, width, last bin boundary. Optionally\n"
      "this can be followed by a comma and more widths and last boundary pairs. Values are in seconds since run start.");

    declareProperty(new API::WorkspaceProperty<API::MatrixWorkspace>("OutputWorkspace","",Direction::Output), "An output workspace.");
  }




} // namespace Algorithms
} // namespace Mantid
