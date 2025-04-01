// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/GeneratePeaks.h"
#include "MantidAPI/Column.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/IBackgroundFunction.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/HistogramBuilder.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/RebinParamsValidator.h"
#include "MantidTypes/SpectrumDefinition.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::HistogramData;

namespace Mantid::Algorithms {
DECLARE_ALGORITHM(GeneratePeaks)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
GeneratePeaks::GeneratePeaks()
    : m_peakFunction(), m_bkgdFunction(), m_vecPeakParamValues(), m_vecBkgdParamValues(), m_SpectrumMap(),
      m_spectraSet(), m_useAutoBkgd(false), m_funcParamWS(), inputWS(), m_newWSFromParent(false), binParameters(),
      m_genBackground(false), m_useRawParameter(false), m_maxChi2(0.), m_numPeakWidth(0.), m_funcParameterNames(),
      i_height(-1), i_centre(-1), i_width(-1), i_a0(-1), i_a1(-1), i_a2(-1), m_useFuncParamWS(false), m_wsIndex(-1) {}

//----------------------------------------------------------------------------------------------
/** Define algorithm's properties
 */
void GeneratePeaks::init() {
  declareProperty(std::make_unique<API::WorkspaceProperty<DataObjects::TableWorkspace>>(
                      "PeakParametersWorkspace", "", Direction::Input, PropertyMode::Optional),
                  "Input TableWorkspace for peak's parameters.");

  // Peak function properties
  std::vector<std::string> peakNames = FunctionFactory::Instance().getFunctionNames<IPeakFunction>();
  std::vector<std::string> peakFullNames = addFunctionParameterNames(peakNames);
  declareProperty("PeakType", "", std::make_shared<StringListValidator>(peakFullNames), "Peak function type. ");

  for (size_t i = 0; i < peakFullNames.size(); ++i)
    g_log.debug() << "Peak function " << i << ": " << peakFullNames[i] << "\n";

  declareProperty(std::make_unique<ArrayProperty<double>>("PeakParameterValues"),
                  "List of peak parameter values.  They must have a 1-to-1 "
                  "mapping to PeakParameterNames list. ");

  // Background properties
  std::vector<std::string> bkgdtypes{"Auto",
                                     "Flat (A0)",
                                     "Linear (A0, A1)",
                                     "Quadratic (A0, A1, A2)",
                                     "Flat"
                                     "Linear",
                                     "Quadratic"};
  declareProperty("BackgroundType", "Linear", std::make_shared<StringListValidator>(bkgdtypes), "Type of Background.");

  declareProperty(std::make_unique<ArrayProperty<double>>("BackgroundParameterValues"),
                  "List of background parameter values.  They must have a "
                  "1-to-1 mapping to PeakParameterNames list. ");

  declareProperty(std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>("InputWorkspace", "", Direction::Input,
                                                                                 PropertyMode::Optional),
                  "InputWorkspace (optional) to take information for the instrument, and "
                  "where to evaluate the x-axis.");

  declareProperty("WorkspaceIndex", 0,
                  "Spectrum of the peak to be generated.  "
                  "It is only applied to the case by input parameter values in "
                  "vector format. ");

  declareProperty(std::make_unique<Kernel::ArrayProperty<double>>("BinningParameters",
                                                                  std::make_shared<Kernel::RebinParamsValidator>(true)),
                  "A comma separated list of first bin boundary, width, last "
                  "bin boundary. Optionally\n"
                  "this can be followed by a comma and more widths and last "
                  "boundary pairs.\n"
                  "Negative width values indicate logarithmic binning.");

  declareProperty("NumberWidths", 2., "Number of peak width to evaluate each peak for. Default=2.");
  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>("OutputWorkspace", "", Direction::Output),
      "Output Workspace to put the calculated data.");

  declareProperty("GenerateBackground", true, "Whether or not to generate the background");

  declareProperty("MaxAllowedChi2", 100.0, "Maximum chi^2 of the peak allowed to calculate. Default 100.");

  declareProperty("IgnoreWidePeaks", false,
                  "If selected, the peaks that are wider than fit window "
                  "(denoted by negative chi^2) are ignored.");

