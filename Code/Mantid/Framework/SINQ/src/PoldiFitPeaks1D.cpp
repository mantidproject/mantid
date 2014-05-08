/*WIKI*
PoldiFitPeaks1D takes a TableWorkspace with peaks (for example from [[ PoldiPeakSearch ]]) and a spectrum from
[[ PoldiAutoCorrelation ]] and tries to fit a Gaussian peak profile to the spectrum for each peak. Usually, the
peaks are accompanied by a quadratic background, so this is fitted as well.

The implementation is very close to the original POLDI analysis software (using the same profile function). One
point where this routine differs is error calculation. In the original program the parameter errors were adjusted
by averaging <math>\chi^2</math>-values, but this does not work properly if there is an outlier caused by a bad
fit for one of the peaks.
*WIKI*/

#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"

#include "MantidSINQ/PoldiFitPeaks1D.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidAPI/TableRow.h"

#include "MantidSINQ/PoldiUtilities/UncertainValue.h"
#include "MantidSINQ/PoldiUtilities/UncertainValueIO.h"


namespace Mantid
{
namespace Poldi
{

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::CurveFitting;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(PoldiFitPeaks1D)


PoldiFitPeaks1D::PoldiFitPeaks1D() :
    m_peaks(),
    m_profileTies(),
    m_fitCharacteristics(),
    m_peakResultOutput(),
    m_fwhmMultiples(1.0)
{
}

PoldiFitPeaks1D::~PoldiFitPeaks1D()
{
}


/// Algorithm's name for identification. @see Algorithm::name
const std::string PoldiFitPeaks1D::name() const { return "PoldiFitPeaks1D";}

/// Algorithm's version for identification. @see Algorithm::version
int PoldiFitPeaks1D::version() const { return 1;}

/// Algorithm's category for identification. @see Algorithm::category
const std::string PoldiFitPeaks1D::category() const { return "SINQ\\Poldi\\PoldiSet"; }


/// Sets documentation strings for this algorithm
void PoldiFitPeaks1D::initDocs()
{
    this->setWikiSummary("PoldiPeakFit1D fits peak profiles to POLDI auto-correlation data.");
    this->setOptionalMessage("PoldiPeakFit1D fits peak profiles to POLDI auto-correlation data.");
}

void PoldiFitPeaks1D::init()
{
    declareProperty(new WorkspaceProperty<Workspace2D>("InputWorkspace","",Direction::Input), "An input workspace containing a POLDI auto-correlation spectrum.");
    boost::shared_ptr<BoundedValidator<double> > minFwhmPerDirection = boost::make_shared<BoundedValidator<double> >();
    minFwhmPerDirection->setLower(2.0);
    declareProperty("FwhmMultiples", 2.0, minFwhmPerDirection, "Each peak will be fitted using x * FWHM data in each direction.", Direction::Input);
    declareProperty(new WorkspaceProperty<TableWorkspace>("PoldiPeakTable","",Direction::Input), "A table workspace containing POLDI peak data.");

    declareProperty(new WorkspaceProperty<TableWorkspace>("OutputWorkspace","RefinedPeakTable",Direction::Output), "Output workspace with refined peak data.");
    declareProperty(new WorkspaceProperty<TableWorkspace>("ResultTableWorkspace","ResultTable",Direction::Output), "Fit results.");
    declareProperty(new WorkspaceProperty<TableWorkspace>("FitCharacteristicsWorkspace","FitCharacteristics",Direction::Output), "Fit characteristics for each peak.");
    declareProperty(new WorkspaceProperty<Workspace>("FitPlotsWorkspace","FitPlots",Direction::Output), "Plots of all peak fits.");
}

void PoldiFitPeaks1D::initializePeakFunction(IPeakFunction_sptr peakFunction, IFunction_sptr backgroundFunction, std::string ties)
{
    m_profileTemplate = peakFunction;
    m_backgroundTemplate = backgroundFunction;
    m_profileTies = ties;
}

PoldiPeakCollection_sptr PoldiFitPeaks1D::getInitializedPeakCollection(TableWorkspace_sptr peakTable)
{
    PoldiPeakCollection_sptr peaks(new PoldiPeakCollection(peakTable));
    peaks->setProfileFunction(m_profileTemplate);
    peaks->setBackgroundFunction(m_backgroundTemplate);
    peaks->setProfileTies(m_profileTies);

    return peaks;
}

void PoldiFitPeaks1D::exec()
{
    initializePeakFunction(boost::dynamic_pointer_cast<IPeakFunction>(FunctionFactory::Instance().createFunction("Gaussian")),
                           FunctionFactory::Instance().createInitialized("name=UserFunction, Formula=A0 + A1*(x - x0)^2"),
                           "f1.x0 = f0.PeakCentre");

    // Number of points around the peak center to use for the fit
    m_fwhmMultiples = getProperty("FwhmMultiples");

    // try to construct PoldiPeakCollection from provided TableWorkspace
    TableWorkspace_sptr poldiPeakTable = getProperty("PoldiPeakTable");    
    m_peaks = getInitializedPeakCollection(poldiPeakTable);

    g_log.information() << "Peaks to fit: " << m_peaks->peakCount() << std::endl;



    Workspace2D_sptr dataWorkspace = getProperty("InputWorkspace");

    m_fitCharacteristics = boost::dynamic_pointer_cast<TableWorkspace>(WorkspaceFactory::Instance().createTable());
    WorkspaceGroup_sptr fitPlotGroup(new WorkspaceGroup);

    for(size_t i = 0; i < m_peaks->peakCount(); ++i) {
        PoldiPeak_sptr currentPeak = m_peaks->peak(i);
        IFunction_sptr currentProfile = m_peaks->getSinglePeakProfile(i);

        IAlgorithm_sptr fit = getFitAlgorithm(dataWorkspace, currentPeak, currentProfile);

        bool fitSuccess = fit->execute();

        if(fitSuccess) {
            m_peaks->setSingleProfileParameters(i, fit->getProperty("Function"));
            addPeakFitCharacteristics(fit->getProperty("OutputParameters"));

            MatrixWorkspace_sptr fpg = fit->getProperty("OutputWorkspace");
            fitPlotGroup->addWorkspace(fpg);
        }
    }

    m_peakResultOutput = generateResultTable(m_peaks);

    setProperty("OutputWorkspace", m_peaks->asTableWorkspace());
    setProperty("FitCharacteristicsWorkspace", m_fitCharacteristics);
    setProperty("ResultTableWorkspace", m_peakResultOutput);
    setProperty("FitPlotsWorkspace", fitPlotGroup);
}

IAlgorithm_sptr PoldiFitPeaks1D::getFitAlgorithm(Workspace2D_sptr dataWorkspace, PoldiPeak_sptr peak, IFunction_sptr profile)
{
    double width = peak->fwhm();
    double extent = std::min(0.05, std::max(0.002, width)) * m_fwhmMultiples;

    std::pair<double, double> xBorders(peak->q() - extent, peak->q() + extent);

    IAlgorithm_sptr fitAlgorithm = createChildAlgorithm("Fit", -1, -1, false);
    fitAlgorithm->setProperty("CreateOutput", true);
    fitAlgorithm->setProperty("Output", "FitPeaks1D");
    fitAlgorithm->setProperty("CalcErrors", true);
    fitAlgorithm->setProperty("Function", profile);
    fitAlgorithm->setProperty("InputWorkspace", dataWorkspace);
    fitAlgorithm->setProperty("WorkspaceIndex", 0);
    fitAlgorithm->setProperty("StartX", xBorders.first);
    fitAlgorithm->setProperty("EndX", xBorders.second);

    return fitAlgorithm;
}

void PoldiFitPeaks1D::addPeakFitCharacteristics(ITableWorkspace_sptr fitResult)
{
    if(m_fitCharacteristics->columnCount() == 0) {
        initializeFitResultWorkspace(fitResult);
    }

    TableRow newRow = m_fitCharacteristics->appendRow();

    for(size_t i = 0; i < fitResult->rowCount(); ++i) {
        TableRow currentRow = fitResult->getRow(i);

        newRow << UncertainValueIO::toString(UncertainValue(currentRow.Double(1), currentRow.Double(2)));
    }
}

void PoldiFitPeaks1D::initializeFitResultWorkspace(ITableWorkspace_sptr fitResult)
{
    for(size_t i = 0; i < fitResult->rowCount(); ++i) {
        TableRow currentRow = fitResult->getRow(i);
        m_fitCharacteristics->addColumn("str", currentRow.cell<std::string>(0));
    }
}

void PoldiFitPeaks1D::initializePeakResultWorkspace(TableWorkspace_sptr peakResultWorkspace)
{
    peakResultWorkspace->addColumn("str", "Q");
    peakResultWorkspace->addColumn("str", "d");
    peakResultWorkspace->addColumn("double", "deltaD/d *10^3");
    peakResultWorkspace->addColumn("str", "FWHM rel. *10^3");
    peakResultWorkspace->addColumn("str", "Intensity");
}

void PoldiFitPeaks1D::storePeakResult(TableRow tableRow, PoldiPeak_sptr peak)
{
    UncertainValue q = peak->q();
    UncertainValue d = peak->d();

    tableRow << UncertainValueIO::toString(q)
             << UncertainValueIO::toString(d)
             << d.error() / d.value() * 1e3
             << UncertainValueIO::toString(peak->fwhm(PoldiPeak::Relative) * 1e3)
             << UncertainValueIO::toString(peak->intensity());
}

TableWorkspace_sptr PoldiFitPeaks1D::generateResultTable(PoldiPeakCollection_sptr peaks)
{
    TableWorkspace_sptr outputTable = boost::dynamic_pointer_cast<TableWorkspace>(WorkspaceFactory::Instance().createTable());
    initializePeakResultWorkspace(outputTable);

    for(size_t i = 0; i < peaks->peakCount(); ++i) {
        storePeakResult(outputTable->appendRow(), peaks->peak(i));
    }

    return outputTable;
}


} // namespace Poldi
} // namespace Mantid
