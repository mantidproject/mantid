/*WIKI*


Generate a workspace by summing over the peak functions.
The peaks' parameters are given in a [[TableWorkspace]].

==== Peak Parameters ====
Peak parameters must have the following parameters, which are case sensitive in input [[TableWorkspace]]
 1. spectrum
 2. centre
 3. height
 4. width (FWHM)
 5. backgroundintercept (a0)
 6. backgroundslope (a1)
 7. A2
 8. chi2

==== Output ====
 Output will include
 1. pure peak
 2. pure background (with specified range of FWHM (int))
 3. peak + background

[[Category:Algorithms]]

{{AlgorithmLinks|GeneratePeaks}}


*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

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
using namespace Mantid::DataObjects;

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
  
  //----------------------------------------------------------------------------------------------
  /** Wiki documentation
    */
  void GeneratePeaks::initDocs()
  {
    this->setWikiSummary("Generate peaks in an output workspace according to a [[TableWorkspace]] containing a list of peak's parameters.");
  }

  //----------------------------------------------------------------------------------------------
  /** Define algorithm's properties
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
      new Kernel::ArrayProperty<double>("BinningParameters", boost::make_shared<Kernel::RebinParamsValidator>(true)),
      "A comma separated list of first bin boundary, width, last bin boundary. Optionally\n"
      "this can be followed by a comma and more widths and last boundary pairs.\n"
      "Negative width values indicate logarithmic binning.");


    this->declareProperty("NumberWidths", 2., "Number of peak width to evaluate each peak for. Default=2.");
    this->declareProperty(new API::WorkspaceProperty<API::MatrixWorkspace>("OutputWorkspace", "", Direction::Output),
        "Output Workspace to put the calculated data.");

    this->declareProperty("GenerateBackground", true, "Whether or not to generate the background");

    this->declareProperty("MaxAllowedChi2", 100.0, "Maximum chi^2 of the peak allowed to calculate. Default 100.");

    declareProperty("IgnoreWidePeaks", false, "If selected, the peaks that are wider than fit window "
                    "(denoted by negative chi^2) are ignored.");

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Main execution body
   */
  void GeneratePeaks::exec()
  {
    // Get properties
    DataObjects::TableWorkspace_sptr peakParamWS = this->getProperty("PeakParametersWorkspace");
    std::string peakFunction = this->getProperty("PeakFunction");

    const std::vector<double> binParameters = this->getProperty("BinningParameters");
    MatrixWorkspace_const_sptr inputWS = this->getProperty("InputWorkspace");

    // Process peak parameter workspace
    std::set<specid_t> spectra;
    getSpectraSet(peakParamWS, spectra);

    // Reference workspace and output workspace
    API::MatrixWorkspace_sptr outputWS;
    bool newWSFromParent = true;
    if (!inputWS && binParameters.empty())
    {
      // Error! Neither bin parameters or reference workspace is given.
      std::string errmsg("Must define either InputWorkspace or BinningParameters.");
      g_log.error(errmsg);
      throw std::invalid_argument(errmsg);
    }
    else if (inputWS)
    {
      // Generate Workspace2D from input workspace
      if (!binParameters.empty())
        g_log.notice() << "Both binning parameters and input workspace are given. "
                       << "Using input worksapce to generate output workspace!\n";

      outputWS = API::WorkspaceFactory::Instance().create(inputWS, inputWS->getNumberHistograms(),
          inputWS->dataX(0).size(), inputWS->dataY(0).size());

      std::set<specid_t>::iterator siter;
      // Only copy the X-values from spectra with peaks specified in the table workspace.
      for (siter = spectra.begin(); siter != spectra.end(); ++siter)
      {
        specid_t iws = *siter;
        std::copy(inputWS->dataX(iws).begin(), inputWS->dataX(iws).end(), outputWS->dataX(iws).begin());
      }

      newWSFromParent = true;
    }
    else
    {
      // Generate a one-spectrum Workspace2D from binning
      outputWS = createDataWorkspace(spectra, binParameters);
      newWSFromParent = false;
    }

    this->setProperty("OutputWorkspace", outputWS);

    // 4. Generate peaks
    generatePeaks(outputWS, peakParamWS, peakFunction, newWSFromParent);

    return;
  }

  namespace { // anonymous name space
  /**
   * Determine if the table contains raw parameters.
   */
  bool isRawTable(const std::vector<std::string> & colNames)
  {
    if (colNames.size() != 6)
      return true;
    if (colNames[0].compare("centre") != 0)
      return true;
    if (colNames[1].compare("width") != 0)
      return true;
    if (colNames[2].compare("height") != 0)
      return true;
    if (colNames[3].compare("backgroundintercept") != 0)
      return true;
    if (colNames[4].compare("backgroundslope") != 0)
      return true;
    if (colNames[5].compare("A2") != 0)
      return true;
    return false;
  }

  /**
   * Determine how many parameters are in the peak function.
   */
  std::size_t getBkgOffset(const std::vector<std::string> & colNames, const bool isRaw)
  {
    if (!isRaw)
      return 3;

    for (std::size_t i = 0; i < colNames.size(); i++)
    {
      if (colNames[i].substr(0,3).compare("f1.") == 0)
        return i;
    }
    return colNames.size(); // shouldn't get here
  }
  }

  //----------------------------------------------------------------------------------------------
  /** Generate peaks and background if optioned
   */
  void GeneratePeaks::generatePeaks(API::MatrixWorkspace_sptr dataWS, DataObjects::TableWorkspace_const_sptr peakparameters,
                                    std::string peakfunction, bool newWSFromParent)
  {
    // Special properties related
    double maxchi2 = this->getProperty("MaxAllowedChi2");
    double numWidths = this->getProperty("NumberWidths");
    bool ignorewidepeaks = getProperty("IgnoreWidePeaks");
    bool generateBackground = this->getProperty("GenerateBackground");

    size_t numpeaks = peakparameters->rowCount();

    // Get the parameter names for the peak function
    std::vector<std::string> columnNames = peakparameters->getColumnNames();
    if (!columnNames.front().compare("spectrum") == 0)
    {
      std::stringstream msg;
      msg << "First column of table should be \"spectrum\". Found \"" << columnNames.front() << "\"";
      throw std::invalid_argument(msg.str());
    }
    columnNames.erase(columnNames.begin()); // don't need to know that name
    if (!columnNames.back().compare("chi2") == 0)
    {
      std::stringstream msg;
      msg << "Last column of table should be \"chi2\". Found \"" << columnNames.back() << "\"";
      throw std::invalid_argument(msg.str());
    }
    columnNames.erase(columnNames.begin()+(columnNames.size()-1)); // don't need to know that name either
    const bool isRaw = isRawTable(columnNames);
    const std::size_t bkgOffset = getBkgOffset(columnNames, isRaw);
    g_log.information() << "isRaw=" << isRaw << " bkgOffset=" << bkgOffset << "\n";

    // Create data structure for all peaks functions
    std::map<specid_t, std::vector<std::pair<double, API::IFunction_sptr> > > functionmap;
    std::map<specid_t, std::vector<std::pair<double, API::IFunction_sptr> > >::iterator mapiter;

    for (size_t ipk = 0; ipk < numpeaks; ipk ++)
    {
      // Get to know workspace index
      specid_t specid = static_cast<specid_t>(getTableValue(peakparameters, "spectrum", ipk));
      specid_t wsindex;
      if (newWSFromParent)
        wsindex = specid;
      else
        wsindex = mSpectrumMap[specid];

      // Existed?
      mapiter = functionmap.find(wsindex);
      if (mapiter == functionmap.end())
      {
        std::vector<std::pair<double, API::IFunction_sptr> > tempvector;
        std::pair<std::map<specid_t, std::vector<std::pair<double, API::IFunction_sptr> > >::iterator, bool> ret;
        ret = functionmap.insert(std::make_pair(wsindex, tempvector));
        mapiter = ret.first;
      }

      // Generate peak function
      double chi2 = getTableValue(peakparameters, "chi2", ipk);
      double centre, fwhm; // output parameters
      API::IFunction_sptr mfunc = createFunction(peakfunction, columnNames, isRaw, generateBackground, peakparameters, bkgOffset, ipk,
                                                 centre, fwhm);

      if (chi2 > maxchi2)
      {
        g_log.notice() << "Skip Peak " << ipk << " (chi^2 " << chi2 << " > " << maxchi2 << ") at " << centre << "\n";
        continue;
      }
      else if (chi2 < 0. && ignorewidepeaks)
      {
        g_log.notice() << "Skip Peak " << ipk << " (chi^2 " << chi2 << " < 0 ) at " << centre << "\n";
      }
      else
      {
        mapiter->second.push_back(std::make_pair(centre, mfunc));
      }

      g_log.debug() << "Peak " << ipk << ": Spec = " << specid << " -> WSIndex = " << wsindex
                    << " func: " << mfunc->asString() << "\n";
    }

    for (mapiter = functionmap.begin(); mapiter != functionmap.end(); ++mapiter)
    {
      // Sort functions in same spectrum by centre
      std::vector<std::pair<double, API::IFunction_sptr> >& vecfuncs = mapiter->second;
      std::sort(vecfuncs.begin(), vecfuncs.end());
      specid_t wsindex = mapiter->first;

      // Plot each
      for (size_t ipk = 0; ipk < mapiter->second.size(); ++ipk)
      {
        // Basic parameters of the peak
        API::IFunction_sptr mfunc = vecfuncs[ipk].second;
        API::IPeakFunction_sptr mpeak = getPeakFunction(mfunc);
        if (!mpeak)
          throw std::runtime_error("Function in array does not contain any peak function!");

        double centre = vecfuncs[ipk].first;
        double fwhm = mpeak->fwhm();

        // Determine boundary
        const MantidVec& X = dataWS->dataX(wsindex);
        double leftbound = centre - numWidths*fwhm;
        if (ipk > 0)
        {
          // Not left most peak.
          double middle = 0.5*(centre + vecfuncs[ipk-1].first);
          if (leftbound < middle)
            leftbound = middle;
        }
        std::vector<double>::const_iterator left = std::lower_bound(X.begin(), X.end(), leftbound);
        if (left == X.end())
          left = X.begin();

        double rightbound = centre + numWidths*fwhm;
        if (ipk != vecfuncs.size()-1)
        {
          // Not the rightmost peak
          double middle = 0.5*(centre + vecfuncs[ipk+1].first);
          if (rightbound > middle)
            rightbound = middle;
        }
        std::vector<double>::const_iterator right = std::lower_bound(left + 1, X.end(), rightbound);

        // Build domain & function
        API::FunctionDomain1DVector domain(left, right); //dataWS->dataX(wsindex));

        // 4. Evaluate the function
        API::FunctionValues values(domain);
        mfunc->function(domain, values);

        // 5. Put to output
        std::size_t offset = (left-X.begin());
        std::size_t numY = values.size();
        for (std::size_t i = 0; i < numY; i ++)
        {
          dataWS->dataY(wsindex)[i + offset] += values[i];
        }
      } // for peak

    } // for spectra

    return;
  }

  //----------------------------------------------------------------------------------------------
  /**
   * Create a function for fitting.
   * @return The requested function to fit.
   */
  API::IFunction_sptr GeneratePeaks::createFunction(const std::string &peakFuncType, const std::vector<std::string> &colNames,
                                                    const bool isRaw, const bool withBackground,
                                                    DataObjects::TableWorkspace_const_sptr peakParmsWS,
                                                    const std::size_t bkg_offset, const std::size_t rowNum, double &centre, double &fwhm)
  {
    // create the peak
    auto tempPeakFunc = API::FunctionFactory::Instance().createFunction(peakFuncType);
    auto peakFunc = boost::dynamic_pointer_cast<IPeakFunction>(tempPeakFunc);
    if (isRaw)
    {
      std::string paramName;
      for (std::size_t i = 0; i < bkg_offset; i++)
      {
        paramName = colNames[i].substr(3);
        peakFunc->setParameter(paramName, getTableValue(peakParmsWS, colNames[i], rowNum));
      }
    }
    else
    {
      peakFunc->setHeight(getTableValue(peakParmsWS, "height", rowNum));
      peakFunc->setCentre(getTableValue(peakParmsWS, "centre", rowNum));
      peakFunc->setFwhm(getTableValue(peakParmsWS, "width", rowNum));
    }
    centre = peakFunc->centre();
    fwhm = peakFunc->fwhm();

    // skip out early
    if (!withBackground)
      return boost::shared_ptr<IFunction>(peakFunc);


    // create the background
    auto backFunc =
        API::FunctionFactory::Instance().createFunction("Quadratic");
    if (isRaw)
    {
      std::string paramName;
      for (std::size_t i = bkg_offset; i < colNames.size(); i++)
      {
        paramName = colNames[i].substr(3);
        backFunc->setParameter(paramName, getTableValue(peakParmsWS, colNames[i], rowNum));
      }
    }
    else
    {
      backFunc->setParameter("A0", getTableValue(peakParmsWS, "backgroundintercept", rowNum));
      backFunc->setParameter("A1", getTableValue(peakParmsWS, "backgroundslope", rowNum));
      backFunc->setParameter("A2", getTableValue(peakParmsWS, "A2", rowNum));
    }

    // setup the output
    API::CompositeFunction* fitFunc = new CompositeFunction();
    fitFunc->addFunction(peakFunc);
    fitFunc->addFunction(backFunc);

    return boost::shared_ptr<IFunction>(fitFunc);
  }

  //----------------------------------------------------------------------------------------------
  /** Create a Workspace2D (MatrixWorkspace) with given spectra and bin parameters
   */
  MatrixWorkspace_sptr GeneratePeaks::createDataWorkspace(std::set<specid_t> spectra, std::vector<double> binparameters)
  {
    // Check validity
    if (spectra.size() == 0)
      throw std::invalid_argument("Input spectra list is empty. Unable to generate a new workspace.");

    if (binparameters.size() < 3)
    {
      std::stringstream errss;
      errss << "Number of input binning parameters are not enough (" << binparameters.size() << "). "
            << "Binning parameters should be 3 (x0, step, xf).";
      g_log.error(errss.str());
      throw std::invalid_argument(errss.str());
    }

    double x0 = binparameters[0];
    double dx = binparameters[1];
    double xf = binparameters[2];
    if (x0 >= xf || (xf - x0) < dx || dx == 0.)
    {
      std::stringstream errss;
      errss << "Order of input binning parameters is not correct.  It is not logical to have "
            << "x0 = " << x0 << ", xf = " << xf << ", dx = " << dx;
      g_log.error(errss.str());
      throw std::invalid_argument(errss.str());
    }

    // Determine number of x values
    std::vector<double> xarray;
    double xvalue = x0;
    while (xvalue <= xf)
    {
      // Push current value to vector
      xarray.push_back(xvalue);

      // Calculate next value, linear or logarithmic
      if (dx > 0)
        xvalue += dx;
      else
        xvalue += fabs(dx)*xvalue;
    }
    size_t numxvalue = xarray.size();

    // Create new workspace
    MatrixWorkspace_sptr ws = API::WorkspaceFactory::Instance().create("Workspace2D", spectra.size(), numxvalue, numxvalue-1);
    for (size_t ip = 0; ip < spectra.size(); ip ++)
      std::copy(xarray.begin(), xarray.end(), ws->dataX(ip).begin());

    // Set spectrum numbers
    std::map<specid_t, specid_t>::iterator spiter;
    for (spiter = mSpectrumMap.begin(); spiter != mSpectrumMap.end(); ++spiter)
    {
      specid_t specid = spiter->first;
      specid_t wsindex = spiter->second;
      g_log.debug() << "Build WorkspaceIndex-Spectrum  " << wsindex << " , " << specid << "\n";
      ws->getSpectrum(wsindex)->setSpectrumNo(specid);
    }

    return ws;
  }

  //----------------------------------------------------------------------------------------------
  /** Get set of spectra of the input table workspace
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
    specid_t icount = 0;
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

  //----------------------------------------------------------------------------------------------
  /** Get the IPeakFunction part in the input function
    */
  API::IPeakFunction_sptr GeneratePeaks::getPeakFunction(API::IFunction_sptr infunction)
  {
    // Not a composite function
    API::CompositeFunction_sptr compfunc = boost::dynamic_pointer_cast<API::CompositeFunction>(infunction);

    // If it is a composite function (complete part for return)
    if (compfunc)
    {
      for (size_t i = 0; i < compfunc->nFunctions(); ++i)
      {
        API::IFunction_sptr subfunc = compfunc->getFunction(i);
        API::IPeakFunction_sptr peakfunc = boost::dynamic_pointer_cast<API::IPeakFunction>(subfunc);
        if (peakfunc)
          return peakfunc;
      }

    }

    // Return if not a composite function
    API::IPeakFunction_sptr peakfunc = boost::dynamic_pointer_cast<API::IPeakFunction>(infunction);

    return peakfunc;
  }

} // namespace Mantid
} // namespace Algorithms