  declareProperty("IsRawParameter", true,
                  "Flag to show whether the parameter table contains raw parameters. "
                  "In the case that parameter values are input via vector, and this flag "
                  "is set to false, "
                  "the default order of effective peak parameters is centre, height and "
                  "width; "
                  "the default order of effective background parameters is A0, A1 and "
                  "A2. ");
}

//----------------------------------------------------------------------------------------------
/** Main execution body
 */
void GeneratePeaks::exec() {
  // Process input parameters
  std::string peaktype, bkgdtype;
  processAlgProperties(peaktype, bkgdtype);

  // Create functions
  createFunction(peaktype, bkgdtype);

  // Process parameter table
  if (m_useFuncParamWS) {
    processTableColumnNames();
    getSpectraSet(m_funcParamWS);
  }

  // Create output workspace
  API::MatrixWorkspace_sptr outputWS = createOutputWorkspace();
  setProperty("OutputWorkspace", outputWS);

  // Generate peaks
  std::map<specnum_t, std::vector<std::pair<double, API::IFunction_sptr>>> functionmap;
  if (m_useFuncParamWS)
    importPeaksFromTable(functionmap);
  else {
    std::vector<std::pair<double, API::IFunction_sptr>> vecpeakfunc;
    importPeakFromVector(vecpeakfunc);
    functionmap.emplace(m_wsIndex, vecpeakfunc);
  }

  generatePeaks(functionmap, outputWS);
}

//----------------------------------------------------------------------------------------------
/** Process algorithm properties
 */
void GeneratePeaks::processAlgProperties(std::string &peakfunctype, std::string &bkgdfunctype) {
  // Function parameters
  std::string paramwsname = getPropertyValue("PeakParametersWorkspace");
  if (!paramwsname.empty()) {
    // Using parameter table workspace has a higher priority
    m_useFuncParamWS = true;
    m_funcParamWS = getProperty("PeakParametersWorkspace");
  } else {
    m_useFuncParamWS = false;
    m_vecPeakParamValues = getProperty("PeakParameterValues");
    m_vecBkgdParamValues = getProperty("BackgroundParameterValues");
  }

  peakfunctype = getPropertyValue("PeakType");
  // Remove extra helping message
  if (peakfunctype.find('(') != std::string::npos) {
    std::vector<std::string> strs;
    boost::split(strs, peakfunctype, boost::is_any_of(" ("));
    peakfunctype = strs[0];
  }

  bkgdfunctype = getPropertyValue("BackgroundType");
  // Remove extra helping message
  if (bkgdfunctype.find('(') != std::string::npos) {
    std::vector<std::string> strs;
    boost::split(strs, bkgdfunctype, boost::is_any_of(" ("));
    bkgdfunctype = strs[0];
  }

  if (bkgdfunctype == "Auto") {
    m_useAutoBkgd = true;
    bkgdfunctype = "Quadratic";
  } else if (bkgdfunctype == "None") {
    m_useAutoBkgd = false;
    m_genBackground = false;
  } else if (bkgdfunctype == "Linear" || bkgdfunctype == "Flat") {
    m_useAutoBkgd = false;
    bkgdfunctype = bkgdfunctype + "Background";
  }

  binParameters = this->getProperty("BinningParameters");
  inputWS = this->getProperty("InputWorkspace");

  m_genBackground = getProperty("GenerateBackground");

  m_useRawParameter = getProperty("IsRawParameter");

  // Special properties related
  m_maxChi2 = this->getProperty("MaxAllowedChi2");
  m_numPeakWidth = this->getProperty("NumberWidths");

  // Spectrum set if not using parameter table workspace
  // One and only one peak
  if (!m_useFuncParamWS) {
    m_wsIndex = getProperty("WorkspaceIndex");
    m_spectraSet.insert(static_cast<specnum_t>(m_wsIndex));
    m_SpectrumMap.emplace(static_cast<specnum_t>(m_wsIndex), 0);
  }
}

//----------------------------------------------------------------------------------------------
/** Import peak and background functions from table workspace
 * @param functionmap :: (output) map contains vector of functions for each
 * spectrum
 */
