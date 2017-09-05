#include <fstream>
#include <algorithm>
#include <stdexcept>
#include "MantidAlgorithms/Bin2DPowderDiffraction.h"
#include "MantidAPI/BinEdgeAxis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/SpectrumDetectorMapping.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/SpectraAxisValidator.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/FractionalRebinning.h"
#include "MantidDataObjects/RebinnedOutput.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/Math/ConvexPolygon.h"
#include "MantidGeometry/Math/PolygonIntersection.h"
#include "MantidGeometry/Math/Quadrilateral.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidAPI/HistogramValidator.h"

namespace Mantid {
namespace Algorithms {

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;
using namespace Mantid::HistogramData;


// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(Bin2DPowderDiffraction)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string Bin2DPowderDiffraction::name() const { return "Bin2DPowderDiffraction"; }

/// Algorithm's version for identification. @see Algorithm::version
int Bin2DPowderDiffraction::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string Bin2DPowderDiffraction::category() const {
    return "Diffraction\\Focussing";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string Bin2DPowderDiffraction::summary() const {
    return "Bins TOF powder diffraction event data in 2D space.";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void Bin2DPowderDiffraction::init() {
    auto wsValidator = boost::make_shared<CompositeValidator>();
    wsValidator->add<WorkspaceUnitValidator>("Wavelength");
    wsValidator->add<SpectraAxisValidator>();
    wsValidator->add<InstrumentValidator>();
    wsValidator->add<HistogramValidator>();

    declareProperty(make_unique<WorkspaceProperty<EventWorkspace>>(
                        "InputWorkspace", "", Direction::Input,
                        wsValidator),
                    "An input EventWorkspace must be a Histogram workspace, not Point data. "
                    "X-axis units must be wavelength.");

    declareProperty(
                make_unique<WorkspaceProperty<API::Workspace>>("OutputWorkspace", "",
                                                               Direction::Output),
                "An output workspace.");

    const std::string docString =
            "A comma separated list of first bin boundary, width, last bin boundary. "
            "Optionally "
            "this can be followed by a comma and more widths and last boundary "
            "pairs. "
            "Negative width values indicate logarithmic binning.";
    auto rebinValidator = boost::make_shared<RebinParamsValidator>(true);
    declareProperty(make_unique<ArrayProperty<double>>("Axis1Binning",
                                                       rebinValidator),
                    docString);
    declareProperty(make_unique<ArrayProperty<double>>("Axis2Binning",
                                                       rebinValidator),
                    docString);

    const std::vector<std::string> exts{".txt", ".dat"};
    declareProperty(make_unique<FileProperty>("BinEdgesFile", "",
                                              FileProperty::OptionalLoad, exts),
                    "Optional: The ascii file containing the list of bin edges. "
                    "Either this or Axis1- and Axis2Binning need to be specified.");

    declareProperty(
          Kernel::make_unique<PropertyWithValue<bool>>("NormalizeByBinArea", true),
          "Normalize the binned workspace by the bin area.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void Bin2DPowderDiffraction::exec() {
    m_inputWS = this->getProperty("InputWorkspace");
    m_numberOfSpectra = static_cast<int>(m_inputWS->getNumberHistograms());
    g_log.debug() << "Number of spectra in input workspace: " << m_numberOfSpectra
                  << "\n";

    MatrixWorkspace_sptr outputWS = createOutputWorkspace();

    const bool normalizeByBinArea = this->getProperty("NormalizeByBinArea");
    if (normalizeByBinArea)
        normalizeToBinArea(outputWS);

    setProperty("OutputWorkspace", outputWS);
}
//----------------------------------------------------------------------------------------------
/**
 * @brief Bin2DPowderDiffraction::validateInputs Validate inputs
 * @return
 */
std::map<std::string, std::string> Bin2DPowderDiffraction::validateInputs() {
    std::map<std::string, std::string> result;

    const auto useBinFile = !getPointerToProperty("BinEdgesFile")->isDefault();
    const auto useBinning1 = !getPointerToProperty("Axis1Binning")->isDefault();
    const auto useBinning2 = !getPointerToProperty("Axis2Binning")->isDefault();
    if (!useBinFile && !useBinning1 && !useBinning2) {
        const std::string msg = "You must specify either Axis1Binning and Axis2Binning, or a BinEdgesFile.";
        result["Axis1Binning"] = msg;
        result["Axis2Binning"] = msg;
        result["BinEdgesFile"] = msg;
    } else if (useBinFile && (useBinning1 || useBinning2)) {
        const std::string msg = "You must specify either Axis1Binning and Axis2Binning, or a BinEdgesFile, but not both.";
        result["BinEdgesFile"] = msg;
    }

    return result;
}
//----------------------------------------------------------------------------------------------
/**
 * @brief createOutputWorkspace create an output workspace and setup axis
 * @throw std::runtime_error If theta=0 or cos(theta)<=0
 * @return MatrixWorkspace with binned events
 */

MatrixWorkspace_sptr Bin2DPowderDiffraction::createOutputWorkspace() {

    using VectorHelper::createAxisFromRebinParams;
    bool binsFromFile(false);
    size_t newYSize = 0;
    size_t newXSize = 0;
    MatrixWorkspace_sptr outputWS;
    const auto &spectrumInfo = m_inputWS->spectrumInfo();

    const std::string beFileName = getProperty("BinEdgesFile");
    if (!beFileName.empty())
        binsFromFile=true;

    const auto &oldXEdges = m_inputWS->x(0);
    BinEdges newXBins(oldXEdges.size());
    BinEdges newYBins(oldXEdges.size());

    auto &newY = newYBins.mutableRawData();
    std::vector<std::vector<double>> fileXbins;

    // First create the output Workspace filled with zeros
    if (binsFromFile) {
        newY.clear();
        ReadBinsFromFile(newY, fileXbins);
        newYSize = newY.size();
        // unify xbins
        newXSize = UnifyXBins(fileXbins);
        g_log.debug() << "Maximal size of Xbins = " << newXSize;
        outputWS =  WorkspaceFactory::Instance().create(m_inputWS, newYSize-1, newXSize, newXSize-1);
        g_log.debug() << "Outws has " << outputWS->getNumberHistograms() << " histograms and " << outputWS->blocksize() << " bins." << std::endl;

        size_t idx = 0;
        for (const auto Xbins : fileXbins) {
            g_log.debug() << "Xbins size: " << Xbins.size() << std::endl;
            BinEdges binEdges (Xbins);
            outputWS->setBinEdges(idx, binEdges);
            idx++;
        }

    } else {
        static_cast<void>(createAxisFromRebinParams(getProperty("Axis1Binning"),
                                                    newXBins.mutableRawData()));
        HistogramData::BinEdges binEdges(newXBins);
        newYSize =
                createAxisFromRebinParams(getProperty("Axis2Binning"), newY);
        newXSize = binEdges.size();
        outputWS =  WorkspaceFactory::Instance().create(m_inputWS, newYSize - 1, newXSize, newXSize-1);
        for (size_t idx=0; idx<newYSize-1; idx++)
            outputWS->setBinEdges(idx, binEdges);
        NumericAxis *const abscissa = new BinEdgeAxis(newXBins.mutableRawData());
        outputWS->replaceAxis(0, abscissa);
    }

    outputWS->getAxis(0)->unit() = UnitFactory::Instance().create("dSpacing");

    NumericAxis *const verticalAxis = new BinEdgeAxis(newY);
    // Meta data
    verticalAxis->unit() = UnitFactory::Instance().create("dSpacingPerpendicular");
    verticalAxis->title() = "d_p";
    outputWS->replaceAxis(1, verticalAxis);

    Progress prog(this, 0.0, 1.0, m_numberOfSpectra);
    int64_t numSpectra = static_cast<int64_t>(m_numberOfSpectra);
    std::vector<std::vector<double>> newYValues(newYSize-1, std::vector<double>(newXSize-1, 0.0));
    std::vector<std::vector<double>> newEValues(newYSize-1, std::vector<double>(newXSize-1, 0.0));

    // fill the workspace with data
    g_log.debug() << "newYSize = " << newYSize << std::endl;
    g_log.debug() << "newXSize = " << newXSize << std::endl;
    std::vector<double> dp_vec (verticalAxis->getValues());

    PARALLEL_FOR_IF(Kernel::threadSafe(*m_inputWS, *outputWS))
    for (int64_t snum=0; snum<numSpectra; ++snum) {
        PARALLEL_START_INTERUPT_REGION
        if (!spectrumInfo.isMasked(snum)) {
        double theta = 0.5*spectrumInfo.twoTheta(snum);
        if (theta==0){
            throw std::runtime_error("Spectrum " + std::to_string(snum) + " has theta=0. Cannot calculate d-Spacing!");
        }
        if (cos(theta)<=0){
            throw std::runtime_error("Spectrum " + std::to_string(snum) + " has cos(theta) <= 0. Cannot calculate d-SpacingPerpendicular!");
        }
        EventList &evList = m_inputWS->getSpectrum(snum);

        // Switch to weighted if needed.
        if (evList.getEventType() == TOF)
            evList.switchTo(WEIGHTED);

        std::vector<WeightedEvent> events = evList.getWeightedEvents();

        for(const auto &ev:events){
            double d, dp;
            convertToDSpacing(ev.tof(), theta, &d, &dp);
            std::vector<double>::iterator upy = std::lower_bound(dp_vec.begin(), dp_vec.end(), dp);
            long int h_index = std::distance(dp_vec.begin(), upy) -1;
            if ((h_index < static_cast<int>(newYSize) -1) && h_index > -1) {
                if (h_index == static_cast<int>(newYSize)-1)
                    g_log.error("h_index is equal to the size of the Y axis!");
                auto xs = binsFromFile ? fileXbins[h_index] : newXBins.rawData();
                std::vector<double>::iterator lowx = std::lower_bound(xs.begin(), xs.end(), d);
                long int index = std::distance(xs.begin(), lowx) -1;
                if ((index <  static_cast<int>(newXSize-1)) && (index > -1)) {
                    // writing to the same vectors is not threat-safe
                    PARALLEL_CRITICAL(newValues) {
                        newYValues[h_index][index] += ev.weight();
                        newEValues[h_index][index] += ev.errorSquared();
                    }
                }

            }

        }
        }
        prog.report("Binning event data...");
        PARALLEL_END_INTERUPT_REGION

    }
    PARALLEL_CHECK_INTERUPT_REGION
    size_t idx=0;
    for (const auto &yVec: newYValues) {
        outputWS->setCounts(idx, yVec);
        idx++;
    }
    idx=0;
    for (auto &eVec: newEValues) {
        std::transform(eVec.begin(), eVec.end(), eVec.begin(),
                           static_cast<double (*)(double)>(sqrt));
        outputWS->setCountStandardDeviations(idx, eVec);
        idx++;
    }
    return outputWS;
}

//----------------------------------------------------------------------------------------------
/**
 * @brief Bin2DPowderDiffraction::ReadBinsFromFile
 * @param[out] Ybins vector of doubles to save the dSpacingPerpendicular bin edges
 * @param[out] Xbins vector of vectors of doubles to save the dSpacing bin edges
 */
void Bin2DPowderDiffraction::ReadBinsFromFile(std::vector<double> &Ybins, std::vector<std::vector<double> > &Xbins) const
{
    const std::string beFileName = getProperty("BinEdgesFile");
    std::ifstream file (beFileName);
    std::string line;
    std::string::size_type n;
    std::string::size_type sz;
    std::vector<double> tmp;
    int dpno = 0;
    while(getline(file, line)){
        n = line.find("dp =");
        if (n != std::string::npos) {
            if (!tmp.empty()){
                Xbins.push_back(tmp);
                tmp.clear();
            }
            double dp1 = std::stod (line.substr(4),&sz);        // 4 is needed to crop 'dp='
            double dp2 = std::stod (line.substr(sz + 4));
            if (dpno < 1){
                Ybins.push_back(dp1);
                Ybins.push_back(dp2);
            } else {
                Ybins.push_back(dp2);
            }
            dpno++;
        } else if(line.find("#")==std::string::npos) {
            std::stringstream ss(line);
            double d;
            while (ss >> d)
            {
                tmp.push_back(d);
            }

        }
    }
    Xbins.push_back(tmp);
    g_log.information() << "Number of Ybins: " <<  Ybins.size() << std::endl;
    g_log.information() << "Number of Xbins sets: " <<  Xbins.size() << std::endl;

}
//----------------------------------------------------------------------------------------------
/**
 * @brief Bin2DPowderDiffraction::UnifyXBins unifies size of the vectors in Xbins.
 * Just fills std::nans at the end of the shorter bins.
 * Required to avoid garbage values in the X values after ws->setHistogram.
 * returns the maximal size
 *
 * @param Xbins[in] --- bins to unify. Will be overwritten.
 */
size_t Bin2DPowderDiffraction::UnifyXBins(std::vector<std::vector<double>> &Xbins) const
{
    // get maximal vector size
    size_t max_size = 0;
    for (const auto& v : Xbins) {
            max_size = std::max(v.size(), max_size);
    }
    // resize all vectors to maximum size, fill last vector element at the end
    for (auto &v: Xbins) {
        if (v.size() < max_size)
            v.resize(max_size, v.back());
    }
    return max_size;

}

void Bin2DPowderDiffraction::normalizeToBinArea(MatrixWorkspace_sptr outWS)
{
    NumericAxis *verticalAxis = dynamic_cast<NumericAxis *>(outWS->getAxis(1));
    const std::vector<double> yValues = verticalAxis->getValues();
    auto nhist = outWS->getNumberHistograms();
    g_log.debug() << "Number of hists: " << nhist << " Length of YAxis: " << verticalAxis->length() << std::endl;

    for (size_t idx=0; idx<nhist; ++idx){
        double factor = 1.0/(yValues[idx+1] - yValues[idx]);
        // divide by the xBinWidth
        outWS->convertToFrequencies(idx);
        auto &freqs = outWS->mutableY(idx);
        std::transform(freqs.begin(), freqs.end(), freqs.begin(),
                       std::bind1st(std::multiplies<double>(), factor));
        auto &errors = outWS->mutableE(idx);
        std::transform(errors.begin(), errors.end(), errors.begin(),
                       std::bind1st(std::multiplies<double>(), factor));

    }
}

void convertToDSpacing(double wavelength, double theta, double *d, double *dp)
{
    *d = wavelength*0.5/sin(theta);
    *dp = sqrt(wavelength*wavelength - 2.0*log(cos(theta)));
}

} // namespace Algorithms
} // namespace Mantid
