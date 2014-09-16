#include "MantidAlgorithms/Stitch1DMany.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/BoundedValidator.h"

#include <boost/make_shared.hpp>

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
  namespace Algorithms
  {
    DECLARE_ALGORITHM(Stitch1DMany)

    /** Initialize the algorithm's properties.
     */
    void Stitch1DMany::init()
    {

      declareProperty(new ArrayProperty<std::string>("InputWorkspaces"),
          "Input Workspaces. List of histogram workspaces to stitch together.");

      declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace", "", Direction::Output),
          "Output stitched workspace.");

      declareProperty(new ArrayProperty<double>("Params", boost::make_shared<RebinParamsValidator>(true)),
          "Rebinning Parameters. See Rebin for format.");

      declareProperty(new ArrayProperty<double>("StartOverlaps"), "Start overlaps for stitched workspaces.");

      declareProperty(new ArrayProperty<double>("EndOverlaps"), "End overlaps for stitched workspaces.");

      declareProperty(new PropertyWithValue<bool>("ScaleRHSWorkspace", true, Direction::Input),
          "Scaling either with respect to workspace 1 or workspace 2");

      declareProperty(new PropertyWithValue<bool>("UseManualScaleFactor", false, Direction::Input),
          "True to use a provided value for the scale factor.");

      auto manualScaleFactorValidator = boost::make_shared<BoundedValidator<double> >();
      manualScaleFactorValidator->setLower(0);
      manualScaleFactorValidator->setExclusive(true);
      declareProperty(new PropertyWithValue<double>("ManualScaleFactor", 1.0, manualScaleFactorValidator, Direction::Input),
          "Provided value for the scale factor.");

      declareProperty(new ArrayProperty<double>("OutScaleFactors", Direction::Output),
          "The actual used values for the scaling factores at each stitch step.");
    }

    /** Execute the algorithm.
     */
    void Stitch1DMany::exec()
    {
    }

  } // namespace Algorithms
} // namespace Mantid