void GeneratePeaks::importPeaksFromTable(
    std::map<specnum_t, std::vector<std::pair<double, API::IFunction_sptr>>> &functionmap) {
  size_t numpeaks = m_funcParamWS->rowCount();
  size_t icolchi2 = m_funcParamWS->columnCount() - 1;
  size_t numpeakparams = m_peakFunction->nParams();
  size_t numbkgdparams = 0;
  if (m_bkgdFunction)
    numbkgdparams = m_bkgdFunction->nParams();
  else
    g_log.warning("There is no background function specified. ");

  // Create data structure for all peaks functions
  std::map<specnum_t, std::vector<std::pair<double, API::IFunction_sptr>>>::iterator mapiter;

  // Go through the table workspace to create peak/background functions
  for (size_t ipeak = 0; ipeak < numpeaks; ++ipeak) {
    // Spectrum
    int wsindex = m_funcParamWS->cell<int>(ipeak, 0);

    // Ignore peak with large chi^2/Rwp
    double chi2 = m_funcParamWS->cell<double>(ipeak, icolchi2);
    if (chi2 > m_maxChi2) {
      g_log.notice() << "Skip Peak " << ipeak << " (chi^2 " << chi2 << " > " << m_maxChi2 << ".) "
                     << "\n";
      continue;
    } else if (chi2 < 0.) {
      g_log.notice() << "Skip Peak " << ipeak << " (chi^2 " << chi2 << " < 0 )"
                     << "\n";
      continue;
    } else {
      g_log.debug() << "[DB] Chi-square = " << chi2 << "\n";
    }

    // Set up function
    if (m_useRawParameter) {
      for (size_t p = 0; p < numpeakparams; ++p) {
        std::string parname = m_funcParameterNames[p];
        double parvalue = m_funcParamWS->cell<double>(ipeak, p + 1);
        m_peakFunction->setParameter(parname, parvalue);
      }
      if (m_genBackground) {
        for (size_t p = 0; p < numbkgdparams; ++p) {
          std::string parname = m_funcParameterNames[p + numpeakparams];
          double parvalue = m_funcParamWS->cell<double>(ipeak, p + 1 + numpeakparams);
          m_bkgdFunction->setParameter(parname, parvalue);
        }
      }
    } else {
      double tmpheight = m_funcParamWS->cell<double>(ipeak, i_height);
      double tmpwidth = m_funcParamWS->cell<double>(ipeak, i_width);
      double tmpcentre = m_funcParamWS->cell<double>(ipeak, i_centre);
      m_peakFunction->setHeight(tmpheight);
      m_peakFunction->setCentre(tmpcentre);
      m_peakFunction->setFwhm(tmpwidth);

      if (m_genBackground) {
        double tmpa0 = m_funcParamWS->cell<double>(ipeak, i_a0);
        double tmpa1 = m_funcParamWS->cell<double>(ipeak, i_a1);
        double tmpa2 = m_funcParamWS->cell<double>(ipeak, i_a2);
        m_bkgdFunction->setParameter("A0", tmpa0);
        m_bkgdFunction->setParameter("A1", tmpa1);
        m_bkgdFunction->setParameter("A2", tmpa2);
      }
    }

    double centre = m_peakFunction->centre();

    // Generate function to plot
    API::CompositeFunction_sptr plotfunc = std::make_shared<CompositeFunction>();
    plotfunc->addFunction(m_peakFunction);
    if (m_genBackground)
      plotfunc->addFunction(m_bkgdFunction);

    // Clone to another function
    API::IFunction_sptr clonefunction = plotfunc->clone();

    // Existed?
    mapiter = functionmap.find(wsindex);
    if (mapiter == functionmap.end()) {
      std::vector<std::pair<double, API::IFunction_sptr>> tempvector;
      std::pair<std::map<specnum_t, std::vector<std::pair<double, API::IFunction_sptr>>>::iterator, bool> ret;
      ret = functionmap.emplace(wsindex, tempvector);
      mapiter = ret.first;
    }

    // Generate peak function
    mapiter->second.emplace_back(centre, clonefunction);

    g_log.information() << "Peak " << ipeak << ": Spec = " << wsindex << " func: " << clonefunction->asString() << "\n";

  } // ENDFOR (ipeak)

  // Sort by peak position
  for (mapiter = functionmap.begin(); mapiter != functionmap.end(); ++mapiter) {
    std::vector<std::pair<double, API::IFunction_sptr>> &vec_centrefunc = mapiter->second;
    std::sort(vec_centrefunc.begin(), vec_centrefunc.end());
  }
}

