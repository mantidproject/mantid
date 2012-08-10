#include "MantidCurveFitting/LeBailFit.h"
#include "MantidKernel/ListValidator.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/ParameterTie.h"
#include "MantidCurveFitting/Fit.h"
#include "MantidAPI/IFunction.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidCurveFitting/BackgroundFunction.h"
#include "MantidAPI/FunctionFactory.h"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>

namespace Mantid
{
namespace CurveFitting
{

#define PEAKRANGECONSTANT 5.0

bool compDescending(int a, int b)
{
    return (a >= b);
}

DECLARE_ALGORITHM(LeBailFit)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
LeBailFit::LeBailFit()
{
}
    
//----------------------------------------------------------------------------------------------
/** Destructor
 */
LeBailFit::~LeBailFit()
{
}
  
/*
 * Sets documentation strings for this algorithm
 */
void LeBailFit::initDocs()
{
    this->setWikiSummary("Do LeBail Fit to a spectrum of powder diffraction data.. ");
    this->setOptionalMessage("Do LeBail Fit to a spectrum of powder diffraction data. ");
}

/*
 * Define the input properties for this algorithm
 */
void LeBailFit::init()
{
    /// InputWorkspace
    this->declareProperty(new API::WorkspaceProperty<API::MatrixWorkspace>("InputWorkspace", "", Direction::Input),
      "Input workspace containing the data to fit by LeBail algorithm.");

    /// ParametersWorkspace
    this->declareProperty(new API::WorkspaceProperty<DataObjects::TableWorkspace>("ParametersWorkspace", "", Direction::InOut),
      "Input table workspace containing the parameters required by LeBail fit. ");

    /// ReflectionWorkspace
    this->declareProperty(new API::WorkspaceProperty<DataObjects::TableWorkspace>("ReflectionsWorkspace", "", Direction::InOut),
      "Input table workspace containing the list of reflections (HKL). ");

    /// WorkspaceIndex
    this->declareProperty("WorkspaceIndex", 0, "Workspace index of the spectrum to fit by LeBail.");

    /// Interested region
    this->declareProperty(new Kernel::ArrayProperty<double>("FitRegion"),
                          "Region of data (TOF) for LeBail fit.  Default is whole range. ");

    /// Function
    std::vector<std::string> functions;
    functions.push_back("LeBailFit");
    functions.push_back("Calculation");
    functions.push_back("AutoSelectBackgroundPoints");
    auto validator = boost::make_shared<Kernel::StringListValidator>(functions);
    this->declareProperty("Function", "LeBailFit", validator, "Functionality");

    /// Background type
    std::vector<std::string> bkgdtype;
    bkgdtype.push_back("Polynomial");
    bkgdtype.push_back("Chebyshev");
    auto bkgdvalidator = boost::make_shared<Kernel::StringListValidator>(bkgdtype);
    this->declareProperty("BackgroundType", "Polynomial", bkgdvalidator, "Background type");

    /// Input background parameters (array)
    this->declareProperty(new Kernel::ArrayProperty<double>("BackgroundParameters"),
                          "Optional: enter a comma-separated list of background order parameters from order 0. ");

    /// Input background parameters (tableworkspace)
    this->declareProperty(new API::WorkspaceProperty<DataObjects::TableWorkspace>("BackgroundParametersWorkspace", "", Direction::InOut, API::PropertyMode::Optional),
            "Optional table workspace containing the fit result for background.");

    /// OutputWorkspace
    this->declareProperty(new API::WorkspaceProperty<DataObjects::Workspace2D>("OutputWorkspace", "", Direction::Output),
                          "Output workspace containing calculated pattern or calculated background. ");

    /// PeaksWorkspace
    this->declareProperty(new API::WorkspaceProperty<DataObjects::TableWorkspace>("PeaksWorkspace", "", Direction::Output, API::PropertyMode::Optional),
                        "Optional output workspace containing the information for all peaks. ");

    /// UseInputPeakHeights
    this->declareProperty("UseInputPeakHeights", true,
                        "For function Calculation, use peak heights specified in ReflectionWorkspace.  Otherwise, calcualte peaks' heights. ");

    /// Peak Radius
    this->declareProperty("PeakRadius", 5, "Range (multiplier relative to FWHM) for a full peak. ");

    return;
}

/*
 * Implement abstract Algorithm methods
 */
void LeBailFit::exec()
{
    // 1. Get input
    dataWS = this->getProperty("InputWorkspace");
    parameterWS = this->getProperty("ParametersWorkspace");
    reflectionWS = this->getProperty("ReflectionsWorkspace");

    int tempindex = this->getProperty("WorkspaceIndex");
    if (tempindex < 0)
        throw std::invalid_argument("Input workspace index cannot be negative.");
    size_t workspaceindex = size_t(tempindex);

    mPeakRadius = this->getProperty("PeakRadius");

    g_log.debug() << "DB1113: Input Data(Workspace) Range: " << dataWS->dataX(workspaceindex)[0] << ", "
              << dataWS->dataX(workspaceindex).back() << std::endl;

    // 2. Determine Functionality
    std::string function = this->getProperty("Function");
    int functionmode = 0; // calculation
    if (function.compare("Calculation") == 0)
    {
        // peak calculation
        functionmode = 1;
    }
    else if (function.compare("AutoSelectBackgroundPoints") == 0)
    {
        // automatic background points selection
        functionmode = 2;
    }

    // 2B) Background
    std::string backgroundtype = this->getProperty("BackgroundType");
    std::vector<double> bkgdorderparams = this->getProperty("BackgroundParameters");
    DataObjects::TableWorkspace_sptr bkgdparamws = this->getProperty("BackgroundParametersWorkspace");
    if (!bkgdparamws)
    {
        std::cout << "Use background specified with vector. " << std::endl;
    }
    else
    {
        std::cout << "Use background specified by table workspace. " << std::endl;
        parseBackgroundTableWorkspace(bkgdparamws, bkgdorderparams);
    }
    generateBackgroundFunction(backgroundtype, bkgdorderparams);

    // 3. Check and/or process inputs
    if (workspaceindex >= dataWS->getNumberHistograms())
    {
        g_log.error() << "Input WorkspaceIndex " << workspaceindex << " is out of boundary [0, " <<
                         dataWS->getNumberHistograms() << ")" << std::endl;
        throw std::invalid_argument("Invalid input workspace index. ");
    }

    this->importParametersTable();
    this->importReflections();

    // 3. Create LeBail Function & initialize from input
    generatePeaksFromInput();

    // 4. Create CompositeFunction
    API::CompositeFunction compfunction;
    mLeBailFunction = boost::make_shared<API::CompositeFunction>(compfunction);

    std::map<int, CurveFitting::ThermalNeutronBk2BkExpConvPV_sptr>::iterator mit;
    for (mit = mPeaks.begin(); mit != mPeaks.end(); ++ mit)
    {
        mLeBailFunction->addFunction(mit->second);
    }
    mLeBailFunction->addFunction(mBackgroundFunction);

    g_log.information() << "LeBail Composite Function: " << mLeBailFunction->asString() << std::endl;

    // 5. Create output workspace
    size_t nspec = 1;
    if (functionmode == 0)
    {
        // Lebail Fit mode
        // (0) final calculated result (1) original data (2) difference
        // (3) first calculation
        nspec = 4;
    }
    else if (functionmode == 1)
    {
        // There will be Number(Peaks) + 1 spectrum in output workspace in calculation mode.
        // One spectrum for each peak
        nspec += mPeaks.size();
    }
    size_t nbin = dataWS->dataX(workspaceindex).size();
    outputWS = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
                API::WorkspaceFactory::Instance().create("Workspace2D", nspec, nbin, nbin));
    for (size_t i = 0; i < nbin; ++i)
    {
      outputWS->dataX(0)[i] = dataWS->readX(workspaceindex)[i];
      outputWS->dataY(0)[i] = dataWS->readY(workspaceindex)[i];
    }
    outputWS->getAxis(0)->setUnit("TOF");

