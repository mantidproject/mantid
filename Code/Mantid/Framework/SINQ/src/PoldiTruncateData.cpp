#include "MantidSINQ/PoldiTruncateData.h"
#include "MantidSINQ/PoldiUtilities/PoldiInstrumentAdapter.h"

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

/** Extract chopper from workspace.
 *
 *  A POLDI chopper is constructed from the instrument and log-information present
 *  in the workspace. If there is no valid instrument or the chopper speed is missing
 *  from the run information, exceptions are thrown.
 *
 *  @param workspace :: MatrixWorkspace containing POLDI data, instrument definition and log.
 */
void PoldiTruncateData::setChopperFromWorkspace(MatrixWorkspace_const_sptr workspace)
{
    PoldiInstrumentAdapter poldiInstrument(workspace);
    setChopper(poldiInstrument.chopper());
}

/** Sets the chopper used for the calculations.
 *
 *  @param chopper :: POLDI chopper compatible with the measurement.
 */
void PoldiTruncateData::setChopper(PoldiAbstractChopper_sptr chopper)
{
    m_chopper = chopper;
}

/** Extracts timing information from the given MatrixWorkspace.
 *
 *  This method checks that the workspace is valid, has at least one histogram and
 *  at least 2 bins. The bin count is stored, as well as the difference x_1 - x_0 as
 *  time bin width.
 *
 *  @param workspace :: Workspace with POLDI data
 */
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

/** Sets the width of one time bin.
 *
 *  @param timeBinWidth :: Width of one time bin in micro seconds.
 */
void PoldiTruncateData::setTimeBinWidth(double timeBinWidth)
{
    m_timeBinWidth = timeBinWidth;
}

/** Sets the number of time bins actually present in the data.
 *
 *  @param actualBinCount :: Number of time bins.
 */
void PoldiTruncateData::setActualBinCount(size_t actualBinCount)
{
    m_actualBinCount = actualBinCount;
}

/** Calculates the theoretical number of tim bins.
 *
 *  This method calculates the number of time bins according to the chopper speed and
 *  time bin width by the formula t(chopper cycle) / t(bin).
 *
 *  If chopper or time bin width have not been set previously, the method throws an
 *  std::invalid_argument exception.
 *
 *  @return Calculated number of time bins.
 */
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

/** Returns the number of time bins actually stored.
 *
 *  @return Actual number of bins in a spectrum.
 */
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

        removeProperty("OutputWorkspace");
    }
}

/** Transform the time bin count to the maximum allowed arrival time
 *
 *  This method gives the maximum allowed arrival time in the data, calculated this way:
 *    t(bin width) * (N(bins) - 1)
 *  The subtraction is necessary because the spectrum starts at 0.
 *
 *  @param calculatedBinCount :: Number of bins calculated.
 *  @return Maximum arrival time present in the data.
 */
double PoldiTruncateData::getMaximumTimeValue(size_t calculatedBinCount)
{
    if(calculatedBinCount == 0 || calculatedBinCount > m_actualBinCount) {
        throw std::invalid_argument("Maximum time value is not defined when calculated bin count is 0 or larger than actual bin count.");
    }

    return m_timeBinWidth * static_cast<double>(calculatedBinCount - 1);
}

/** Returns the first arrival time value that is not allowed in the data
 *
 *  t(bin width) * N(bins) is the first arrival time outside the allowed spectrum.
 *  If calculated count is larger than actual count, the method throws std::invalid_argument,
 *  since the workspace is too small to match the experimental parameters.
 *
 *  @param calculatedBinCount :: Number of bins calculated.
 *  @return Minimum arrival time that does not belong to the data.
 */
double PoldiTruncateData::getMinimumExtraTimeValue(size_t calculatedBinCount)
{
    if(calculatedBinCount >= m_actualBinCount) {
        throw std::invalid_argument("Cannot process bin count which is larger than actual bin count in the data.");
    }

    return m_timeBinWidth * static_cast<double>(calculatedBinCount);
}

/** Returns a MatrixWorkspace cropped to the correct time bin count
 *
 *  @param workspace :: Raw POLDI data with possible additional time bins.
 *  @return Workspace with exactly as many time bins as expected for the experiment parameters.
 */