//----------------------------------------------------------------------------------------------
/** Import one and only one function vector input
 */
void GeneratePeaks::importPeakFromVector(std::vector<std::pair<double, API::IFunction_sptr>> &functionmap) {
  API::CompositeFunction_sptr compfunc = std::make_shared<API::CompositeFunction>();

  // Set up and clone peak function
  if (m_useRawParameter) {
    // Input vector of values are for raw parameter name
    size_t numpeakparams = m_peakFunction->nParams();
    if (m_vecPeakParamValues.size() == numpeakparams) {
      for (size_t i = 0; i < numpeakparams; ++i)
        m_peakFunction->setParameter(i, m_vecPeakParamValues[i]);
    } else {
      // Number of input parameter values is not correct. Throw!
      std::stringstream errss;
      errss << "Number of input peak parameters' value (" << m_vecPeakParamValues.size()
            << ") is not correct (should be " << numpeakparams << " for peak of type " << m_peakFunction->name()
            << "). ";
      throw std::runtime_error(errss.str());
    }
  } else {
    // Input vector of values are for effective parameter names
    if (m_vecPeakParamValues.size() != 3)
      throw std::runtime_error("Input peak parameters must have 3 numbers for "
                               "effective parameter names.");

    m_peakFunction->setCentre(m_vecPeakParamValues[0]);
    m_peakFunction->setHeight(m_vecPeakParamValues[1]);
    m_peakFunction->setFwhm(m_vecPeakParamValues[2]);
  }
  compfunc->addFunction(m_peakFunction->clone());

  // Set up and clone background function
  if (m_genBackground) {
    size_t numbkgdparams = m_bkgdFunction->nParams();
    if (m_useRawParameter) {
      // Raw background parameters
      if (m_vecBkgdParamValues.size() != numbkgdparams)
        throw std::runtime_error("Number of background parameters' value is not correct. ");
      else {
        for (size_t i = 0; i < numbkgdparams; ++i)
          m_bkgdFunction->setParameter(i, m_vecBkgdParamValues[i]);
      }
    } else {
      // Effective background parameters
      if (m_vecBkgdParamValues.size() < 3 && m_vecBkgdParamValues.size() < numbkgdparams) {
        throw std::runtime_error("There is no enough effective background parameter values.");
      }

      // FIXME - Assume that all background functions define parameter i for A_i
      for (size_t i = 0; i < numbkgdparams; ++i)
        m_bkgdFunction->setParameter(i, m_vecBkgdParamValues[i]);
    }

    compfunc->addFunction(m_bkgdFunction->clone());
  }

  // Set up function map
  functionmap.emplace_back(m_peakFunction->centre(), compfunc);
}

//----------------------------------------------------------------------------------------------
/** Generate peaks in the given output workspace
 * @param functionmap :: map to contain the list of functions with key as their
 * spectra
 * @param dataWS :: output matrix workspace
 */
void GeneratePeaks::generatePeaks(
    const std::map<specnum_t, std::vector<std::pair<double, API::IFunction_sptr>>> &functionmap,
    const API::MatrixWorkspace_sptr &dataWS) {
  // Calcualte function
  std::map<specnum_t, std::vector<std::pair<double, API::IFunction_sptr>>>::const_iterator mapiter;
  for (mapiter = functionmap.begin(); mapiter != functionmap.end(); ++mapiter) {
    // Get spec id and translated to wsindex in the output workspace
    specnum_t specid = mapiter->first;
    specnum_t wsindex;
    if (m_newWSFromParent)
      wsindex = specid;
    else
      wsindex = m_SpectrumMap[specid];

    const std::vector<std::pair<double, API::IFunction_sptr>> &vec_centrefunc = mapiter->second;
    size_t numpeaksinspec = mapiter->second.size();

    for (size_t ipeak = 0; ipeak < numpeaksinspec; ++ipeak) {
      const std::pair<double, API::IFunction_sptr> &centrefunc = vec_centrefunc[ipeak];

      // Determine boundary
      API::IPeakFunction_sptr thispeak = getPeakFunction(centrefunc.second);
      double centre = centrefunc.first;
      double fwhm = thispeak->fwhm();

      //
      const auto &X = dataWS->x(wsindex);
      double leftbound = centre - m_numPeakWidth * fwhm;
      if (ipeak > 0) {
        // Not left most peak.
        API::IPeakFunction_sptr leftPeak = getPeakFunction(vec_centrefunc[ipeak - 1].second);

        double middle = 0.5 * (centre + leftPeak->centre());
        if (leftbound < middle)
          leftbound = middle;
      }
      auto left = std::lower_bound(X.cbegin(), X.cend(), leftbound);
      if (left == X.end())
        left = X.begin();

      double rightbound = centre + m_numPeakWidth * fwhm;
      if (ipeak != numpeaksinspec - 1) {
        // Not the rightmost peak
        IPeakFunction_sptr rightPeak = getPeakFunction(vec_centrefunc[ipeak + 1].second);
        double middle = 0.5 * (centre + rightPeak->centre());
        if (rightbound > middle)
          rightbound = middle;
      }
      auto right = std::lower_bound(left + 1, X.cend(), rightbound);

      // Build domain & function
      API::FunctionDomain1DVector domain(left,
                                         right); // dataWS->dataX(wsindex));

      // Evaluate the function
      API::FunctionValues values(domain);
      centrefunc.second->function(domain, values);

      // Put to output
      std::size_t offset = (left - X.begin());
      std::size_t numY = values.size();

      auto &dataY = dataWS->mutableY(wsindex);
      for (std::size_t i = 0; i < numY; i++) {
        dataY[i + offset] += values[i];
      }

    } // ENDFOR(ipeak)
  }
}

