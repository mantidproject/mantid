#include "MantidSINQ/PoldiTruncateData.h"
#include "MantidSINQ/PoldiUtilities/PoldiChopperFactory.h"

namespace Mantid
{
namespace Poldi
{

using namespace Kernel;
using namespace API;
using namespace Geometry;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(PoldiTruncateData)


PoldiTruncateData::PoldiTruncateData() :
    m_chopper(),
    m_timeBinWidth(0.0),
    m_actualBinCount(0)
{
}


/// Algorithm's version for identification. @see Algorithm::version
int PoldiTruncateData::version() const { return 1;}

/// Algorithm's category for identification. @see Algorithm::category
const std::string PoldiTruncateData::category() const { return "SINQ\\Poldi"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string PoldiTruncateData::summary() const { return "Truncate POLDI time bins according to chopper speed."; }

void PoldiTruncateData::setChopperFromWorkspace(MatrixWorkspace_const_sptr workspace)
{
    /* This stuff will be gone once the changes from ticket #9445 have been integrated (PoldiInstrumentAdapter). */
    double chopperSpeed = 0.0;

    try {
        chopperSpeed = workspace->run().getPropertyValueAsType<std::vector<double> >("chopperspeed").front();
    } catch(std::invalid_argument&) {
        throw(std::runtime_error("Chopper speed could not be extracted from Workspace '" + workspace->name() + "'. Aborting."));
    }

    // Instrument definition
    Instrument_const_sptr poldiInstrument = workspace->getInstrument();

    // Chopper configuration
    PoldiChopperFactory chopperFactory;
    PoldiAbstractChopper_sptr chopper(chopperFactory.createChopper(std::string("default-chopper")));
    chopper->loadConfiguration(poldiInstrument);
    chopper->setRotationSpeed(chopperSpeed);

    setChopper(chopper);
}

void PoldiTruncateData::setChopper(PoldiAbstractChopper_sptr chopper)
{
    m_chopper = chopper;
}

void PoldiTruncateData::setTimeBinWidthFromWorkspace(MatrixWorkspace_const_sptr workspace)
{
    if(!workspace || workspace->getNumberHistograms() < 1) {
        throw std::invalid_argument("Workspace does not contain any data. Aborting.");
    }

    const MantidVec &xData = workspace->readX(0);

    if(xData.size() < 2) {
        throw std::invalid_argument("Spectrum does not contain any bins. Aborting.");
    }

    setActualBinCount(xData.size());
    setTimeBinWidth(xData[1] - xData[0]);
}

void PoldiTruncateData::setTimeBinWidth(double timeBinWidth)
{
    m_timeBinWidth = timeBinWidth;
}

void PoldiTruncateData::setActualBinCount(size_t actualBinCount)
{
    m_actualBinCount = actualBinCount;
}

size_t PoldiTruncateData::getCalculatedBinCount()
{
    if(!m_chopper) {
        throw std::invalid_argument("Cannot calculate bin count without chopper.");
    }

    if(m_timeBinWidth <= 0.0) {
        throw std::invalid_argument("Cannot perform calculations with a bin width of 0 or less.");
    }

    return static_cast<size_t>(m_chopper->cycleTime() / m_timeBinWidth);
}

size_t PoldiTruncateData::getActualBinCount()
{
    return m_actualBinCount;
}

void PoldiTruncateData::init()
{
    declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input), "Input workspace containing raw POLDI data.");
    declareProperty(new PropertyWithValue<std::string>("ExtraCountsWorkspaceName", "", Direction::Input), "Workspace name for extra counts. Leave empty if not required.");
    declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace","",Direction::Output), "Output workspace with truncated POLDI data.");
}

void PoldiTruncateData::exec()
{
    MatrixWorkspace_sptr inputWorkspace = getProperty("InputWorkspace");

    setChopperFromWorkspace(inputWorkspace);
    setTimeBinWidthFromWorkspace(inputWorkspace);

    try {
        MatrixWorkspace_sptr cropped = getCroppedWorkspace(inputWorkspace);
        setProperty("OutputWorkspace", cropped);

        if(!getPointerToProperty("ExtraCountsWorkspaceName")->isDefault()) {
            try {
                MatrixWorkspace_sptr extraCounts = getExtraCountsWorkspace(inputWorkspace);

                std::string extraCountsWorkspaceName = getProperty("ExtraCountsWorkspaceName");
                declareProperty(new WorkspaceProperty<MatrixWorkspace>("ExtraCountsWorkspace", extraCountsWorkspaceName, Direction::Output));
                setProperty("ExtraCountsWorkspace", extraCounts);
            } catch(std::invalid_argument) {
                m_log.warning() << "Extra count information was requested, but there are no extra bins." << std::endl;
            }
        }
    } catch(std::invalid_argument) {
        m_log.error() << "Cannot crop workspace. Please check the timing information." << std::endl;
        m_log.error() << "  Calculated bin count: " << getCalculatedBinCount() << std::endl;
        m_log.error() << "  Bin count in the workspace: " << getActualBinCount() << std::endl;
    }


}

double PoldiTruncateData::getMaximumTimeValue(size_t calculatedBinCount)
{
    if(calculatedBinCount == 0 || calculatedBinCount > m_actualBinCount) {
        throw std::invalid_argument("Maximum time value is not defined when calculated bin count is 0 or larger than actual bin count.");
    }

    return m_timeBinWidth * static_cast<double>(calculatedBinCount - 1);
}

double PoldiTruncateData::getMinimumExtraTimeValue(size_t calculatedBinCount)
{
    if(calculatedBinCount >= m_actualBinCount) {
        throw std::invalid_argument("Cannot process bin count which is larger than actual bin count in the data.");
    }

    return m_timeBinWidth * static_cast<double>(calculatedBinCount);
}

MatrixWorkspace_sptr PoldiTruncateData::getCroppedWorkspace(MatrixWorkspace_sptr workspace)
{
    double maximumXValue = getMaximumTimeValue(getCalculatedBinCount());

    return getWorkspaceBelowX(workspace, maximumXValue);
}

MatrixWorkspace_sptr PoldiTruncateData::getExtraCountsWorkspace(MatrixWorkspace_sptr workspace)
{
    double minimumXValue = getMinimumExtraTimeValue(getCalculatedBinCount());
    MatrixWorkspace_sptr croppedOutput = getWorkspaceAboveX(workspace, minimumXValue);

    return getSummedSpectra(croppedOutput);
}

MatrixWorkspace_sptr PoldiTruncateData::getWorkspaceBelowX(MatrixWorkspace_sptr workspace, double x)
{
    Algorithm_sptr crop = getCropAlgorithmForWorkspace(workspace);
    crop->setProperty("XMax", x);

    return getOutputWorkspace(crop);
}

MatrixWorkspace_sptr PoldiTruncateData::getWorkspaceAboveX(MatrixWorkspace_sptr workspace, double x)
{
    Algorithm_sptr crop = getCropAlgorithmForWorkspace(workspace);
    crop->setProperty("Xmin", x);

    return getOutputWorkspace(crop);
}

Algorithm_sptr PoldiTruncateData::getCropAlgorithmForWorkspace(MatrixWorkspace_sptr workspace)
{
    Algorithm_sptr crop = createChildAlgorithm("CropWorkspace");

    if(!crop) {
        throw std::runtime_error("Could not create CropWorkspace algorithm");
    }

    crop->setProperty("InputWorkspace", workspace);

    return crop;
}

MatrixWorkspace_sptr PoldiTruncateData::getOutputWorkspace(Algorithm_sptr algorithm)
{
    if(!algorithm || !algorithm->execute()) {
        throw std::runtime_error("Workspace could not be retrieved successfully.");
    }

    MatrixWorkspace_sptr outputWorkspace = algorithm->getProperty("OutputWorkspace");
    return outputWorkspace;
}

MatrixWorkspace_sptr PoldiTruncateData::getSummedSpectra(MatrixWorkspace_sptr workspace)
{
    Algorithm_sptr sumSpectra = createChildAlgorithm("SumSpectra");

    if(!sumSpectra) {
        throw std::runtime_error("Could not create SumSpectra algorithm.");
    }

    sumSpectra->setProperty("InputWorkspace", workspace);

    return getOutputWorkspace(sumSpectra);
}





} // namespace SINQ
} // namespace Mantid