    this->setProperty("OutputWorkspace", outputWS);

    // 6. Real work
    switch (functionmode)
    {
    case 0:
        // LeBail Fit
        g_log.notice() << "Do LeBail Fit." << std::endl;

        doLeBailFit(workspaceindex);

        break;

    case 1:
        // Calculation
        g_log.notice() << "Function: Pattern Calculation." << std::endl;
        calculatePattern(workspaceindex);
        break;

    case 2:
        // Background
        g_log.notice() << "Automatic background background selection. " << std::endl;
        g_log.error() << "To be implemented soon!" << std::endl;
        break;

    default:
        // Impossible
        g_log.warning() << "FunctionMode = " << functionmode <<".  It is not possible" << std::endl;
        break;
    }    

    // 7. Output peak (table) workspace
    createPeaksWorkspace();

    return;
}


/*
 * Calcualte LeBail diffraction pattern
 */
void LeBailFit::calculatePattern(size_t workspaceindex)
{
    // 1. Generate domain and value
    const std::vector<double> x = dataWS->readX(workspaceindex);
    API::FunctionDomain1DVector domain(x);
    API::FunctionValues values(domain);

    // 2. Calculate diffraction pattern
    bool useinputpeakheights = this->getProperty("UseInputPeakHeights");
    this->calculateDiffractionPattern(workspaceindex, domain, values, mFuncParameters, !useinputpeakheights);

    // 3. Retrieve and construct the output
    for (size_t i = 0; i < values.size(); ++i)
    {
        outputWS->dataY(0)[i] = values[i];
    }

    // 4. Do peak calculation for all peaks, and append to output workspace
    for (size_t ipk = 0; ipk < mPeakHKL2.size(); ++ipk)
    {
        int hkl2 = mPeakHKL2[ipk];
        CurveFitting::ThermalNeutronBk2BkExpConvPV_sptr peak = mPeaks[hkl2];
        if (!peak)
        {
            g_log.error() << "There is no peak corresponding to (HKL)^2 = " << hkl2 << std::endl;
        }
        else
        {
            peak->function(domain, values);
            for (size_t i = 0; i < domain.size(); ++i)
            {
                outputWS->dataX(ipk+1)[i] = domain[i];
            }
            for (size_t i = 0; i < values.size(); ++i)
            {
                outputWS->dataY(ipk+1)[i] = values[i];
            }
        }
    }

    return;
}

/*
 * LeBail Fitting for one self-consistent iteration
 */
void LeBailFit::doLeBailFit(size_t workspaceindex)
{
    // 1. Get a copy of input function parameters (map)
    std::map<std::string, std::pair<double, char> > parammap;
    parammap = mFuncParameters;

    // 2. Do 1 iteration of LeBail fit
    this->unitLeBailFit(workspaceindex, parammap);

    // 3. Output
    exportParametersWorkspace(parammap);

    return;
}


/*
 * Calculate diffraction pattern from a more fexible parameter map
 */
void LeBailFit::calculateDiffractionPattern(
        size_t workspaceindex, API::FunctionDomain1DVector domain, API::FunctionValues& values,
        std::map<std::string, std::pair<double, char> > parammap, bool recalpeakintesity)
{
    // 1. Set parameters to each peak
    std::map<int, CurveFitting::ThermalNeutronBk2BkExpConvPV_sptr>::iterator pit;
    for (pit = mPeaks.begin(); pit != mPeaks.end(); ++pit)
    {
        int hkl2 = pit->first;
        double peakheight = mPeakHeights[hkl2];
        setPeakParameters(mPeaks[hkl2], parammap, peakheight);
    }

    // 2. Calculate peak intensities
    if (recalpeakintesity)
    {
        std::vector<std::pair<int, double> > peakheights;
        this->calPeaksIntensities(peakheights, workspaceindex);

        for (size_t ipk = 0; ipk < peakheights.size(); ++ipk)
        {
            int hkl2 = peakheights[ipk].first;
            std::cout << "Peak w/ HKL^2 = " << hkl2 << "  Intensity = " << peakheights[ipk].second << std::endl;

            CurveFitting::ThermalNeutronBk2BkExpConvPV_sptr peak = mPeaks[hkl2];
            if (!peak)
            {
                g_log.error() << "No peak corresponds to (HKL)^2 = " << hkl2 << std::endl;
            }
            else
            {
                peak->setParameter("Height", peakheights[ipk].second);
            }
        }
    } // Re-calcualte peak intensity

    // 3. Calcualte model pattern
    mLeBailFunction->function(domain, values);

    return;
}

/*
 * Perform one itearation of LeBail fitting
 * Including
 * a) Calculate pattern for peak intensities
 * b) Set peak intensities
 */
bool LeBailFit::unitLeBailFit(size_t workspaceindex, std::map<std::string, std::pair<double, char> >& parammap)
{
    // 1. Generate domain and value
    const std::vector<double> x = dataWS->readX(workspaceindex);
    API::FunctionDomain1DVector domain(x);
    API::FunctionValues values(domain);

    // 2. Calculate peak intensity and etc.
    bool calpeakintensity = true;
    this->calculateDiffractionPattern(workspaceindex, domain, values, parammap, calpeakintensity);

    // 2.5 Apply initial calculated result to output workspace
    mWSIndexToWrite = 3;
    writeToOutputWorkspace(domain, values);

    // 3. Construct the tie.  2-level loop. (1) peak parameter (2) peak
    std::map<std::string, std::pair<double, char> >::iterator pariter;
    for (pariter = mFuncParameters.begin(); pariter != mFuncParameters.end(); ++pariter)
    {
        std::string parname = pariter->first;
        double parvalue = pariter->second.first;
        char fitortie = pariter->second.second;

        if (fitortie == 't')
        {
            // a) Tie the value to a constant number
            std::map<int, CurveFitting::ThermalNeutronBk2BkExpConvPV_sptr>::iterator piter;
            size_t peakindex = 0;
            for (piter = mPeaks.begin(); piter != mPeaks.end(); ++piter)
            {
                std::stringstream ss1, ss2;
                ss1 << "f" << peakindex << "." << parname;
                ss2 << parvalue;
                std::string tiepart1 = ss1.str();
                std::string tievalue = ss2.str();
                mLeBailFunction->tie(tiepart1, tievalue);
                g_log.information() << "LeBailFit.  Tie / " << tiepart1 << " / " << tievalue << " /" << std::endl;

                ++ peakindex;
            } // For each peak
        }
        else
        {
            // b) Tie the values among all peaks, but will fit
            for (size_t ipk = 1; ipk < mPeaks.size(); ++ipk)
            {
                std::stringstream ss1, ss2;
                ss1 << "f" << (ipk-1) << "." << parname;
                ss2 << "f" << ipk << "." << parname;
                std::string tiepart1 = ss1.str();
                std::string tiepart2 = ss2.str();
                mLeBailFunction->tie(tiepart1, tiepart2);
                g_log.information() << "LeBailFit.  Fit(Tie) / " << tiepart1 << " / " << tiepart2 << " /" << std::endl;
            }
        }
    }

    // 4. Construct the Fit
    double tof_min = dataWS->dataX(workspaceindex)[0];
    double tof_max = dataWS->dataX(workspaceindex).back();
    std::vector<double> fitrange = this->getProperty("FitRegion");
    if (fitrange.size() == 2 && fitrange[0] < fitrange[1])
    {
        // Properly defined
        tof_min = fitrange[0];
        tof_max = fitrange[1];
    }

    // a) Initialize
    API::IAlgorithm_sptr fitalg = this->createSubAlgorithm("Fit", 0.0, 0.2, true);
    fitalg->initialize();

    g_log.information() << "Function To Fit: " << mLeBailFunction->asString() << std::endl;

    // b) Set property
    fitalg->setProperty("Function", boost::shared_ptr<API::IFunction>(mLeBailFunction));
    fitalg->setPropertyValue("InputWorkspace", dataWS->name());
    fitalg->setProperty("WorkspaceIndex", int(workspaceindex));
    fitalg->setProperty("StartX", tof_min);
    fitalg->setProperty("EndX", tof_max);
    fitalg->setProperty("Minimizer", "Levenberg-MarquardtMD");
    fitalg->setProperty("CostFunction", "Least squares");
    fitalg->setProperty("MaxIterations", 100);

    // c) Execute
    bool successfulfit = fitalg->execute();
    if (!fitalg->isExecuted() || ! successfulfit)
    {
        // Early return due to bad fit
        g_log.error() << "Fitting to LeBail function failed. " << std::endl;
        return false;
    }

    double chi2 = fitalg->getProperty("OutputChi2overDoF");
    std::string fitstatus = fitalg->getProperty("OutputStatus");

    g_log.notice() << "LeBailFit (LeBailFunction) Fit result:  Chi^2 = " << chi2
                   << " Fit Status = " << fitstatus << std::endl;

    // b) Get parameters
    API::IFunction_sptr fitout = fitalg->getProperty("Function");

    std::vector<std::string> parnames = fitout->getParameterNames();
    std::sort(parnames.begin(), parnames.end());

    std::vector<std::string>::iterator nit;
    for (nit = parnames.begin(); nit != parnames.end(); ++ nit)
    {
        std::string parname = *nit;
        double curvalue = fitout->getParameter(parname);

        // split parameter string
        std::vector<std::string> results;
        boost::split(results, parname, boost::is_any_of("."));

        if (results.size() != 2)
        {
            g_log.error() << "Parameter name : " << parname << " does not have 1 and only 1 (.).  Cannot support!" << std::endl;
            throw std::runtime_error("Unable to support parameter name to split.");
        }

        if (results[0].compare("f0") == 0)
        {
            g_log.debug() << "DB216 Parameter " << results[1] << ": " << curvalue << std::endl;

            // Set to parammap
            if (parammap[results[1]].second == 'f')
            {
                // Fit
                parammap[results[1]] = std::make_pair(curvalue, 'f');
                g_log.information() << "DB1105 Fit Parameter " << results[1] << " To  " << curvalue << std::endl;
            }
        }
    }

    // 5. Do calculation again and set the output
    calpeakintensity = true;
    API::FunctionValues newvalues(domain);
    this->calculateDiffractionPattern(workspaceindex, domain, newvalues, parammap, calpeakintensity);

    // Add final calculated value to output workspace
    mWSIndexToWrite = 0;
    writeToOutputWorkspace(domain, newvalues);

    // Add original data and
    writeInputDataNDiff(workspaceindex, domain);

    return true;
}

/*
 * Calculate peak heights from the model to the observed data
 * Algorithm will deal with
 * (1) Peaks are close enough to overlap with each other
 * The procedure will be
 * (a) Assign peaks into groups; each group contains either (1) one peak or (2) peaks overlapped
 * (b) Calculate peak intensities for every peak per group
 */
void LeBailFit::calPeaksIntensities(std::vector<std::pair<int, double> >& peakheights, size_t workspaceindex)
{
    std::cout << "--------- Calculate (ALL) Peaks' Heights" << std::endl;

    // 0. Prepare
    peakheights.clear();

    // 1) Sort peaks list by HKL^2:
    //    FIXME: This sorting scheme may be brokend, if the crystal is not cubical!
    std::cout << "DBX122 PeakHKL2 Size = " << mPeakHKL2.size() << std::endl;
    std::sort(mPeakHKL2.begin(), mPeakHKL2.end(), CurveFitting::compDescending);

    // 1. Calculate the FWHM of each peak: Only peakcenterpairs is in order of peak position.
    //    Others are in input order of peaks
    std::vector<double> peakfwhms;
    std::vector<double> peakcenters;
    std::vector<std::pair<double, double> > peakboundaries;
    std::vector<std::pair<double, size_t> > peakcenterpairs;

    // 2. Obtain each peak's center and range from calculation
    //    Must be in the descending order of HKL2 for adjacent peaks
    for (size_t ipk = 0; ipk < mPeakHKL2.size(); ++ipk)
    {
        int hkl2 = mPeakHKL2[ipk]; // key
        double fwhm = mPeaks[hkl2]->fwhm();
        double center = mPeaks[hkl2]->centre();

        /* Give up using observation result due to some peak with narrow peak width
        double tof_left, tof_right, tof_center;
        this->estimatePeakRange(workspaceindex, center, fwhm, tof_center, tof_left, tof_right);
        */

        double tof_left = center - fwhm;
        double tof_right = center + fwhm;

        peakfwhms.push_back(fwhm);
        peakcenters.push_back(center);
        peakcenterpairs.push_back(std::make_pair(center, ipk));
        peakboundaries.push_back(std::make_pair(tof_left, tof_right));

        g_log.debug() << "DB1659 Peak " << ipk << ":  FWHM = " << fwhm << " @ TOF = " << center << std::endl;
    }

    // 3. Regroup peaks. record peaks in groups; peaks in same group are very close
    std::vector<std::set<size_t> > peakgroups; // index is for peak groups.  Inside, are indices for mPeakHKL2
    std::set<size_t> peakindices;
    // FIXME: This should be a more flexible number
    double boundaryconst = double(mPeakRadius);

    for (size_t ix = 0; ix < peakcenters.size(); ++ix)
    {
      // Note: ix is only bounded to peakcenterpairs
      size_t ipk = peakcenterpairs[ix].second;

      if (peakindices.size() > 0)
      {
        size_t leftpeakindex = peakcenterpairs[ix-1].second;
        double leftpeakcenter = peakcenterpairs[ix-1].first;
        double leftpeakrange = peakboundaries[leftpeakindex].second - leftpeakcenter;
        double leftpeak_rightbound =
            leftpeakcenter + boundaryconst * leftpeakrange;

        double thispeak_leftbound =
            peakcenterpairs[ix].first - boundaryconst * (peakcenterpairs[ix].first - peakboundaries[ipk].first);

        if (thispeak_leftbound > leftpeak_rightbound)
        {
          // current peak has no overlap with previous peak, start a new peak group
          std::set<size_t> settoinsert = peakindices;
          peakgroups.push_back(settoinsert);
          peakindices.clear();
        }
      }

      // Insert the current peak index to set
      peakindices.insert(ipk);
    } // ENDFOR

    // Insert the last group
    peakgroups.push_back(peakindices);

    std::cout << "LeBailFit:  Size(Peak Groups) = " << peakgroups.size() << std::endl;

    // 4. Calculate each peak's intensity and set
    std::vector<std::pair<size_t, double> > peakintensities;
    for (size_t ig = 0; ig < peakgroups.size(); ++ig)
    {
        std::cout << "Group " << ig << ": Number of Peaks = " << peakgroups[ig].size() << std::endl;

        std::vector<std::pair<size_t, double> > tempintensities; // index is mPeakHKL2's
        this->calPerGroupPeaksIntensities(workspaceindex, peakgroups[ig], peakcenters, peakboundaries, tempintensities);

        // b) Sort in order of mPeakHKL2
        std::sort(tempintensities.begin(), tempintensities.end());
        for (size_t ipk = 0; ipk < tempintensities.size(); ++ipk)
        {
            int hkl2 = mPeakHKL2[tempintensities[ipk].first];
            double height = tempintensities[ipk].second;
            peakheights.push_back(std::make_pair(hkl2, height));
        }

        peakintensities.insert(peakintensities.end(), tempintensities.begin(), tempintensities.end());
    }

    return;
}

/*
 * Calculate peak intensities for each group of peaks
 * Input: (1) Workspace Index (2) Peaks' indicies in mPeakHKL2  (3) Peaks' centers
 *        (4) Peaks' boundaries
 * Output: Peak intensities (index is peaks' indicies in mPeakHKL2)
 */
void LeBailFit::calPerGroupPeaksIntensities(size_t wsindex, std::set<size_t> peakindices, std::vector<double> peakcenters,
                                   std::vector<std::pair<double, double> > peakboundaries, std::vector<std::pair<size_t, double> >& peakintensities)
{
    std::cout << "----------- calPerGroupPeakIntensity() ----------" << std::endl;

    // 1. Determine range of the peak group
    std::cout << "DB252 Group Size = " << peakindices.size() << " Including peak indexed " << std::endl;

    peakintensities.clear();

    std::vector<size_t> peaks; // index for mPeakHKL2
    std::set<size_t>::iterator pit;
    for (pit = peakindices.begin(); pit != peakindices.end(); ++pit)
    {
        peaks.push_back(*pit);
        std::cout << "Peak index = " << *pit << std::endl;
    }

    if (peaks.size() > 1)
        std::sort(peaks.begin(), peaks.end());

    size_t iLeftPeak = peaks[0];
    double leftpeakcenter = peakcenters[iLeftPeak];
    double mostleftpeakwidth = -peakboundaries[iLeftPeak].first+peakcenters[iLeftPeak];
    double leftbound = leftpeakcenter-PEAKRANGECONSTANT*mostleftpeakwidth;

    size_t iRightPeak = peaks.back();
    double rightpeakcenter = peakcenters[iRightPeak];
    double mostrightpeakwidth = peakboundaries[iRightPeak].second-peakcenters[iRightPeak];
    double rightbound = rightpeakcenter+PEAKRANGECONSTANT*mostrightpeakwidth;

    std::cout << "DB1204 Left bound = " << leftbound << " (" << iLeftPeak << "), Right bound = " << rightbound << "(" << iRightPeak << ")."
                 << std::endl;

    // 1.5 Return if the complete peaks' range is out side of all data (boundary)
    if (leftbound < dataWS->readX(wsindex)[0] || rightbound > dataWS->readX(wsindex).back())
    {
        for (size_t i = 0; i < peaks.size(); ++i)
        {
            peakintensities.push_back(std::make_pair(peaks[i], 0.0));
        }
        return;
    }

    // 2. Obtain access of dataWS to calculate from
    const MantidVec& datax = dataWS->readX(wsindex);
    const MantidVec& datay = dataWS->readY(wsindex);
    std::vector<double>::const_iterator cit;

    cit = std::lower_bound(datax.begin(), datax.end(), leftbound);
    size_t ileft = size_t(cit-datax.begin());
    if (ileft > 0)
        ileft -= 1;

    cit = std::lower_bound(datax.begin(), datax.end(), rightbound);
    size_t iright = size_t(cit-datax.begin());
    if (iright < datax.size()-1)
        iright += 1;

    if (iright <= ileft)
    {
        g_log.error() << "Try to integrate peak from " << leftbound << " To " << rightbound << std::endl <<
                         "  Peak boundaries : " << peakboundaries[peaks[0]].first << ", " << peakboundaries[peaks[0]].second <<
                         "  Peak center: " << peakcenters[peaks[0]] << "  ... " << peakcenters[peaks.back()] << std::endl;
        throw std::logic_error("iRight cannot be less or equal to iLeft.");
    }
    else
    {
        std::cout << "DB452 Integrate peak from " << leftbound << "/" <<
                         ileft << " To " << rightbound << "/" << iright << std::endl;
    }

    // 3. Integrate
    size_t ndata = iright - ileft + 1;
    std::vector<double>::const_iterator xbegin = datax.begin()+ileft;
    std::vector<double>::const_iterator xend;
    if (iright+1 <= datax.size()-1)
        xend = datax.begin()+iright+1;
    else
        xend = datax.end();
    std::vector<double> reddatax(xbegin, xend); // reduced-size data x

    std::cout << "DBx356:  ndata = " << ndata << " Reduced data range: " << reddatax[0] << ", " << reddatax.back() << std::endl;

    API::FunctionDomain1DVector xvalues(reddatax);
    std::vector<double> sumYs;
    sumYs.reserve(xvalues.size());

    for (size_t iy = 0; iy < xvalues.size(); ++iy)
    {
        sumYs.push_back(0.0);
    }

    std::vector<API::FunctionValues> peakvalues;

    for (size_t i = 0; i < peaks.size(); ++i)
    {
        size_t peakindex = peaks[i];
        int hkl2 = mPeakHKL2[peakindex];
        std::cout << "DBx359  Peak " << peakindex << ": (HKL)^2 = " << hkl2 << std::endl;
        CurveFitting::ThermalNeutronBk2BkExpConvPV_sptr ipeak = mPeaks[hkl2];
        if (!ipeak)
            throw std::runtime_error("Not a peak function at all. ");
        API::FunctionValues yvalues(xvalues);

        ipeak->function(xvalues, yvalues);
        peakvalues.push_back(yvalues);

        for (size_t j = 0; j < ndata; ++j)
        {
            sumYs[j] += yvalues[j];
        }
    }

    // 3. Calculate intensity for each peak
    for (size_t i = 0; i < peaks.size(); ++i)
    {
        double intensity = 0.0;
        for (size_t j = 0; j < sumYs.size(); ++j)
        {
            if (sumYs[j] > 1.0E-5)
            {
                double temp = datay[ileft+j]*peakvalues[i][j]/sumYs[j];
                intensity += temp*(datax[ileft+j+1]-datax[ileft+j]);
            }
        }
        peakintensities.push_back(std::make_pair(peaks[i], intensity));
        std::cout << "DBx406 Result Per Group: Peak " << peaks[i] << "  Height = " << intensity << std::endl;
    }

    return;
}

/*
 * Estimate peak center and peak range according to input information (from observation)
 * - center: user input peak center
 * - fwhm: user input fwhm
 * - tof_center: estimated peak center (output)
 * - tof_left: estimated left boundary at half maximum
 * - tof_right: estimated right boundary at half maximum
 * Return: False if no peak found (maximum value is at center+/-fwhm
 *
 * WARNING: This algorithm fails if the peak only has the width of very few pixels.
 *          because it heavily replies on observation data.
 */
bool LeBailFit::observePeakRange(size_t workspaceindex, double center, double fwhm,
    double& tof_center, double& tof_left, double& tof_right)
{
    // 0. Get access to data
    const MantidVec& datax = dataWS->readX(workspaceindex);
    const MantidVec& datay = dataWS->readY(workspaceindex);

    // 1. Find index of center
    std::vector<double>::const_iterator cit;
    cit = std::lower_bound(datax.begin(), datax.end(), center-fwhm);
    size_t ileft = size_t(cit-datax.begin());
    if (ileft > 0)
        ileft --;
    cit = std::lower_bound(datax.begin(), datax.end(), center+fwhm);
    size_t iright = size_t(cit-datax.begin());

    std::cout << "DBX315 Estimate Peak Range:  iLeft = " << ileft << " For " << center-fwhm
              << ", iRight = " << iright << " For " << center+fwhm
              << ".  Data Range: " << datax[0] << " to " << datax.back() << std::endl;

    // 2. Find maximum
    double maxh = 0;
  size_t icenter = ileft;
  for (size_t i = ileft; i <= iright; ++i)
  {
    if (datay[i] > maxh)
    {
      icenter = i;
      maxh = datay[i];
    }
  }
  if (icenter == ileft || icenter == iright)
  {
    g_log.error() << "Designated peak @ TOF = " << center << " cannot be located within user input center+/-fwhm = "
        << fwhm << std::endl;
    tof_center = -0.0;
    tof_left = center-fwhm;
    tof_right = center+fwhm;

    return false;
  }

  // 3. Find half maximum
  double halfmax = 0.5*maxh;

  // a) Find left boundary
  size_t itof = icenter-1;
  bool continuesearch = true;
  bool cannotfindL = false;
  while (continuesearch)
  {
    if (datay[itof] <= halfmax && datay[itof+1] > halfmax)
    {
      // Find it!
      ileft = itof;
      continuesearch = false;
    }
    else if (datay[itof] > datay[itof+1])
    {
      // Min value exceeds half maximum
      cannotfindL = true;
      ileft = itof+1;
      continuesearch = false;
    }
    else if (itof == 0)
    {
      // Impossible situation
      cannotfindL = true;
      ileft = itof;
      continuesearch = false;
    }
    itof --;
  }

  // b) Find right boundary
  // a) Find left boundary
  itof = icenter+1;
  continuesearch = true;
  bool cannotfindR = false;
  while (continuesearch)
  {
    if (datay[itof] <= halfmax && datay[itof-1] > halfmax)
    {
      // Find it!
      iright = itof;
      continuesearch = false;
    }
    else if (datay[itof] > datay[itof-1])
    {
      // Min value exceeds half maximum
      cannotfindR = true;
      iright = itof-1;
      continuesearch = false;
    }
    else if (itof == datax.size()-1)
    {
      // Impossible situation
      cannotfindR = true;
      iright = itof;
      continuesearch = false;
    }
    itof ++;
  }

  tof_center = datax[icenter];
  tof_left = datax[ileft]+(datax[ileft+1]-datax[ileft])*(halfmax-datay[ileft])/(datay[ileft+1]-datay[ileft]);
  tof_right = datax[iright]-(datax[iright]-datax[iright-1])*(halfmax-datay[iright])/(datay[iright-1]-datay[iright]);

  g_log.information() << "DB502 Estimate Peak Range:  Center = " << tof_center << ";  Left = " << tof_left << ", Right = " << tof_right << std::endl;

  return (!cannotfindL && !cannotfindR);
}


/*
 * Create and set up an output TableWorkspace for all the peaks.
 */
void LeBailFit::createPeaksWorkspace()
{
    // 1. Create peaks workspace
    DataObjects::TableWorkspace tbws;
    DataObjects::TableWorkspace_sptr peakWS = boost::make_shared<DataObjects::TableWorkspace>(tbws);

    // 2. Set up peak workspace
    peakWS->addColumn("int", "H");
    peakWS->addColumn("int", "K");
    peakWS->addColumn("int", "L");
    peakWS->addColumn("double", "height");
    peakWS->addColumn("double", "TOF_h");

    // 3. Construct a list
    std::sort(mPeakHKL2.begin(), mPeakHKL2.end(), compDescending);

    for (size_t ipk = 0; ipk < mPeakHKL2.size(); ++ipk)
    {
        int hkl2 = mPeakHKL2[ipk];
        CurveFitting::ThermalNeutronBk2BkExpConvPV_sptr tpeak = mPeaks[hkl2];
        int h, k, l;
        tpeak->getMillerIndex(h, k, l);
        double tof_h = tpeak->centre();
        double height = tpeak->height();

        API::TableRow newrow = peakWS->appendRow();
        newrow << h << k << l << height << tof_h;
    }

    // 4. Set
    this->setProperty("PeaksWorkspace", peakWS);

    return;
}


/*
 * From table/map to set parameters to an individual peak
 */
void LeBailFit::setPeakParameters(
        CurveFitting::ThermalNeutronBk2BkExpConvPV_sptr peak,
        std::map<std::string, std::pair<double, char> > parammap, double peakheight)
{
    // 1. Set parameters ...
    std::map<std::string, std::pair<double, char> >::iterator pit;

    std::vector<std::string> lebailparnames = peak->getParameterNames();
    std::sort(lebailparnames.begin(), lebailparnames.end());

    // 2. Apply parameters values to peak function
    for (pit = parammap.begin(); pit != parammap.end(); ++pit)
    {
        std::string parname = pit->first;
        double value = pit->second.first;

        // char fitortie = pit->second.second;

        g_log.debug() << "LeBailFit Set " << parname << "= " << value << std::endl;

        std::vector<std::string>::iterator ifind =
                std::find(lebailparnames.begin(), lebailparnames.end(), parname);
        if (ifind == lebailparnames.end())
        {
            g_log.warning() << "Parameter " << parname << " in input parameter table workspace is not for peak function. " << std::endl;
            continue;
        }

        peak->setParameter(parname, value);

    } // ENDFOR: parameter iterator

    // 3. Peak height
    peak->setParameter("Height", peakheight);

    return;
}

/*
 * Generate a list of peaks from input
 */
void LeBailFit::generatePeaksFromInput()
{
    // There is no need to consider peak's order now due to map
    for (size_t ipk = 0; ipk < mPeakHKLs.size(); ++ipk)
    {
        CurveFitting::ThermalNeutronBk2BkExpConvPV tmppeak;
        int h = mPeakHKLs[ipk][0];
        int k = mPeakHKLs[ipk][1];
        int l = mPeakHKLs[ipk][2];
        tmppeak.setMillerIndex(h, k, l);
        tmppeak.initialize();
        CurveFitting::ThermalNeutronBk2BkExpConvPV_sptr speak = boost::make_shared<CurveFitting::ThermalNeutronBk2BkExpConvPV>(tmppeak);

        int hkl2 = h*h+k*k+l*l;
        mPeaks.insert(std::make_pair(hkl2, speak));

        speak->setPeakRadius(mPeakRadius);
    }

    return;
}

/*
 * Generate background function accroding to input
 */
void LeBailFit::generateBackgroundFunction(std::string backgroundtype, std::vector<double> bkgdparamws)
{
    auto background = API::FunctionFactory::Instance().createFunction(backgroundtype);
    mBackgroundFunction = boost::dynamic_pointer_cast<CurveFitting::BackgroundFunction>(background);

    // CurveFitting::BackgroundFunction_sptr mBackgroundFunction = boost::make_shared<CurveFitting::BackgroundFunction>(background);
    //            boost::dynamic_pointer_cast<CurveFitting::BackgroundFunction>(
    //            boost::make_shared<(background));
    size_t order = bkgdparamws.size();
    std::cout << "-----------   polynomial order = " << order << std::endl;

    mBackgroundFunction->setAttributeValue("n", int(order));
    mBackgroundFunction->initialize();

    for (size_t i = 0; i < order; ++i)
    {
        std::stringstream ss;
        ss << "A" << i;
        std::string parname = ss.str();

        background->setParameter(parname, bkgdparamws[i]);
    }

    // FIXME Requires a background shared_pointer

    return;
}

/*
 * Parse table workspace (from Fit()) containing background parameters to a vector
 */
void LeBailFit::parseBackgroundTableWorkspace(DataObjects::TableWorkspace_sptr bkgdparamws, std::vector<double>& bkgdorderparams)
{
    // 1. Clear (output) map
    bkgdorderparams.clear();
    std::map<std::string, double> parmap;

    // 2. Check
    std::vector<std::string> colnames = bkgdparamws->getColumnNames();
    if (colnames.size() < 2)
    {
        g_log.error() << "Input parameter table workspace must have more than 1 columns" << std::endl;
        throw std::invalid_argument("Invalid input background table workspace. ");
    }
    else
    {
        if (!(boost::starts_with(colnames[0], "Name") && boost::starts_with(colnames[1], "Value")))
        {
            // Column 0 and 1 must be Name and Value (at least started with)
            g_log.error() << "Input parameter table workspace have wrong column definition." << std::endl;
            for (size_t i = 0; i < 2; ++i)
                g_log.error() << "Column " << i << " Should Be Name.  But Input is " << colnames[0] << std::endl;
            throw std::invalid_argument("Invalid input background table workspace. ");
        }
    }

    // 3. Input
    for (size_t ir = 0; ir < bkgdparamws->rowCount(); ++ir)
    {
        API::TableRow row = bkgdparamws->getRow(ir);
        std::string parname;
        double parvalue;
        row >> parname >> parvalue;

        if (parname.size() > 0 && parname[0] == 'A')
        {
            // Insert parameter name starting with A
            parmap.insert(std::make_pair(parname, parvalue));
        }
    }

    // 4. Sort: increasing order
    bkgdorderparams.reserve(parmap.size());
    for (size_t i = 0; i < parmap.size(); ++i)
    {
        bkgdorderparams.push_back(0.0);
    }

    std::map<std::string, double>::iterator mit;
    for (mit = parmap.begin(); mit != parmap.end(); ++mit)
    {
        std::string parname = mit->first;
        double parvalue = mit->second;
        std::vector<std::string> terms;
        boost::split(terms, parname, boost::is_any_of("A"));
        int tmporder = atoi(terms[1].c_str());
        bkgdorderparams[tmporder] = parvalue;
    }

    return;
}

/*
 * Parse the input TableWorkspace to some maps for easy access
 */
void LeBailFit::importParametersTable()
{
  // 1. Check column orders
  std::vector<std::string> colnames = parameterWS->getColumnNames();
  if (colnames.size() < 3)
  {
    g_log.error() << "Input parameter table workspace does not have enough number of columns. "
        << " Number of columns = " << colnames.size() << " < 3 as required. " << std::endl;
    throw std::runtime_error("Input parameter workspace is wrong. ");
  }

  if (colnames[0].compare("Name") != 0 ||
      colnames[1].compare("Value") != 0 ||
      colnames[2].compare("FitOrTie") != 0)
  {
    g_log.error() << "Input parameter table workspace does not have the columns in order.  "
        << " It must be Name, Value, FitOrTie." << std::endl;
    throw std::runtime_error("Input parameter workspace is wrong. ");
  }

  // 2. Import data to maps
  std::string parname, fitortie;
  double value;

  size_t numrows = parameterWS->rowCount();

  for (size_t ir = 0; ir < numrows; ++ir)
  {
    API::TableRow trow = parameterWS->getRow(ir);
    trow >> parname >> value >> fitortie;
    // fit or tie?
    char tofit = 'f';
    if (fitortie.length() > 0)
    {
      char fc = fitortie.c_str()[0];
      if (fc == 't' || fc == 'T')
      {
        tofit = 't';
      }
    }
    mFuncParameters.insert(std::make_pair(parname, std::make_pair(value, tofit)));
  }

  return;
}

/*
 * Parse the reflections workspace to a list of reflections;
 */
void LeBailFit::importReflections()
{
    // 1. Check column orders
    std::vector<std::string> colnames = reflectionWS->getColumnNames();
    if (colnames.size() < 3)
  {
    g_log.error() << "Input parameter table workspace does not have enough number of columns. "
        << " Number of columns = " << colnames.size() << " < 3 as required. " << std::endl;
    throw std::runtime_error("Input parameter workspace is wrong. ");
  }
    if (colnames[0].compare("H") != 0 ||
            colnames[1].compare("K") != 0 ||
            colnames[2].compare("L") != 0)
  {
    g_log.error() << "Input parameter table workspace does not have the columns in order.  "
        << " It must be H, K, L." << std::endl;
    throw std::runtime_error("Input parameter workspace is wrong. ");
  }

    bool hasPeakHeight = false;
    if (colnames.size() >= 4)
    {
        // Has a column for peak height
        hasPeakHeight = true;
    }

    // 2. Import data to maps
    int h, k, l;

    size_t numrows = reflectionWS->rowCount();
    std::cout << "DBX123 Import Reflection Table.  Size of Rows = " << numrows << std::endl;
    for (size_t ir = 0; ir < numrows; ++ir)
    {
        // 1. Get from table row
        API::TableRow trow = reflectionWS->getRow(ir);
        trow >> h >> k >> l;

        // 2. Check whether this (hkl)^2 exits.  Throw exception if it exist
        int hkl2 = h*h + k*k + l*l;
        std::vector<int>::iterator fit = std::find(mPeakHKL2.begin(), mPeakHKL2.end(), hkl2);
        if (fit != mPeakHKL2.end())
        {
            g_log.error() << "H^2+K^2+L^2 = " << hkl2 << " already exists. This situation is not considered" << std::endl;
            throw std::invalid_argument("2 reflections have same H^2+K^2+L^2, which is not supported.");
        }
        else
        {
            mPeakHKL2.insert(fit, hkl2);
        }

        // 3. Insert related data structure
        std::vector<int> hkl;
        hkl.push_back(h);
        hkl.push_back(k);
        hkl.push_back(l);
        mPeakHKLs.push_back(hkl);

        // optional peak height
        double peakheight = 1.0;
        if (hasPeakHeight)
        {
            trow >> peakheight;
        }

        mPeakHeights.insert(std::make_pair(hkl2, peakheight));
  } // ENDFOR row

  return;
}

/*
 * Write data (domain, values) to one specified spectrum of output workspace
 */
void LeBailFit::writeToOutputWorkspace(API::FunctionDomain1DVector domain,  API::FunctionValues values)
{
    if (outputWS->getNumberHistograms() <= mWSIndexToWrite)
    {
        g_log.error() << "LeBailFit.write2OutputWorkspace.  Try to write to spectrum " << mWSIndexToWrite << " out of range = "
                         << outputWS->getNumberHistograms() << std::endl;
        throw std::invalid_argument("Try to write to a spectrum out of range.");
    }

    for (size_t i = 0; i < domain.size(); ++i)
    {
        outputWS->dataX(mWSIndexToWrite)[i] = domain[i];
    }
    for (size_t i = 0; i < values.size(); ++i)
    {
        outputWS->dataY(mWSIndexToWrite)[i] = values[i];
        if (fabs(values[i]) > 1.0)
            outputWS->dataE(mWSIndexToWrite)[i] = std::sqrt(fabs(values[i]));
        else
            outputWS->dataE(mWSIndexToWrite)[i] = 1.0;
    }

    return;
}

/*
 * Write orignal data and difference b/w data and model to
 */
void LeBailFit::writeInputDataNDiff(size_t workspaceindex, API::FunctionDomain1DVector domain)
{
    // 1. X-axis
    for (size_t i = 0; i < domain.size(); ++i)
    {
        outputWS->dataX(1)[i] = domain[i];
        outputWS->dataX(2)[i] = domain[i];
    }

    // 2. Add data and difference to output workspace (spectrum 1)
    for (size_t i = 0; i < dataWS->readY(workspaceindex).size(); ++i)
    {
        double modelvalue = outputWS->readY(0)[i];
        double inputvalue = dataWS->readY(workspaceindex)[i];
        double diff = modelvalue - inputvalue;
        outputWS->dataY(1)[i] = inputvalue;
        outputWS->dataY(2)[i] = diff;
    }

    return;
}

/*
 * Create a table workspace for parameter values and set to output
 */
void LeBailFit::exportParametersWorkspace(std::map<std::string, std::pair<double, char> > parammap)
{
    DataObjects::TableWorkspace *tablews;

    tablews = new DataObjects::TableWorkspace();
    DataObjects::TableWorkspace_sptr parameterws(tablews);

    tablews->addColumn("str", "Name");
    tablews->addColumn("double", "Value");
    tablews->addColumn("str", "FitOrTie");

    // 3. Add value
    std::map<std::string, std::pair<double, char> >::iterator paramiter;
    for (paramiter = parammap.begin(); paramiter != parammap.end(); ++paramiter)
    {
        std::string parname = paramiter->first;
        double parvalue = paramiter->second.first;
        char fitortie = paramiter->second.second;
        std::stringstream ss;
        ss << fitortie;
        std::string fit_tie = ss.str();

        API::TableRow newparam = tablews->appendRow();
        newparam << parname << parvalue << fit_tie;
    }

    // 4. Add to output peroperty
    this->setProperty("ParametersWorkspace", parameterws);

    return;
}

} // namespace CurveFitting
} // namespace Mantid