//----------------------------------------------------------------------------------------------
/** Create a function for fitting.
 */
void GeneratePeaks::createFunction(std::string &peaktype, std::string &bkgdtype) {
  // Create peak function
  m_peakFunction = std::dynamic_pointer_cast<IPeakFunction>(API::FunctionFactory::Instance().createFunction(peaktype));

  // create the background
  if (m_genBackground)
    m_bkgdFunction =
        std::dynamic_pointer_cast<IBackgroundFunction>(API::FunctionFactory::Instance().createFunction(bkgdtype));
}

//----------------------------------------------------------------------------------------------
/** Process table column names for peak and background function parameters names
 */
void GeneratePeaks::processTableColumnNames() {
  using namespace boost::algorithm;

  // Initial check
  std::vector<std::string> colnames = m_funcParamWS->getColumnNames();

  if (colnames[0] != "spectrum")
    throw std::runtime_error("First column must be 'spectrum' in integer. ");
  if (colnames.back() != "chi2")
    throw std::runtime_error("Last column must be 'chi2'.");

  // Process column names in case that there are not same as parameter names
  // fx.name might be available
  m_funcParameterNames.resize(colnames.size() - 2);
  for (size_t i = 0; i < colnames.size() - 2; ++i) {
    std::string str = colnames[i + 1];
    std::vector<std::string> tokens;
    split(tokens, str, is_any_of(".")); // here it is
    m_funcParameterNames[i] = tokens.back();
  }

  // Check column number
  size_t numparamcols = colnames.size() - 2;
  if (m_useRawParameter) {
    // Number of parameters must be peak parameters + background parameters
    size_t numpeakparams = m_peakFunction->nParams();
    size_t numbkgdparams = 0;
    if (m_genBackground)
      numbkgdparams = m_bkgdFunction->nParams();
    size_t numparams = numpeakparams + numbkgdparams;
    if (numparamcols < numparams)
      throw std::runtime_error("Parameters number is not correct. ");
    else if (numparamcols > numparams)
      g_log.warning("Number of parameters given in table workspace is more than "
                    "number of parameters of function(s) to generate peaks. ");

    // Check column names are same as function parameter naems
    for (size_t i = 0; i < numpeakparams; ++i) {
      if (!hasParameter(m_peakFunction, m_funcParameterNames[i])) {
        std::stringstream errss;
        errss << "Peak function " << m_peakFunction->name() << " does not have paramter " << m_funcParameterNames[i]
              << "\n"
              << "Allowed function parameters are ";
        std::vector<std::string> parnames = m_peakFunction->getParameterNames();
        for (auto &parname : parnames)
          errss << parname << ", ";
        throw std::runtime_error(errss.str());
      }
    }

    // Background function
    for (size_t i = 0; i < numbkgdparams; ++i) {
      if (!hasParameter(m_bkgdFunction, m_funcParameterNames[i + numpeakparams])) {
        std::stringstream errss;
        errss << "Background function does not have paramter " << m_funcParameterNames[i + numpeakparams];
        throw std::runtime_error(errss.str());
      }
    }
  } else {
    // Effective parameter names
    if (numparamcols != 6)
      throw std::runtime_error("Number of columns must be 6 if not using raw.");

    // Find the column index of height, width, centre, a0, a1 and a2
    i_height = static_cast<int>(std::find(m_funcParameterNames.begin(), m_funcParameterNames.end(), "height") -
                                m_funcParameterNames.begin()) +
               1;
    i_centre = static_cast<int>(std::find(m_funcParameterNames.begin(), m_funcParameterNames.end(), "centre") -
                                m_funcParameterNames.begin()) +
               1;
    i_width = static_cast<int>(std::find(m_funcParameterNames.begin(), m_funcParameterNames.end(), "width") -
                               m_funcParameterNames.begin()) +
              1;
    i_a0 = static_cast<int>(std::find(m_funcParameterNames.begin(), m_funcParameterNames.end(), "backgroundintercept") -
                            m_funcParameterNames.begin()) +
           1;
    i_a1 = static_cast<int>(std::find(m_funcParameterNames.begin(), m_funcParameterNames.end(), "backgroundslope") -
                            m_funcParameterNames.begin()) +
           1;
    i_a2 = static_cast<int>(std::find(m_funcParameterNames.begin(), m_funcParameterNames.end(), "A2") -
                            m_funcParameterNames.begin()) +
           1;
  }
}

