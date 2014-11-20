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

void PoldiAnalyseResiduals::setMeasuredCounts(const DataObjects::Workspace2D_sptr &measuredCounts)
{
    m_measured = measuredCounts;
}

void PoldiAnalyseResiduals::setFittedCounts(const DataObjects::Workspace2D_sptr &fittedCounts)
{
    m_fitted = fittedCounts;
}

void PoldiAnalyseResiduals::setInstrumentFromWorkspace(const DataObjects::Workspace2D_const_sptr &instrumentWorkspace)
{
    m_instrument = boost::make_shared<PoldiInstrumentAdapter>(instrumentWorkspace);
}

double PoldiAnalyseResiduals::sumCounts(const DataObjects::Workspace2D_sptr &workspace) const
{
    double sum = 0.0;
    for(size_t i = 0; i < workspace->getNumberHistograms(); ++i) {
        if(!workspace->getDetector(i)->isMasked()) {
            const MantidVec &counts = workspace->readY(i);
            sum += std::accumulate(counts.begin(), counts.end(), 0.0);
        }
    }

    return sum;
}

size_t PoldiAnalyseResiduals::numberOfPoints(const DataObjects::Workspace2D_sptr &workspace) const
{
    size_t sum = 0;
    for(size_t i = 0; i < workspace->getNumberHistograms(); ++i) {
        if(!workspace->getDetector(i)->isMasked()) {
            const MantidVec &counts = workspace->readY(i);
            sum += counts.size();
        }
    }

    return sum;// - 500;
}

void PoldiAnalyseResiduals::addValue(DataObjects::Workspace2D_sptr &workspace, double value) const
{
    for(size_t i = 0; i < workspace->getNumberHistograms(); ++i) {
        if(!workspace->getDetector(i)->isMasked()) {
            MantidVec &counts = workspace->dataY(i);
            for(size_t i = 0; i < counts.size(); ++i) {
                counts[i] += value;
            }
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
    setMeasuredCounts(getProperty("MeasuredCountData"));
    setFittedCounts(getProperty("FittedCountData"));

    setInstrumentFromWorkspace(m_measured);

    PoldiResidualCorrelationCore core(g_log);
    core.setInstrument(PoldiAbstractDetector_sptr(new PoldiDeadWireDecorator(m_measured->getInstrument(), m_instrument->detector())), m_instrument->chopper());
    core.setWavelengthRange(1.1, 5.0);

    double totalMeasured = sumCounts(m_measured);
    double totalFitted = sumCounts(m_fitted);
    double numberOfDataPoints = static_cast<double>(numberOfPoints(m_measured));

    double difference = (totalMeasured - totalFitted) / numberOfDataPoints;

    //DataObjects::Workspace2D_sptr fittedDifference
    addValue(m_fitted, difference);


    IAlgorithm_sptr minus = createChildAlgorithm("Minus");
    minus->setProperty("LHSWorkspace", m_measured);
    minus->setProperty("RHSWorkspace", m_fitted);
    minus->execute();

    MatrixWorkspace_sptr fg = minus->getProperty("OutputWorkspace");
    DataObjects::Workspace2D_sptr residuals = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(fg);

    DataObjects::Workspace2D_sptr sum = core.calculate(residuals, m_fitted);

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
        DataObjects::Workspace2D_sptr corr = core.calculate(residuals, m_fitted);

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

    addValue(m_fitted, -difference);

    setProperty("OutputWorkspace", boost::dynamic_pointer_cast<Workspace>(sum));
}



} // namespace SINQ
} // namespace Mantid
