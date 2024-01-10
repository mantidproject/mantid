// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/StringTokenizer.h"
#include <algorithm>
#include <boost/algorithm/string/replace.hpp>
#include <boost/lexical_cast.hpp>
#include <cmath>
#include <fstream>
#include <sstream>
#include <vector>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/BinEdgeAxis.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/CostFunctionFactory.h"
#include "MantidAPI/FuncMinimizerFactory.h"
#include "MantidAPI/FunctionProperty.h"
#include "MantidAPI/IFuncMinimizer.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidCurveFitting/Algorithms/PlotPeakByLogValue.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/TimeSeriesProperty.h"

namespace {
Mantid::Kernel::Logger g_log("PlotPeakByLogValue");
}

namespace Mantid::CurveFitting::Algorithms {

using namespace Kernel;
using namespace API;

// Register the class into the algorithm factory
DECLARE_ALGORITHM(PlotPeakByLogValue)

/** Initialisation method. Declares properties to be used in algorithm.
 *
 */
void PlotPeakByLogValue::init() {
  declareProperty("Input", "", std::make_shared<MandatoryValidator<std::string>>(),
                  "A list of sources of data to fit. \n"
                  "Sources can be either workspace names or file names followed optionally "
                  "by a list of spectra/workspace-indices \n"
                  "or values using the notation described in the description section of "
                  "the help page.");

  declareProperty("Spectrum", 1,
                  "Set a spectrum to fit. \n"
                  "However, if spectra lists (or workspace-indices/values "
                  "lists) are specified in the Input parameter string these "
                  "take precedence.");
  declareProperty("WorkspaceIndex", 0,
                  "Set a workspace-index to fit (alternative option to Spectrum). "
                  "However, if spectra lists (or workspace-indices/values lists) are "
                  "specified in the Input parameter string, \n"
                  "or the Spectrum parameter integer, these take precedence.");
  declareProperty(std::make_unique<WorkspaceProperty<ITableWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "The output TableWorkspace");
  declareProperty(std::make_unique<API::FunctionProperty>("Function", Direction::InOut),
                  "Parameters defining the fitting function and its initial values");
  declareProperty("LogValue", "",
                  "Name of the log value to plot the "
                  "parameters against. Default: use spectra "
                  "numbers.");
  declareProperty(std::make_unique<ArrayProperty<double>>("StartX"), "A value of x in, or on the low x "
                                                                     "boundary of, the first bin to "
                                                                     "include in\n"
                                                                     "the fit (default lowest value of x)");
  declareProperty(std::make_unique<ArrayProperty<double>>("EndX"), "A value in, or on the high x boundary "
                                                                   "of, the last bin the fitting range\n"
                                                                   "(default the highest value of x)");

  std::vector<std::string> fitOptions{"Sequential", "Individual"};
  declareProperty("FitType", "Sequential", std::make_shared<StringListValidator>(fitOptions),
                  "Defines the way of setting initial values. \n"
                  "If set to 'Sequential' every next fit starts with "
                  "parameters returned by the previous fit. \n"
                  "If set to 'Individual' each fit starts with the same "
                  "initial values defined in the Function property.");

  declareProperty("PassWSIndexToFunction", false,
                  "For each spectrum in Input pass its workspace index to all "
                  "functions that"
                  "have attribute WorkspaceIndex.");

  declareProperty("Minimizer", "Levenberg-Marquardt",
                  "Minimizer to use for fitting. Minimizers available are "
                  "'Levenberg-Marquardt', 'Simplex', 'FABADA',\n"
                  "'Conjugate gradient (Fletcher-Reeves imp.)', 'Conjugate "
                  "gradient (Polak-Ribiere imp.)' and 'BFGS'");

  std::vector<std::string> costFuncOptions = CostFunctionFactory::Instance().getKeys();
  declareProperty("CostFunction", "Least squares", std::make_shared<StringListValidator>(costFuncOptions),
                  "Cost functions to use for fitting. Cost functions available "
                  "are 'Least squares' and 'Ignore positive peaks'",
                  Direction::InOut);

  declareProperty("MaxIterations", 500,
                  "Stop after this number of iterations if a good fit is not "
                  "found");
  declareProperty("PeakRadius", 0,
                  "A value of the peak radius the peak functions should use. A "
                  "peak radius defines an interval on the x axis around the "
                  "centre of the peak where its values are calculated. Values "
                  "outside the interval are not calculated and assumed zeros."
                  "Numerically the radius is a whole number of peak widths "
                  "(FWHM) that fit into the interval on each side from the "
                  "centre. The default value of 0 means the whole x axis.");

  declareProperty("CreateOutput", false,
                  "Set to true to create output "
                  "workspaces with the results of the "
                  "fit(default is false).");

  declareProperty("OutputCompositeMembers", false,
                  "If true and CreateOutput is true then the value of each "
                  "member of a Composite Function is also output.");

  declareProperty(std::make_unique<Kernel::PropertyWithValue<bool>>("ConvolveMembers", false),
                  "If true and OutputCompositeMembers is true members of any "
                  "Convolution are output convolved\n"
                  "with corresponding resolution");

  std::array<std::string, 2> evaluationTypes = {{"CentrePoint", "Histogram"}};
  declareProperty("EvaluationType", "CentrePoint",
                  Kernel::IValidator_sptr(new Kernel::ListValidator<std::string>(evaluationTypes)),
                  "The way the function is evaluated: CentrePoint or Histogram.", Kernel::Direction::Input);

  declareProperty(std::make_unique<ArrayProperty<double>>("Exclude", ""),
                  "A list of pairs of real numbers, defining the regions to "
                  "exclude from the fit for all spectra.");

  declareProperty(std::make_unique<ArrayProperty<std::string>>("ExcludeMultiple", ""),
                  "A list of Exclusion ranges, defining the regions to "
                  "exclude from the fit for each spectra. Must have the "
                  "same number of sets as the number of the spectra.");

  declareProperty("IgnoreInvalidData", false, "Flag to ignore infinities, NaNs and data with zero errors.");

  declareProperty("OutputFitStatus", false,
                  "Flag to output fit status information which consists of the fit "
                  "OutputStatus and the OutputChiSquared");
}

std::map<std::string, std::string> PlotPeakByLogValue::validateInputs() {
  std::map<std::string, std::string> errors;
  std::string inputList = getPropertyValue("Input");
  int default_wi = getProperty("WorkspaceIndex");
  int default_spec = getProperty("Spectrum");
  const std::vector<InputSpectraToFit> wsNames = makeNames(inputList, default_wi, default_spec);
  std::vector<std::string> excludeList = getProperty("ExcludeMultiple");
  if (!excludeList.empty() && excludeList.size() != wsNames.size()) {
    errors["ExcludeMultiple"] = "ExcludeMultiple must be the same size has the number of spectra.";
  }
  return errors;
}

/**
 *   Executes the algorithm
 */
void PlotPeakByLogValue::exec() {

  // Create a list of the input workspace
  std::string inputList = getPropertyValue("Input");
  int default_wi = getProperty("WorkspaceIndex");
  int default_spec = getProperty("Spectrum");
  const std::vector<InputSpectraToFit> wsNames = makeNames(inputList, default_wi, default_spec);

  std::string logName = getProperty("LogValue");
  bool individual = getPropertyValue("FitType") == "Individual";
  bool passWSIndexToFunction = getProperty("PassWSIndexToFunction");
  bool createFitOutput = getProperty("CreateOutput");
  bool outputCompositeMembers = getProperty("OutputCompositeMembers");
  bool outputConvolvedMembers = getProperty("ConvolveMembers");
  bool outputFitStatus = getProperty("OutputFitStatus");
  m_baseName = getPropertyValue("OutputWorkspace");
  std::vector<double> startX = getProperty("StartX");
  std::vector<double> endX = getProperty("EndX");
  std::vector<std::string> exclude = getExclude(wsNames.size());

  bool isDataName = false; // if true first output column is of type string and
                           // is the data source name
  // Create an instance of the fitting function to obtain the names of fitting
  // parameters
  IFunction_sptr inputFunction = getProperty("Function");
  if (!inputFunction) {
    throw std::invalid_argument("Fitting function failed to initialize");
  }
  bool isMultiDomainFunction = std::dynamic_pointer_cast<MultiDomainFunction>(inputFunction) != nullptr;

  IFunction_sptr ifunSingle = isMultiDomainFunction ? inputFunction->getFunction(0) : inputFunction;

  // for inidividual fittings store the initial parameters
  std::vector<double> initialParams(ifunSingle->nParams());
  if (individual) {
    for (size_t i = 0; i < initialParams.size(); ++i) {
      initialParams[i] = ifunSingle->getParameter(i);
    }
  }
  ITableWorkspace_sptr result = createResultsTable(logName, ifunSingle, isDataName);

  std::vector<MatrixWorkspace_sptr> fitWorkspaces;
  std::vector<ITableWorkspace_sptr> parameterWorkspaces;
  std::vector<ITableWorkspace_sptr> covarianceWorkspaces;
  if (createFitOutput) {
    covarianceWorkspaces.reserve(wsNames.size());
    fitWorkspaces.reserve(wsNames.size());
    parameterWorkspaces.reserve(wsNames.size());
  }

  std::vector<std::string> fitStatus;
  std::vector<double> fitChiSquared;
  if (outputFitStatus) {
    declareProperty(std::make_unique<ArrayProperty<std::string>>("OutputStatus", Direction::Output));
    declareProperty(std::make_unique<ArrayProperty<double>>("OutputChiSquared", Direction::Output));
    fitStatus.reserve(wsNames.size());
    fitChiSquared.reserve(wsNames.size());
  }

  double dProg = 1. / static_cast<double>(wsNames.size());
  double Prog = 0.;
  for (int i = 0; i < static_cast<int>(wsNames.size()); ++i) {
    InputSpectraToFit data = wsNames[i];

    if (!data.ws) {
      g_log.warning() << "Cannot access workspace " << data.name << '\n';
      continue;
    }

    if (data.i < 0) {
      g_log.warning() << "Zero spectra selected for fitting in workspace " << wsNames[i].name << '\n';
      continue;
    }

    IFunction_sptr ifun =
        setupFunction(individual, passWSIndexToFunction, inputFunction, initialParams, isMultiDomainFunction, i, data);
    std::shared_ptr<Algorithm> fit;
    if (startX.size() == 0) {
      fit = runSingleFit(createFitOutput, outputCompositeMembers, outputConvolvedMembers, ifun, data, EMPTY_DBL(),
                         EMPTY_DBL(), exclude[i]);
    } else if (startX.size() == 1) {
      fit = runSingleFit(createFitOutput, outputCompositeMembers, outputConvolvedMembers, ifun, data, startX[0],
                         endX[0], exclude[i]);
    } else {
      fit = runSingleFit(createFitOutput, outputCompositeMembers, outputConvolvedMembers, ifun, data, startX[i],
                         endX[i], exclude[i]);
    }

    ifun = fit->getProperty("Function");
    double chi2 = fit->getProperty("OutputChi2overDoF");

    if (createFitOutput) {
      MatrixWorkspace_sptr outputFitWorkspace = fit->getProperty("OutputWorkspace");
      ITableWorkspace_sptr outputParamWorkspace = fit->getProperty("OutputParameters");
      ITableWorkspace_sptr outputCovarianceWorkspace = fit->getProperty("OutputNormalisedCovarianceMatrix");
      fitWorkspaces.emplace_back(outputFitWorkspace);
      parameterWorkspaces.emplace_back(outputParamWorkspace);
      covarianceWorkspaces.emplace_back(outputCovarianceWorkspace);
    }
    if (outputFitStatus) {
      fitStatus.push_back(fit->getProperty("OutputStatus"));
      fitChiSquared.push_back(chi2);
    }

    g_log.debug() << "Fit result " << fit->getPropertyValue("OutputStatus") << ' ' << chi2 << '\n';

    // Find the log value: it is either a log-file value or
    // simply the workspace number
    double logValue = calculateLogValue(logName, data);
    appendTableRow(isDataName, result, ifun, data, logValue, chi2);

    Prog += dProg;
    std::string current = std::to_string(i);
    progress(Prog, ("Fitting Workspace: (" + current + ") - "));
    interruption_point();
  }

  if (outputFitStatus) {
    setProperty("OutputStatus", fitStatus);
    setProperty("OutputChiSquared", fitChiSquared);
  }

  finaliseOutputWorkspaces(createFitOutput, fitWorkspaces, parameterWorkspaces, covarianceWorkspaces);
}

IFunction_sptr PlotPeakByLogValue::setupFunction(bool individual, bool passWSIndexToFunction,
                                                 const IFunction_sptr &inputFunction,
                                                 const std::vector<double> &initialParams, bool isMultiDomainFunction,
                                                 int i, const InputSpectraToFit &data) const {
  IFunction_sptr ifun;
  if (isMultiDomainFunction) {
    ifun = inputFunction->getFunction(i);
    if (!individual && i != 0) {
      IFunction_sptr prevFunction = inputFunction->getFunction(i - 1);
      for (size_t k = 0; k < ifun->nParams(); ++k) {
        ifun->setParameter(k, prevFunction->getParameter(k));
      }
    }

  } else {
    ifun = inputFunction;
  }
  if (passWSIndexToFunction) {
    this->setWorkspaceIndexAttribute(ifun, data.i);
  }

  if (individual && !isMultiDomainFunction) {
    for (size_t k = 0; k < initialParams.size(); ++k) {
      ifun->setParameter(k, initialParams[k]);
    }
  }
  return ifun;
}

void PlotPeakByLogValue::finaliseOutputWorkspaces(bool createFitOutput,
                                                  const std::vector<MatrixWorkspace_sptr> &fitWorkspaces,
                                                  const std::vector<ITableWorkspace_sptr> &parameterWorkspaces,
                                                  const std::vector<ITableWorkspace_sptr> &covarianceWorkspaces) {
  if (createFitOutput) {
    // collect output of fit for each spectrum into workspace groups
    WorkspaceGroup_sptr covarianceGroup = std::make_shared<WorkspaceGroup>();
    for (auto const &workspace : covarianceWorkspaces)
      covarianceGroup->addWorkspace(workspace);
    AnalysisDataService::Instance().addOrReplace(this->m_baseName + "_NormalisedCovarianceMatrices", covarianceGroup);

    WorkspaceGroup_sptr parameterGroup = std::make_shared<WorkspaceGroup>();
    for (auto const &workspace : parameterWorkspaces)
      parameterGroup->addWorkspace(workspace);
    AnalysisDataService::Instance().addOrReplace(this->m_baseName + "_Parameters", parameterGroup);

    WorkspaceGroup_sptr fitGroup = std::make_shared<WorkspaceGroup>();
    for (auto const &workspace : fitWorkspaces)
      fitGroup->addWorkspace(workspace);
    AnalysisDataService::Instance().addOrReplace(this->m_baseName + "_Workspaces", fitGroup);
  }

  for (auto &minimizerWorkspace : this->m_minimizerWorkspaces) {
    const std::string paramName = minimizerWorkspace.first;
    auto groupAlg = this->createChildAlgorithm("GroupWorkspaces");
    groupAlg->initialize();
    groupAlg->setProperty("InputWorkspaces", minimizerWorkspace.second);
    groupAlg->setProperty("OutputWorkspace", this->m_baseName + "_" + paramName);
    groupAlg->execute();
  }
}

void PlotPeakByLogValue::appendTableRow(
    bool isDataName, ITableWorkspace_sptr &result, const IFunction_sptr &ifun, const InputSpectraToFit &data,
    double logValue, double chi2) const { // Extract the fitted parameters and put them into the result table
  TableRow row = result->appendRow();
  if (isDataName) {
    row << data.name;
  } else {
    row << logValue;
  }

  auto p = std::dynamic_pointer_cast<API::CompositeFunction>(ifun);
  if (p) {
    for (size_t i = 0; i < p->nFunctions(); ++i) {
      auto f = ifun->getFunction(i);
      for (size_t j = 0; j < f->nParams(); ++j) {
        row << p->getParameter(i, j) << p->getError(i, j);
      }

      /* Output integrated intensity */
      auto intensity_handle = std::dynamic_pointer_cast<API::IPeakFunction>(f);
      if (intensity_handle) {
        row << intensity_handle->intensity() << intensity_handle->intensityError();
      }
    }
  }

  else {
    for (size_t iPar = 0; iPar < ifun->nParams(); ++iPar) {
      row << ifun->getParameter(iPar) << ifun->getError(iPar);
    }

    /* Output integrated intensity */
    auto intensity_handle = std::dynamic_pointer_cast<API::IPeakFunction>(ifun);
    if (intensity_handle) {
      row << intensity_handle->intensity() << intensity_handle->intensityError();
    }
  }

  row << chi2;
}

ITableWorkspace_sptr PlotPeakByLogValue::createResultsTable(const std::string &logName,
                                                            const IFunction_sptr &ifunSingle, bool &isDataName) {
  ITableWorkspace_sptr result = WorkspaceFactory::Instance().createTable("TableWorkspace");
  if (logName == "SourceName") {
    result->addColumn("str", "SourceName");
    isDataName = true;
  } else if (logName.empty()) {
    auto col = result->addColumn("double", "axis-1");
    col->setPlotType(1); // X-values inplots
  } else {
    auto col = result->addColumn("double", logName);
    col->setPlotType(1); // X-values inplots
  }

  auto p = std::dynamic_pointer_cast<API::CompositeFunction>(ifunSingle);
  if (p) {
    for (size_t i = 0; i < p->nFunctions(); ++i) {
      auto f = ifunSingle->getFunction(i);
      for (size_t j = 0; j < f->nParams(); ++j) {
        result->addColumn("double", p->parameterName(i, j));
        result->addColumn("double", p->parameterName(i, j) + "_Err");
      }

      auto intensity_handle = std::dynamic_pointer_cast<API::IPeakFunction>(f);
      if (intensity_handle) {
        result->addColumn("double", "f" + std::to_string(i) + ".Integrated Intensity");
        result->addColumn("double", "f" + std::to_string(i) + ".Integrated Intensity_Err");
      }
    }
  }

  else {
    for (size_t iPar = 0; iPar < ifunSingle->nParams(); ++iPar) {
      result->addColumn("double", ifunSingle->parameterName(iPar));
      result->addColumn("double", ifunSingle->parameterName(iPar) + "_Err");
    }

    auto intensity_handle = std::dynamic_pointer_cast<API::IPeakFunction>(ifunSingle);
    if (intensity_handle) {
      result->addColumn("double", "Integrated Intensity");
      result->addColumn("double", "Integrated Intensity_Err");
    }
  }

  result->addColumn("double", "Chi_squared");

  this->setProperty("OutputWorkspace", result);
  return result;
}

std::shared_ptr<Algorithm> PlotPeakByLogValue::runSingleFit(bool createFitOutput, bool outputCompositeMembers,
                                                            bool outputConvolvedMembers, const IFunction_sptr &ifun,
                                                            const InputSpectraToFit &data, double startX, double endX,
                                                            const std::string &exclude) {
  g_log.debug() << "Fitting " << data.ws->getName() << " index " << data.i << " with \n";
  g_log.debug() << ifun->asString() << '\n';

  const std::string spectrum_index = std::to_string(data.i);
  std::string wsBaseName;

  if (createFitOutput)
    wsBaseName = data.name + "_" + spectrum_index;
  bool histogramFit = this->getPropertyValue("EvaluationType") == "Histogram";
  bool ignoreInvalidData = this->getProperty("IgnoreInvalidData");

  // Fit the function
  auto fit = this->createChildAlgorithm("Fit");
  fit->initialize();
  fit->setPropertyValue("EvaluationType", this->getPropertyValue("EvaluationType"));
  fit->setProperty("Function", ifun);
  fit->setProperty("InputWorkspace", data.ws);
  fit->setProperty("WorkspaceIndex", data.i);
  fit->setProperty("StartX", startX);
  fit->setProperty("EndX", endX);
  fit->setProperty("IgnoreInvalidData", ignoreInvalidData);
  fit->setPropertyValue("Minimizer", this->getMinimizerString(data.name, spectrum_index));
  fit->setPropertyValue("CostFunction", this->getPropertyValue("CostFunction"));
  fit->setPropertyValue("MaxIterations", this->getPropertyValue("MaxIterations"));
  fit->setPropertyValue("PeakRadius", this->getPropertyValue("PeakRadius"));
  fit->setProperty("CalcErrors", true);
  fit->setProperty("CreateOutput", createFitOutput);
  if (!histogramFit) {
    fit->setProperty("OutputCompositeMembers", outputCompositeMembers);
    fit->setProperty("ConvolveMembers", outputConvolvedMembers);
    fit->setProperty("Exclude", exclude);
  }
  fit->setProperty("Output", wsBaseName);
  fit->setRethrows(true);
  fit->execute();
  return fit;
}

double PlotPeakByLogValue::calculateLogValue(const std::string &logName, const InputSpectraToFit &data) {
  double logValue = 0;
  if (logName.empty() || logName == "axis-1") {
    API::Axis *axis = data.ws->getAxis(1);
    if (dynamic_cast<BinEdgeAxis *>(axis)) {
      double lowerEdge((*axis)(data.i));
      double upperEdge((*axis)(data.i + 1));
      logValue = lowerEdge + (upperEdge - lowerEdge) / 2;
    } else
      logValue = (*axis)(data.i);
  } else if (logName != "SourceName") {
    Kernel::Property *prop = data.ws->run().getLogData(logName);
    if (!prop) {
      throw std::invalid_argument("Log value " + logName + " does not exist");
    }
    auto *logp = dynamic_cast<TimeSeriesProperty<double> *>(prop);
    if (!logp) {
      throw std::runtime_error("Failed to cast " + logName + " to TimeSeriesProperty");
    }
    logValue = logp->lastValue();
  }
  return logValue;
}

void PlotPeakByLogValue::setWorkspaceIndexAttribute(const IFunction_sptr &fun, int wsIndex) const {
  const std::string attName = "WorkspaceIndex";
  if (fun->hasAttribute(attName)) {
    fun->setAttributeValue(attName, wsIndex);
  }

  API::CompositeFunction_sptr cf = std::dynamic_pointer_cast<API::CompositeFunction>(fun);
  if (cf) {
    for (size_t i = 0; i < cf->nFunctions(); ++i) {
      setWorkspaceIndexAttribute(cf->getFunction(i), wsIndex);
    }
  }
}

std::string PlotPeakByLogValue::getMinimizerString(const std::string &wsName, const std::string &wsIndex) {
  std::string format = getPropertyValue("Minimizer");
  std::string wsBaseName = wsName + "_" + wsIndex;
  boost::replace_all(format, "$wsname", wsName);
  boost::replace_all(format, "$wsindex", wsIndex);
  boost::replace_all(format, "$basename", wsBaseName);
  boost::replace_all(format, "$outputname", m_baseName);

  auto minimizer = FuncMinimizerFactory::Instance().createMinimizer(format);
  auto minimizerProps = minimizer->getProperties();
  for (auto &minimizerProp : minimizerProps) {
    auto *wsProp = dynamic_cast<Mantid::API::WorkspaceProperty<> *>(minimizerProp);
    if (wsProp) {
      const std::string &wsPropValue = minimizerProp->value();
      if (!wsPropValue.empty()) {
        const std::string &wsPropName = minimizerProp->name();
        m_minimizerWorkspaces[wsPropName].emplace_back(wsPropValue);
      }
    }
  }

  return format;
}

std::vector<std::string> PlotPeakByLogValue::getExclude(const size_t numSpectra) {
  std::vector<std::string> excludeList = getProperty("ExcludeMultiple");
  if (excludeList.empty()) {
    std::string exclude = getPropertyValue("Exclude");
    std::vector<std::string> excludeVector(numSpectra, exclude);
    return excludeVector;
  } else {
    return excludeList;
  }
}

} // namespace Mantid::CurveFitting::Algorithms