//----------------------------------------------------------------------------------------------
/** Get set of spectra of the input table workspace
 * Spectra is set to the column named 'spectrum'.
 * Algorithm supports multiple peaks in multiple spectra
 */
void GeneratePeaks::getSpectraSet(const DataObjects::TableWorkspace_const_sptr &peakParmsWS) {
  size_t numpeaks = peakParmsWS->rowCount();
  API::Column_const_sptr col = peakParmsWS->getColumn("spectrum");

  for (size_t ipk = 0; ipk < numpeaks; ipk++) {
    // Spectrum
    auto specid = static_cast<specnum_t>((*col)[ipk]);
    m_spectraSet.insert(specid);

    std::stringstream outss;
    outss << "Peak " << ipk << ": specid = " << specid;
    g_log.debug(outss.str());
  }

  specnum_t icount = 0;
  for (const auto specnum : m_spectraSet) {
    m_SpectrumMap.emplace(specnum, icount);
    ++icount;
  }
}

//----------------------------------------------------------------------------------------------
/** Get the IPeakFunction part in the input function
 */
API::IPeakFunction_sptr GeneratePeaks::getPeakFunction(const API::IFunction_sptr &infunction) {
  // Not a composite function
  API::CompositeFunction_sptr compfunc = std::dynamic_pointer_cast<API::CompositeFunction>(infunction);

  // If it is a composite function (complete part for return)
  if (compfunc) {
    for (size_t i = 0; i < compfunc->nFunctions(); ++i) {
      API::IFunction_sptr subfunc = compfunc->getFunction(i);
      API::IPeakFunction_sptr peakfunc = std::dynamic_pointer_cast<API::IPeakFunction>(subfunc);
      if (peakfunc)
        return peakfunc;
    }
  }

  // Return if not a composite function
  API::IPeakFunction_sptr peakfunc = std::dynamic_pointer_cast<API::IPeakFunction>(infunction);

  return peakfunc;
}

//----------------------------------------------------------------------------------------------
/** Find out whether a function has a certain parameter
 */
bool GeneratePeaks::hasParameter(const API::IFunction_sptr &function, const std::string &paramname) {
  std::vector<std::string> parnames = function->getParameterNames();
  std::vector<std::string>::iterator piter;
  piter = std::find(parnames.begin(), parnames.end(), paramname);
  return piter != parnames.end();
}

//----------------------------------------------------------------------------------------------
/** Create output workspace
 */
