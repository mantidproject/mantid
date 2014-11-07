#include "MantidSINQ/PoldiAnalyseResiduals.h"
#include "MantidDataObjects/TableWorkspace.h"

namespace Mantid
{
namespace Poldi
{

using namespace Kernel;
using namespace API;
using namespace Geometry;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(PoldiAnalyseResiduals)



//----------------------------------------------------------------------------------------------
/** Constructor
   */
PoldiAnalyseResiduals::PoldiAnalyseResiduals() :
    Algorithm(),
    m_measured(),
    m_fitted(),
    m_difference(),
    m_instrument()
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

void PoldiAnalyseResiduals::setMeasuredCounts(const DataObjects::Workspace2D_const_sptr &measuredCounts)
{
    m_measured = measuredCounts;
}

void PoldiAnalyseResiduals::setFittedCounts(const DataObjects::Workspace2D_const_sptr &fittedCounts)
{
    m_fitted = fittedCounts;
}

void PoldiAnalyseResiduals::setInstrumentFromWorkspace(const DataObjects::Workspace2D_const_sptr &instrumentWorkspace)
{
    m_instrument = boost::make_shared<PoldiInstrumentAdapter>(instrumentWorkspace);
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
   */
void PoldiAnalyseResiduals::init()
{
    declareProperty(new WorkspaceProperty<DataObjects::Workspace2D>("MeasuredCountData", "", Direction::Input), "Input workspace containing the measured data.");
    declareProperty(new WorkspaceProperty<DataObjects::Workspace2D>("FittedCountData", "", Direction::Input), "Input workspace containing the fitted data.");
    declareProperty(new WorkspaceProperty<DataObjects::TableWorkspace>("FittedPeaks", "", Direction::Input), "Input workspace containing fitted peaks.");

    declareProperty(new WorkspaceProperty<DataObjects::Workspace2D>("OutputWorkspace", "", Direction::Output), "An output workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
   */
void PoldiAnalyseResiduals::exec()
{
    setMeasuredCounts(getProperty("MeasuredCountData"));
    setFittedCounts(getProperty("FittedCountData"));

    setInstrumentFromWorkspace(m_measured);
}



} // namespace SINQ
} // namespace Mantid
