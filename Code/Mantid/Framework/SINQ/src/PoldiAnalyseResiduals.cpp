#include "MantidSINQ/PoldiAnalyseResiduals.h"

namespace Mantid
{
namespace Poldi
{

using Mantid::Kernel::Direction;
using Mantid::API::WorkspaceProperty;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(PoldiAnalyseResiduals)



//----------------------------------------------------------------------------------------------
/** Constructor
   */
PoldiAnalyseResiduals::PoldiAnalyseResiduals()
{
}

//----------------------------------------------------------------------------------------------
/** Destructor
   */
PoldiAnalyseResiduals::~PoldiAnalyseResiduals()
{
}


//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string PoldiAnalyseResiduals::name() const { return "PoldiAnalyseResiduals"; }

/// Algorithm's version for identification. @see Algorithm::version
int PoldiAnalyseResiduals::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string PoldiAnalyseResiduals::category() const { return "SINQ\\Poldi"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string PoldiAnalyseResiduals::summary() const { return "Analysis of residuals after fitting POLDI 2D-spectra."; }

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
   */
void PoldiAnalyseResiduals::init()
{
    declareProperty(new WorkspaceProperty<>("MeasuredCountData","",Direction::Input), "Input workspace containing the measured data.");
    declareProperty(new WorkspaceProperty<>("FittedCountData","",Direction::Input), "Input workspace containing the fitted data.");

    declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output), "An output workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
   */
void PoldiAnalyseResiduals::exec()
{
    // TODO Auto-generated execute stub
}



} // namespace SINQ
} // namespace Mantid
