#include "MantidSINQ/PoldiAnalyseResiduals.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidSINQ/PoldiUtilities/PoldiResidualCorrelationCore.h"
#include "MantidSINQ/PoldiUtilities/PoldiDeadWireDecorator.h"

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
    Algorithm()
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

/// Sums the counts of all spectra specified by the supplied list of workspace indices.
double PoldiAnalyseResiduals::sumCounts(const DataObjects::Workspace2D_sptr &workspace, const std::vector<int> &workspaceIndices) const
{
    double sum = 0.0;
    for(size_t i = 0; i < workspaceIndices.size(); ++i) {
        const MantidVec &counts = workspace->readY(workspaceIndices[i]);
        sum += std::accumulate(counts.begin(), counts.end(), 0.0);
    }

    return sum;
}

/// Counts the number of values in each spectrum specified by the list of workspace indices.
size_t PoldiAnalyseResiduals::numberOfPoints(const DataObjects::Workspace2D_sptr &workspace, const std::vector<int> &workspaceIndices) const
{
    size_t sum = 0;
    for(size_t i = 0; i < workspaceIndices.size(); ++i) {
        const MantidVec &counts = workspace->readY(workspaceIndices[i]);
        sum += counts.size();
    }

    return sum;
}

/// Adds the specified value to all spectra specified by the given workspace indices.
void PoldiAnalyseResiduals::addValue(DataObjects::Workspace2D_sptr &workspace, double value, const std::vector<int> &workspaceIndices) const
{
    for(size_t i = 0; i < workspaceIndices.size(); ++i) {
        MantidVec &counts = workspace->dataY(workspaceIndices[i]);
        for(size_t j = 0; j < counts.size(); ++j) {
            counts[j] += value;
        }
    }
}



//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
   */
void PoldiAnalyseResiduals::init()
{
    declareProperty(new WorkspaceProperty<DataObjects::Workspace2D>("MeasuredCountData", "", Direction::Input), "Input workspace containing the measured data.");
    declareProperty(new WorkspaceProperty<DataObjects::Workspace2D>("FittedCountData", "", Direction::Input), "Input workspace containing the fitted data.");
    declareProperty("MaxIterations", 0, Direction::Input);

    declareProperty(new WorkspaceProperty<DataObjects::Workspace2D>("OutputWorkspace", "", Direction::Output), "An output workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
   */
void PoldiAnalyseResiduals::exec()
{
    DataObjects::Workspace2D_sptr measured = getProperty("MeasuredCountData");
    DataObjects::Workspace2D_sptr calculated = getProperty("FittedCountData");

    PoldiInstrumentAdapter_sptr poldiInstrument = boost::make_shared<PoldiInstrumentAdapter>(measured);
    // Dead wires need to be taken into account
    PoldiAbstractDetector_sptr deadWireDetector = boost::make_shared<PoldiDeadWireDecorator>(measured->getInstrument(), poldiInstrument->detector());

    // Since the valid workspace indices are required for some calculations, we extract and keep them
    const std::vector<int> &validWorkspaceIndices = deadWireDetector->availableElements();

    double totalMeasured = sumCounts(measured, validWorkspaceIndices);
    double totalFitted = sumCounts(calculated, validWorkspaceIndices);
    double numberOfDataPoints = static_cast<double>(numberOfPoints(measured, validWorkspaceIndices));

    double difference = (totalMeasured - totalFitted) / numberOfDataPoints;

    //DataObjects::Workspace2D_sptr fittedDifference
    addValue(calculated, difference, validWorkspaceIndices);


    IAlgorithm_sptr minus = createChildAlgorithm("Minus");
    minus->setProperty("LHSWorkspace", measured);
    minus->setProperty("RHSWorkspace", calculated);
    minus->execute();

    MatrixWorkspace_sptr fg = minus->getProperty("OutputWorkspace");
    DataObjects::Workspace2D_sptr residuals = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(fg);

    PoldiResidualCorrelationCore core(g_log, 0.1);
    core.setInstrument(deadWireDetector, poldiInstrument->chopper());
    core.setWavelengthRange(1.1, 5.0);
    DataObjects::Workspace2D_sptr sum = core.calculate(residuals, calculated);

    const MantidVec &corrCounts = sum->readY(0);
    double csum = 0.0;
    for(auto it = corrCounts.begin(); it != corrCounts.end(); ++it) {
        csum += fabs(*it);
    }

    double percChange = csum / totalMeasured * 100.0;

    std::cout << "Absolute change: " << csum / totalMeasured * 100.0 << std::endl;

    IAlgorithm_sptr plus = createChildAlgorithm("Plus");
    plus->setProperty("LHSWorkspace", sum);

    int maxIterations = getProperty("MaxIterations");
    size_t iterations = 1;

    while(percChange > 1.0 && iterations < maxIterations) {
        DataObjects::Workspace2D_sptr corr = core.calculate(residuals, calculated);

        const MantidVec &corrCounts = corr->readY(0);
        double csum = 0.0;
        for(auto it = corrCounts.begin(); it != corrCounts.end(); ++it) {
            csum += fabs(*it);
        }

        percChange = csum / totalMeasured * 100.0;

        std::cout << "Absolute change: " << percChange << std::endl;

        plus->setProperty("RHSWorkspace", corr);
        plus->execute();

        MatrixWorkspace_sptr plusResult = plus->getProperty("OutputWorkspace");
        sum = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(plusResult);
        ++iterations;
    }

    addValue(calculated, -difference, validWorkspaceIndices);

    setProperty("OutputWorkspace", boost::dynamic_pointer_cast<Workspace>(sum));
}



} // namespace SINQ
} // namespace Mantid
