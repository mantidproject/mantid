#include "MantidCurveFitting/LeBailFit2.h"
#include "MantidKernel/ListValidator.h"
#include "MantidAPI/TableRow.h"

namespace Mantid
{
namespace CurveFitting
{

DECLARE_ALGORITHM(LeBailFit2)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
LeBailFit2::LeBailFit2()
{
}
    
//----------------------------------------------------------------------------------------------
/** Destructor
 */
LeBailFit2::~LeBailFit2()
{
}
  
/*
 * Sets documentation strings for this algorithm
 */
void LeBailFit2::initDocs()
{
    this->setWikiSummary("Do LeBail Fit to a spectrum of powder diffraction data.. ");
    this->setOptionalMessage("Do LeBail Fit to a spectrum of powder diffraction data. ");
}

/*
 * Define the input properties for this algorithm
 */
void LeBailFit2::init()
{
  this->declareProperty(new API::WorkspaceProperty<API::MatrixWorkspace>("InputWorkspace", "", Direction::Input),
      "Input workspace containing the data to fit by LeBail algorithm.");
  this->declareProperty(new API::WorkspaceProperty<DataObjects::TableWorkspace>("ParametersWorkspace", "", Direction::InOut),
      "Input table workspace containing the parameters required by LeBail fit. ");
  this->declareProperty(new API::WorkspaceProperty<DataObjects::TableWorkspace>("ReflectionsWorkspace", "", Direction::InOut),
      "Input table workspace containing the list of reflections (HKL). ");
  this->declareProperty("WorkspaceIndex", 0, "Workspace index of the spectrum to fit by LeBail.");

  std::vector<std::string> functions;
  functions.push_back("LeBailFit");
  functions.push_back("Calculation");
  functions.push_back("AutoSelectBackgroundPoints");
  auto validator = boost::make_shared<Kernel::StringListValidator>(functions);
  this->declareProperty("Function", "LeBailFit", validator, "Functionality");

  this->declareProperty(new API::WorkspaceProperty<DataObjects::Workspace2D>("OutputWorkspace", "", Direction::Output),
                        "Output workspace containing calculated pattern or calculated background. ");

  return;
}

/*
 * Implement abstract Algorithm methods
 */
void LeBailFit2::exec()
{
    // 1. Get input
    dataWS = this->getProperty("InputWorkspace");
    parameterWS = this->getProperty("ParametersWorkspace");
    reflectionWS = this->getProperty("ReflectionsWorkspace");

    int tempindex = this->getProperty("WorkspaceIndex");
    if (tempindex < 0)
        throw std::invalid_argument("Input workspace index cannot be negative.");
    size_t workspaceindex = size_t(tempindex);

    // 2. Determine Function
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

    for (size_t ipk = 0; ipk < mPeaks.size(); ++ipk)
    {
        mLeBailFunction->addFunction(mPeaks[ipk]);
    }
    std::cout << "LeBail Composite Function: " << mLeBailFunction->asString() << std::endl;

    // 5. Create output workspace
    size_t nspec = 1;
    size_t nbin = dataWS->dataX(workspaceindex).size();
    DataObjects::Workspace2D_sptr outws =
        boost::dynamic_pointer_cast<DataObjects::Workspace2D>(API::WorkspaceFactory::Instance().create("Workspace2D", nspec, nbin, nbin));
    for (size_t i = 0; i < nbin; ++i)
    {
      outws->dataX(0)[i] = dataWS->readX(workspaceindex)[i];
      outws->dataY(0)[i] = dataWS->readY(workspaceindex)[i];
    }
    outws->getAxis(0)->setUnit("TOF");

    this->setProperty("OutputWorkspace", outws);

    // 6. Real work
    switch (functionmode)
    {
    case 0:
        // LeBail Fit
        g_log.notice() << "Do LeBail Fit." << std::endl;
        g_log.error() << "To be implemented soon!" << std::endl;
        break;

    case 1:
        // Calculation
        g_log.notice() << "Pattern Calculation. It is not FINISHED yet. " << std::endl;
        g_log.error() << "To be implemented soon!" << std::endl;
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

  return;
}

/*
 * Generate a list of peaks from input
 */
void LeBailFit2::generatePeaksFromInput()
{
    for (size_t ipk = 0; ipk < mPeakHKLs.size(); ++ipk)
    {
        CurveFitting::ThermalNeutronBk2BkExpConvPV tmppeak;
        int h = mPeakHKLs[ipk][0];
        int k = mPeakHKLs[ipk][1];
        int l = mPeakHKLs[ipk][2];
        tmppeak.setMillerIndex(h, k, l);
        CurveFitting::ThermalNeutronBk2BkExpConvPV_sptr speak = boost::make_shared<CurveFitting::ThermalNeutronBk2BkExpConvPV>(tmppeak);

        mPeaks.push_back(speak);
    }

    return;
}

/*
 * Parse the input TableWorkspace to some maps for easy access
 */
void LeBailFit2::importParametersTable()
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
void LeBailFit2::importReflections()
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

  // 2. Import data to maps
  int h, k, l;

  size_t numrows = reflectionWS->rowCount();
  for (size_t ir = 0; ir < numrows; ++ir)
  {
    API::TableRow trow = reflectionWS->getRow(ir);
    trow >> h >> k >> l;
    std::vector<int> hkl;
    hkl.push_back(h);
    hkl.push_back(k);
    hkl.push_back(l);

    mPeakHKLs.push_back(hkl);
  }

  return;
}


} // namespace CurveFitting
} // namespace Mantid