MatrixWorkspace_sptr PoldiTruncateData::getCroppedWorkspace(MatrixWorkspace_sptr workspace)
{
    double maximumXValue = getMaximumTimeValue(getCalculatedBinCount());

    return getWorkspaceBelowX(workspace, maximumXValue);
}

/** Returns a MatrixWorkspace with all extra counts
 *
 *  This method takes the input workspce and exracts the extranous time bins that do
 *  not match the experimental parameters. In an experiment with a chopper cycle time
 *  of 1500 microseconds and a time bin width of 3 microseconds, there would be 500 bins
 *  that contain data. If there are more than 500 bins, these can be used to check if
 *  the instrument is okay - there should be only very few counts in these bins.
 *
 *  The method extracts the extra bins and sums them over all spectra (= detector wires),
 *  so if there were 10 extra bins, this workspace will contain one histogram with 10 bins.
 *
 *  @param workspace :: Raw POLDI data.
 *  @return MatrixWorkspace with summed extra counts.
 */
MatrixWorkspace_sptr PoldiTruncateData::getExtraCountsWorkspace(MatrixWorkspace_sptr workspace)
{
    double minimumXValue = getMinimumExtraTimeValue(getCalculatedBinCount());
    MatrixWorkspace_sptr croppedOutput = getWorkspaceAboveX(workspace, minimumXValue);

    return getSummedSpectra(croppedOutput);
}

/** Returns a cropped workspace with data below the specified x limit
 *
 *  @param workspace :: MatrixWorkspace
 *  @param x :: Maximum allowed x-value in the data.
 *  @return MatrixWorkspace cropped to values with x < specified limit.
 */
MatrixWorkspace_sptr PoldiTruncateData::getWorkspaceBelowX(MatrixWorkspace_sptr workspace, double x)
{
    Algorithm_sptr crop = getCropAlgorithmForWorkspace(workspace);
    crop->setProperty("XMax", x);

    return getOutputWorkspace(crop);
}

/** Returns a cropped workspace with data equal to and above the specified x limit
 *
 *  @param workspace :: MatrixWorkspace
 *  @param x :: Minimum allowed x-value in the data.
 *  @return MatrixWorkspace cropped to values with x >= specified limit.
 */
MatrixWorkspace_sptr PoldiTruncateData::getWorkspaceAboveX(MatrixWorkspace_sptr workspace, double x)
{
    Algorithm_sptr crop = getCropAlgorithmForWorkspace(workspace);
    crop->setProperty("Xmin", x);

    return getOutputWorkspace(crop);
}

/** Creates a CropWorkspace-algorithm for the given workspace
 *
 *  This method calls createChildAlgorithm() to create an instance of the CropWorkspace algorithm.
 *  If the creation is successful, the supplied workspace is set as InputParameter.
 *
 *  @param workspace :: MatrixWorkspace
 *  @return Pointer to crop algorithm.
 */
Algorithm_sptr PoldiTruncateData::getCropAlgorithmForWorkspace(MatrixWorkspace_sptr workspace)
{
    Algorithm_sptr crop = createChildAlgorithm("CropWorkspace");

    if(!crop) {
        throw std::runtime_error("Could not create CropWorkspace algorithm");
    }

    crop->setProperty("InputWorkspace", workspace);

    return crop;
}

/** Extracts OutputWorkspace property from supplied algorithm is present.
 *
 *  This methods executes the given algorithm and tries to extract the output workspace.
 *
 *  @param algorithm :: Pointer to algorithm.
 *  @return MatrixWorkspace stored in algorithm's OutputWorkspace property.
 */
MatrixWorkspace_sptr PoldiTruncateData::getOutputWorkspace(Algorithm_sptr algorithm)
{
    if(!algorithm || !algorithm->execute()) {
        throw std::runtime_error("Workspace could not be retrieved successfully.");
    }

    MatrixWorkspace_sptr outputWorkspace = algorithm->getProperty("OutputWorkspace");
    return outputWorkspace;
}

/** Returns a MatrixWorkspace with all spectrum summed up.
 *
 *  The summation is done with the SumSpectra-algorithm.
 *
 *  @param workspace :: MatrixWorkspace
 *  @return MatrixWorkspace with one spectrum which contains all counts.
 */
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
