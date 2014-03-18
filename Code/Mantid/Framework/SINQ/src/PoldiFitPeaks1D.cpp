/*WIKI*
TODO: Enter a full wiki-markup description of your algorithm here. You can then use the Build/wiki_maker.py script to generate your full wiki page.
*WIKI*/

#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceFactory.h"

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
    m_fitCharacteristics(),
    m_fitResultOutput(),
    m_pointsPerPeak(0)
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
    boost::shared_ptr<BoundedValidator<int> > minPointsPerPeakValidator = boost::make_shared<BoundedValidator<int> >();
    minPointsPerPeakValidator->setLower(4);
    declareProperty("PointsPerPeak", 24, minPointsPerPeakValidator, "Number of points which describe a peak in the spectrum.", Direction::Input);
    declareProperty(new WorkspaceProperty<TableWorkspace>("PoldiPeakTable","",Direction::I), "A table workspace containing POLDI peak data.");
    declareProperty(new WorkspaceProperty<TableWorkspace>("PoldiPeakFitResult","",Direction::Output), "Fit results.");
    declareProperty(new WorkspaceProperty<TableWorkspace>("PoldiPeakFitCharacteristics","",Direction::Output), "Fit characteristics for each peak.");
}

void PoldiFitPeaks1D::exec()
{
    m_profileTemplate = boost::dynamic_pointer_cast<IPeakFunction>(FunctionFactory::Instance().createFunction("Gaussian"));
    m_backgroundTemplate = FunctionFactory::Instance().createInitialized("name=UserFunction, Formula=A0 + A1*(x - x0)^2");

    m_pointsPerPeak = getProperty("PointsPerPeak");
    // try to construct PoldiPeakCollection from provided TableWorkspace
    TableWorkspace_sptr poldiPeakTable = getProperty("PoldiPeakTable");
    m_peaks = PoldiPeakCollection_sptr(new PoldiPeakCollection(poldiPeakTable));

    g_log.information() << "Peaks to fit: " << m_peaks->peakCount() << std::endl;

    m_peaks->setProfileFunction(m_profileTemplate);
    m_peaks->setBackgroundFunction(m_backgroundTemplate);
    m_peaks->setProfileTies("f1.x0 = f0.PeakCentre");

    Workspace2D_sptr dataWorkspace = getProperty("InputWorkspace");

    m_fitCharacteristics = boost::dynamic_pointer_cast<TableWorkspace>(WorkspaceFactory::Instance().createTable());

    for(size_t i = 0; i < m_peaks->peakCount(); ++i) {
        IAlgorithm_sptr fit = getFitAlgorithm(dataWorkspace, i);

        bool fitSuccess = fit->execute();

        if(fitSuccess) {
            m_peaks->setProfileParameters(i, fit->getProperty("Function"));
            storeChiSquare(fit->getProperty("OutputChi2overDoF"));
            addPeakFitCharacteristics(fit->getProperty("OutputParameters"));
        }
    }

    adjustErrorEstimates();

    setProperty("PoldiPeakTable", m_peaks->asTableWorkspace());
    setProperty("PoldiPeakFitCharacteristics", m_fitCharacteristics);
    setProperty("PoldiPeakFitResult", m_fitResultOutput);
}

IAlgorithm_sptr PoldiFitPeaks1D::getFitAlgorithm(Workspace2D_sptr dataWorkspace, size_t peakIndex)
{
    PoldiPeak_sptr peak = m_peaks->peak(peakIndex);
    std::pair<double, double> xBorders = getXBorders(dataWorkspace->dataX(0), peak->q());

    if(peak->fwhm() == 0.0) {
        peak->setFwhm(2.0 * sqrt(2.0 * log(2.0)) * (xBorders.second - xBorders.first) / 20.0);
        peak->setIntensity(peak->intensity() * 1.5);
    }

    IAlgorithm_sptr fitAlgorithm = createChildAlgorithm("Fit", -1, -1, false);
    fitAlgorithm->setProperty("CreateOutput", true);
    fitAlgorithm->setProperty("Output", "PoldiFitPeak1D");
    fitAlgorithm->setProperty("CalcErrors", true);
    fitAlgorithm->setProperty("Function", m_peaks->getPeakProfile(peakIndex));
    fitAlgorithm->setProperty("Ties", m_peaks->getProfileTies());
    fitAlgorithm->setProperty("InputWorkspace", dataWorkspace);
    fitAlgorithm->setProperty("WorkspaceIndex", 0);
    fitAlgorithm->setProperty("StartX", xBorders.first);
    fitAlgorithm->setProperty("EndX", xBorders.second);

    return fitAlgorithm;
}

std::pair<double, double> PoldiFitPeaks1D::getXBorders(const MantidVec &xdata, double peakPosition)
{
    MantidVec::const_iterator peakIndex;
    for(peakIndex = xdata.begin(); peakIndex != xdata.end(); ++peakIndex) {
        if(std::abs(*peakIndex - peakPosition) < 1e-5) {
            break;
        }
    }

    return std::pair<double, double>(*(peakIndex - m_pointsPerPeak / 2), *(peakIndex + m_pointsPerPeak / 2));
}

void PoldiFitPeaks1D::storeChiSquare(double chiSquare)
{
    m_chiSquareValues.push_back(chiSquare);
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

void PoldiFitPeaks1D::adjustUncertainties(PoldiPeak_sptr peak, double factor)
{
    peak->multiplyErrors(factor);
}

void PoldiFitPeaks1D::initializePeakResultWorkspace(TableWorkspace_sptr peakResultWorkspace)
{
    peakResultWorkspace->addColumn("str", "Q");
    peakResultWorkspace->addColumn("str", "d");
    peakResultWorkspace->addColumn("double", "deltaD/d *10^-3");
    peakResultWorkspace->addColumn("str", "FWHM rel. *10^-3");
    peakResultWorkspace->addColumn("str", "Intensity");
}

void PoldiFitPeaks1D::storePeakResult(PoldiPeak_sptr peak)
{
    TableRow newRow = m_fitResultOutput->appendRow();

    UncertainValue q = peak->q();
    UncertainValue d = peak->d();

    newRow << UncertainValueIO::toString(q)
           << UncertainValueIO::toString(d)
           << d.error() / d.value() * 1e3
           << UncertainValueIO::toString(peak->fwhm() / q.value() * 1e3)
           << UncertainValueIO::toString(peak->intensity());
}

void PoldiFitPeaks1D::adjustErrorEstimates()
{
    double chiSquareSum = std::accumulate(m_chiSquareValues.begin(), m_chiSquareValues.end(), 0.0);
    double sqrtChiMean  = sqrt(chiSquareSum / double(m_chiSquareValues.size()));

    m_fitResultOutput = boost::dynamic_pointer_cast<TableWorkspace>(WorkspaceFactory::Instance().createTable());
    initializePeakResultWorkspace(m_fitResultOutput);

    for(size_t i = 0; i < m_peaks->peakCount(); ++i) {
        adjustUncertainties(m_peaks->peak(i), sqrtChiMean);
        storePeakResult(m_peaks->peak(i));
    }
}


} // namespace Poldi
} // namespace Mantid
