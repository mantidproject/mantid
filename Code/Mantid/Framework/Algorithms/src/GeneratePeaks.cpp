#include "MantidAlgorithms/GeneratePeaks.h"
#include "MantidKernel/System.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidKernel/ListValidator.h"
#include "MantidAPI/Column.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionDomain.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidAPI/SpectraAxis.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace Algorithms
{

  DECLARE_ALGORITHM(GeneratePeaks)

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  GeneratePeaks::GeneratePeaks()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  GeneratePeaks::~GeneratePeaks()
  {
  }
  
  void GeneratePeaks::initDocs()
  {

  }

  /*
   * Define algorithm's properties
   */
  void GeneratePeaks::init()
  {
    this->declareProperty(new API::WorkspaceProperty<DataObjects::TableWorkspace>("PeakParametersWorkspace", "", Direction::Input),
        "Input TableWorkspace for peak's parameters.");

    std::vector<std::string> peakNames = FunctionFactory::Instance().getFunctionNames<API::IPeakFunction>();
    this->declareProperty("PeakFunction", "Gaussian", boost::make_shared<StringListValidator>(peakNames),
        "Peak function to calculate.");

    this->declareProperty(new API::WorkspaceProperty<API::MatrixWorkspace>("InputWorkspace", "", Direction::Input, PropertyMode::Optional),
        "InputWorkspace (optional) to take information for the instrument, and where to evaluate the x-axis.");

    this->declareProperty(
      new Kernel::ArrayProperty<double>("BinningParameters", boost::make_shared<Kernel::RebinParamsValidator>()),
      "A comma separated list of first bin boundary, width, last bin boundary. Optionally\n"
      "this can be followed by a comma and more widths and last boundary pairs.\n"
      "Negative width values indicate logarithmic binning.");


    this->declareProperty("NumberWidths", 2., "Number of peak width to evaluate each peak for. Default=2.");
    this->declareProperty(new API::WorkspaceProperty<API::MatrixWorkspace>("OutputWorkspace", "", Direction::Output),
        "Output Workspace to put the calculated data.");

    this->declareProperty("GeneratePeaks", true, "Whether or not to generate the peaks");

    this->declareProperty("GenerateBackground", true, "Whether or not to generate the background");

    this->declareProperty("MaxAllowedChi2", 100.0, "Maximum chi^2 of the peak allowed to calculate.");

    return;
  }

  /*
   * Main execution body
   */
  void GeneratePeaks::exec()
  {
    // 1. Get properties
    DataObjects::TableWorkspace_sptr mPeakParametersWS = this->getProperty("PeakParametersWorkspace");
    std::string mPeakFunction = this->getProperty("PeakFunction");

    mInputWS = this->getProperty("InputWorkspace");
    const std::vector<double> mBinParameters = this->getProperty("BinningParameters");

    mNumWidths = this->getProperty("NumberWidths");
    mGeneratePeak = this->getProperty("GeneratePeaks");
    mGenerateBackground = this->getProperty("GenerateBackground");

    // 2. Understand input workspace
    std::set<specid_t> spectra;
    getSpectraSet(mPeakParametersWS, spectra);

    // 3. Set output workspace
    API::MatrixWorkspace_sptr mOutputWorkspace;
    if (!mInputWS && mBinParameters.empty())
    {
      // a) Nothing is setup
      g_log.error("Must define either InputWorkspace or BinningParameters.");
      throw std::invalid_argument("Must define either InputWorkspace or BinningParameters.");
    }
    else if (mInputWS)
    {
      // c) Generate Workspace2D from input workspace
      if (!mBinParameters.empty())
        g_log.notice() << "Both binning parameters and input workspace are given. "
          << "Using input worksapce to generate output workspace!" << std::endl;

      mOutputWorkspace = API::WorkspaceFactory::Instance().create(mInputWS, mInputWS->getNumberHistograms(),
          mInputWS->dataX(0).size(), mInputWS->dataY(0).size());

      std::set<specid_t>::iterator siter;
      for (siter = spectra.begin(); siter != spectra.end(); ++siter)
      {
        specid_t iws = *siter;
        std::copy(mInputWS->dataX(iws).begin(), mInputWS->dataX(iws).end(), mOutputWorkspace->dataX(iws).begin());
      }

      mNewWSFromParent = true;
    }
    else
    {
      // d) Generate a one-spectrum Workspace2D from binning
      mOutputWorkspace = createOutputWorkspace(spectra, mBinParameters);
      mNewWSFromParent = false;
    }

    this->setProperty("OutputWorkspace", mOutputWorkspace);

    // 4. Generate peaks
    generatePeaks(mOutputWorkspace, mPeakParametersWS, mPeakFunction);

    return;
  }

  /*
   * Generate peaks
   */
  void GeneratePeaks::generatePeaks(API::MatrixWorkspace_sptr dataWS, DataObjects::TableWorkspace_const_sptr peakparameters,
      std::string peakfunction)
  {
    double maxchi2 = this->getProperty("MaxAllowedChi2");

    size_t numpeaks = peakparameters->rowCount();
    for (size_t ipk = 0; ipk < numpeaks; ipk ++)
    {
      // 1. Get to know workspace index
      specid_t specid = static_cast<specid_t>(getTableValue(peakparameters, "spectrum", ipk));
      specid_t wsindex;
      if (mNewWSFromParent)
        wsindex = specid;
      else
        wsindex = mSpectrumMap[specid];

      // 2. Get peak parameters
      double centre = getTableValue(peakparameters, "centre", ipk);
      double width = getTableValue(peakparameters, "width", ipk);
      double height = getTableValue(peakparameters, "height", ipk);
      double a0 = getTableValue(peakparameters, "backgroundintercept", ipk);
      double a1 = getTableValue(peakparameters, "backgroundslope", ipk);
      double a2 = getTableValue(peakparameters, "A2", ipk);
      double chi2 = getTableValue(peakparameters, "chi2", ipk);

      g_log.debug() << "Peak " << ipk << ": Spec = " << specid << " -> WSIndex = " << wsindex <<
          "  center = " << centre << ", height = " << height << ", sigma = " << width <<
          "  a0 = " << a0 << ", a1 = " << a1 << ", a2 = " << a2 << std::endl;

      if (chi2 > maxchi2)
      {
        g_log.notice() << "Skip Peak " << ipk << ": chi^2 = " << chi2 << " Larger than max. allowed chi^2" << std::endl;
        continue;
      }

      // 3. Build domain & function
      // TODO Can make this part more smart to sum over only a portion of X range
      const MantidVec& X = dataWS->dataX(wsindex);
      std::vector<double>::const_iterator left = std::lower_bound(X.begin(), X.end(), centre - mNumWidths*width);
      if (left == X.end())
        left = X.begin();
      std::vector<double>::const_iterator right = std::lower_bound(left + 1, X.end(), centre + mNumWidths*width);
      API::FunctionDomain1DVector domain(left, right); //dataWS->dataX(wsindex));
      API::IFunction_sptr mfunc = this->createFunction(peakfunction, height, centre, width,
          a0, a1, a2, mGeneratePeak, mGenerateBackground);
      API::FunctionValues values(domain);
      mfunc->function(domain, values);

      // 4. Put to output
      std::size_t offset = (left-X.begin());
      std::size_t numY = values.size();
//      PARALLEL_FOR1(dataWS)
      for (std::size_t i = 0; i < numY; i ++)
      {
//        PARALLEL_START_INTERUPT_REGION
        dataWS->dataY(wsindex)[i + offset] += values[i];
//        PARALLEL_END_INTERUPT_REGION
      }
//      PARALLEL_CHECK_INTERUPT_REGION

    } // for peak

    return;
  }

  /**
   * Create a function for fitting.
   * @param withPeak If this is set to false then return only a background function.
   * @return The requested function to fit.
   */
  API::IFunction_sptr GeneratePeaks::createFunction(std::string m_peakFuncType,
      const double height, const double centre, const double sigma,
      const double a0, const double a1, const double a2, const bool withPeak, const bool withBackground)
  {

    // 1. Create return
    API::CompositeFunction* fitFunc = new CompositeFunction();

    // 2. setup the background
    if (withBackground)
    {
      auto background =
          API::FunctionFactory::Instance().createFunction("QuadraticBackground");

      background->setParameter("A0", a0);
      background->setParameter("A1", a1);
      background->setParameter("A2", a2);

      fitFunc->addFunction(background);
    }

    // 3. setup the peak
    if (withPeak)
    {
      auto tempPeakFunc = API::FunctionFactory::Instance().createFunction(m_peakFuncType);
      auto peakFunc = boost::dynamic_pointer_cast<IPeakFunction>(tempPeakFunc);
      peakFunc->setHeight(height);
      peakFunc->setCentre(centre);
      peakFunc->setFwhm(sigma);

      if (withPeak)
        fitFunc->addFunction(peakFunc);
    }

    return boost::shared_ptr<IFunction>(fitFunc);
  }

  /*
   * Create a Workspace2D (MatrixWorkspace) with given spectra and bin parameters
   */
  API::MatrixWorkspace_sptr GeneratePeaks::createOutputWorkspace(std::set<specid_t> spectra, std::vector<double> mBinParameters)
  {
    // 0. Check
    if (spectra.size() == 0)
      throw std::invalid_argument("Input spectra number is 0.  Unable to generate a new workspace.");

    if (mBinParameters.size() < 3)
    {
      g_log.error() << "Input binning parameters are not enough." << std::endl;
      throw std::invalid_argument("Input binning paramemters are not acceptible.");
    }

    // 1. Determine number of x values
    std::vector<double> xarray;
    double x0 = mBinParameters[0];
    double dx = mBinParameters[1];
    double xf = mBinParameters[2];
    double xvalue = x0;
    while (xvalue <= xf)
    {
      // a) push
      xarray.push_back(xvalue);
      // b) next value
      if (dx > 0)
        xvalue += dx;
      else if (dx < 0)
        xvalue += -1*dx*xvalue;
      else
        throw std::invalid_argument("Step of binning parameters cannot be 0.");
    }
    size_t numxvalue = xarray.size();

    // 2. Create new workspace
    API::MatrixWorkspace_sptr ws = API::WorkspaceFactory::Instance().create("Workspace2D", spectra.size(), numxvalue, numxvalue-1);
    for (size_t ip = 0; ip < spectra.size(); ip ++)
      std::copy(xarray.begin(), xarray.end(), ws->dataX(ip).begin());

    // 3. Link spectrum to workspace index
    API::SpectraAxis * ax = dynamic_cast<API::SpectraAxis * >( ws->getAxis(1));
    if (!ax)
      throw std::runtime_error("MatrixWorkspace::getSpectrumToWorkspaceIndexMap: axis[1] is not a SpectraAxis, so I cannot generate a map.");

    std::map<specid_t, specid_t>::iterator spiter;
    for (spiter = mSpectrumMap.begin(); spiter != mSpectrumMap.end(); ++spiter)
    {
      specid_t specid = spiter->first;
      specid_t wsindex = spiter->second;
      g_log.debug() << "Build WorkspaceIndex-Spectrum  " << wsindex << " , " << specid << std::endl;
      ax->setValue(wsindex, specid);
    }

    // TODO Remove after test
    spec2index_map* tmap = ws->getSpectrumToWorkspaceIndexMap();
    spec2index_map::iterator titer;
    for (titer = tmap->begin(); titer != tmap->end(); ++titer)
      g_log.notice() << "Map built:  wsindex = " << titer->first << "\t\tspectrum = " << titer->second << std::endl;

    return ws;
  }

  /*
   * Get set of spectra of the input table workspace
   */
  void GeneratePeaks::getSpectraSet(DataObjects::TableWorkspace_const_sptr peakParmsWS, std::set<specid_t>& spectra)
  {
    size_t numpeaks = peakParmsWS->rowCount();
    for (size_t ipk = 0; ipk < numpeaks; ipk ++)
    {
      API::Column_const_sptr col = peakParmsWS->getColumn("spectrum");
      specid_t specid = static_cast<specid_t>((*col)[ipk]);
      spectra.insert(specid);
      g_log.debug() << "Peak " << ipk << ": specid = " << specid << std::endl;
    }

    std::set<specid_t>::iterator pit;
    size_t icount = 0;
    for (pit = spectra.begin(); pit != spectra.end(); ++pit)
    {
      mSpectrumMap.insert(std::make_pair(*pit, icount));
      ++icount;
    }

    return;
  }

  /*
   * Get one specific table value
   */
  double GeneratePeaks::getTableValue(DataObjects::TableWorkspace_const_sptr tableWS, std::string colname, size_t index)
  {
    API::Column_const_sptr col = tableWS->getColumn(colname);
    if (!col)
    {
      g_log.error() << "Column with name " << colname << " does not exist" << std::endl;
      throw std::invalid_argument("Non-exist column name");
    }

    if (index >= col->size())
    {
      g_log.error() << "Try to access index " << index << " out of boundary = " << col->size() << std::endl;
      throw std::runtime_error("Access column array out of boundary");
    }

    return (*col)[index];
  }

} // namespace Mantid
} // namespace Algorithms