API::MatrixWorkspace_sptr GeneratePeaks::createOutputWorkspace() {
  // Reference workspace and output workspace
  API::MatrixWorkspace_sptr outputWS;

  m_newWSFromParent = true;
  if (!inputWS && binParameters.empty()) {
    // Error! Neither bin parameters or reference workspace is given.
    std::string errmsg("Must define either InputWorkspace or BinningParameters.");
    g_log.error(errmsg);
    throw std::invalid_argument(errmsg);
  } else if (inputWS) {
    // Generate Workspace2D from input workspace
    if (!binParameters.empty())
      g_log.notice() << "Both binning parameters and input workspace are given. "
                     << "Using input worksapce to generate output workspace!\n";

    HistogramBuilder builder;
    builder.setX(inputWS->x(0).size());
    builder.setY(inputWS->y(0).size());

    builder.setDistribution(inputWS->isDistribution());
    outputWS = create<MatrixWorkspace>(*inputWS, builder.build());
    // Only copy the X-values from spectra with peaks specified in the table
    // workspace.
    for (const auto &iws : m_spectraSet) {
      outputWS->setSharedX(iws, inputWS->sharedX(iws));
    }

    m_newWSFromParent = true;
  } else {
    // Generate a one-spectrum Workspace2D from binning
    outputWS = createDataWorkspace(binParameters);
    m_newWSFromParent = false;
  }

  return outputWS;
}

//----------------------------------------------------------------------------------------------
/** Create a Workspace2D (MatrixWorkspace) with given spectra and bin parameters
 */
MatrixWorkspace_sptr GeneratePeaks::createDataWorkspace(const std::vector<double> &binparameters) const {
  // Check validity
  if (m_spectraSet.empty())
    throw std::invalid_argument("Input spectra list is empty. Unable to generate a new workspace.");

  if (binparameters.size() < 3) {
    std::stringstream errss;
    errss << "Number of input binning parameters are not enough (" << binparameters.size() << "). "
          << "Binning parameters should be 3 (x0, step, xf).";
    g_log.error(errss.str());
    throw std::invalid_argument(errss.str());
  }

  double x0 = binparameters[0];
  double dx = binparameters[1]; // binDelta
  double xf = binparameters[2]; // max value
  if (x0 >= xf || (xf - x0) < dx || dx == 0.) {
    std::stringstream errss;
    errss << "Order of input binning parameters is not correct.  It is not "
             "logical to have "
          << "x0 = " << x0 << ", xf = " << xf << ", dx = " << dx;
    g_log.error(errss.str());
    throw std::invalid_argument(errss.str());
  }

  std::vector<double> xarray;
  double xvalue = x0;

  while (xvalue <= xf) {
    // Push current value to vector
    xarray.emplace_back(xvalue);

    // Calculate next value, linear or logarithmic
    if (dx > 0)
      xvalue += dx;
    else
      xvalue += fabs(dx) * xvalue;
  }

  std::vector<Indexing::SpectrumNumber> specNums;
  for (const auto &item : m_SpectrumMap) {
    specnum_t specid = item.first;
    g_log.debug() << "Build WorkspaceIndex-Spectrum  " << specNums.size() << " , " << specid << "\n";
    specNums.emplace_back(specid);
  }

  Indexing::IndexInfo indices(specNums.size());
  // There is no instrument, so the automatic build of a 1:1 mapping would fail.
  // Need to set empty grouping manually.
  indices.setSpectrumDefinitions(std::vector<SpectrumDefinition>(specNums.size()));
  indices.setSpectrumNumbers(std::move(specNums));
  return create<Workspace2D>(indices, BinEdges(std::move(xarray)));
}

/** Add function's parameter names after peak function name
 */
std::vector<std::string> GeneratePeaks::addFunctionParameterNames(const std::vector<std::string> &funcnames) {
  std::vector<std::string> vec_funcparnames;

  for (auto &funcname : funcnames) {
    // Add original name in
    vec_funcparnames.emplace_back(funcname);

    // Add a full function name and parameter names in
    IFunction_sptr tempfunc = FunctionFactory::Instance().createFunction(funcname);

    std::stringstream parnamess;
    parnamess << funcname << " (";
    std::vector<std::string> funcpars = tempfunc->getParameterNames();
    for (size_t j = 0; j < funcpars.size(); ++j) {
      parnamess << funcpars[j];
      if (j != funcpars.size() - 1)
        parnamess << ", ";
    }
    parnamess << ")";

    vec_funcparnames.emplace_back(parnamess.str());
  }

  return vec_funcparnames;
}

} // namespace Mantid::Algorithms
