// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
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
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FunctionProperty.h"
#include "MantidAPI/IFuncMinimizer.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidCurveFitting/Algorithms/PlotPeakByLogValue.h"
#include "MantidCurveFitting/Algorithms/PlotPeakByLogValueHelper.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/TimeSeriesProperty.h"

namespace {
Mantid::Kernel::Logger g_log("PlotPeakByLogValue");
}

namespace Mantid {
namespace CurveFitting {
namespace Algorithms {

using namespace Kernel;
using namespace API;

// Register the class into the algorithm factory
DECLARE_ALGORITHM(PlotPeakByLogValue)

/** Initialisation method. Declares properties to be used in algorithm.
 *
 */
void PlotPeakByLogValue::init() {
  declareProperty(
      "Input", "", boost::make_shared<MandatoryValidator<std::string>>(),
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
  declareProperty(
      "WorkspaceIndex", 0,
      "Set a workspace-index to fit (alternative option to Spectrum). "
      "However, if spectra lists (or workspace-indices/values lists) are "
      "specified in the Input parameter string, \n"
      "or the Spectrum parameter integer, these take precedence.");
  declareProperty(std::make_unique<WorkspaceProperty<ITableWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "The output TableWorkspace");
  declareProperty(
      std::make_unique<API::FunctionProperty>("Function", Direction::InOut),
      "Parameters defining the fitting function and its initial values");
  declareProperty("LogValue", "",
                  "Name of the log value to plot the "
                  "parameters against. Default: use spectra "
                  "numbers.");
  declareProperty("StartX", EMPTY_DBL(),
                  "A value of x in, or on the low x "
                  "boundary of, the first bin to "
                  "include in\n"
                  "the fit (default lowest value of x)");
  declareProperty("EndX", EMPTY_DBL(),
                  "A value in, or on the high x boundary "
                  "of, the last bin the fitting range\n"
                  "(default the highest value of x)");

  std::vector<std::string> fitOptions{"Sequential", "Individual"};
  declareProperty("FitType", "Sequential",
                  boost::make_shared<StringListValidator>(fitOptions),
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

  std::vector<std::string> costFuncOptions =
      CostFunctionFactory::Instance().getKeys();
  declareProperty("CostFunction", "Least squares",
                  boost::make_shared<StringListValidator>(costFuncOptions),
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

  declareProperty(std::make_unique<Kernel::PropertyWithValue<bool>>(
                      "ConvolveMembers", false),
                  "If true and OutputCompositeMembers is true members of any "
                  "Convolution are output convolved\n"
                  "with corresponding resolution");

  std::array<std::string, 2> evaluationTypes = {{"CentrePoint", "Histogram"}};
  declareProperty(
      "EvaluationType", "CentrePoint",
      Kernel::IValidator_sptr(
          new Kernel::ListValidator<std::string>(evaluationTypes)),
      "The way the function is evaluated: CentrePoint or Histogram.",
      Kernel::Direction::Input);

  declareProperty(std::make_unique<ArrayProperty<double>>("Exclude", ""),
                  "A list of pairs of real numbers, defining the regions to "
                  "exclude from the fit.");

  declareProperty("IgnoreInvalidData", false,
                  "Flag to ignore infinities, NaNs and data with zero errors.");
}

/**
 *   Executes the algorithm
 */
void PlotPeakByLogValue::exec() {

  // Create a list of the input workspace
  std::string inputList = getPropertyValue("Input");
  int default_wi = getProperty("WorkspaceIndex");
  int default_spec = getProperty("Spectrum");
  const std::vector<InputData> wsNames =
      makeNames(inputList, default_wi, default_spec);

  const std::vector<double> exclude = getProperty("Exclude");
  std::string logName = getProperty("LogValue");
  bool individual = getPropertyValue("FitType") == "Individual";
  bool passWSIndexToFunction = getProperty("PassWSIndexToFunction");
  bool createFitOutput = getProperty("CreateOutput");
  bool outputCompositeMembers = getProperty("OutputCompositeMembers");
  bool outputConvolvedMembers = getProperty("ConvolveMembers");
  m_baseName = getPropertyValue("OutputWorkspace");

  bool isDataName = false; // if true first output column is of type string and
                           // is the data source name
  ITableWorkspace_sptr result =
      WorkspaceFactory::Instance().createTable("TableWorkspace");
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
  // Create an instance of the fitting function to obtain the names of fitting
  // parameters
  IFunction_sptr inputFunction = getProperty("Function");
  if (!inputFunction) {
    throw std::invalid_argument("Fitting function failed to initialize");
  }
  bool isMultiDomainFunction = boost::dynamic_pointer_cast<MultiDomainFunction>(
                                   inputFunction) != nullptr;

  IFunction_sptr ifunSingle =
      isMultiDomainFunction ? inputFunction->getFunction(0) : inputFunction;

  // for inidividual fittings store the initial parameters
  std::vector<double> initialParams(ifunSingle->nParams());
  if (individual) {
    for (size_t i = 0; i < initialParams.size(); ++i) {
      initialParams[i] = ifunSingle->getParameter(i);
    }
  }

  for (size_t iPar = 0; iPar < ifunSingle->nParams(); ++iPar) {
    result->addColumn("double", ifunSingle->parameterName(iPar));
    result->addColumn("double", ifunSingle->parameterName(iPar) + "_Err");
  }
  result->addColumn("double", "Chi_squared");

  setProperty("OutputWorkspace", result);

  std::vector<MatrixWorkspace_sptr> fitWorkspaces;
  std::vector<ITableWorkspace_sptr> parameterWorkspaces;
  std::vector<ITableWorkspace_sptr> covarianceWorkspaces;

  double dProg = 1. / static_cast<double>(wsNames.size());
  double Prog = 0.;
  IFunction_sptr ifun;
  bool firstInteration = true;
  for (int i = 0; i < static_cast<int>(wsNames.size()); ++i) {
    InputData data = wsNames[i];

    if (!data.ws) {
      g_log.warning() << "Cannot access workspace " << wsNames[i].name << '\n';
      continue;
    }

    if (data.i < 0) {
      g_log.warning() << "Zero spectra selected for fitting in workspace "
                      << wsNames[i].name << '\n';
      continue;
    }

    if (createFitOutput) {
      covarianceWorkspaces.reserve(covarianceWorkspaces.size() + data.i + 1);
      fitWorkspaces.reserve(fitWorkspaces.size() + data.i + 1);
      parameterWorkspaces.reserve(parameterWorkspaces.size() + data.i + 1);
    }

    dProg /= abs(1);
    if (isMultiDomainFunction) {
      ifun = inputFunction->getFunction(i);
      if (!individual && !firstInteration) {
        IFunction_sptr prevFunction = inputFunction->getFunction(i - 1);
        for (size_t k = 0; k < ifun->nParams(); ++k) {
          ifun->setParameter(k, prevFunction->getParameter(k));
        }
      }

    } else {
      ifun = inputFunction;
    }
    firstInteration = false;
    // Find the log value: it is either a log-file value or
    // simply the workspace number
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
        throw std::runtime_error("Failed to cast " + logName +
                                 " to TimeSeriesProperty");
      }
      logValue = logp->lastValue();
    }

    double chi2;

    try {
      if (passWSIndexToFunction) {
        setWorkspaceIndexAttribute(ifun, data.i);
      }

      g_log.debug() << "Fitting " << data.ws->getName() << " index " << data.i
                    << " with \n";
      g_log.debug() << ifun->asString() << '\n';

      const std::string spectrum_index = std::to_string(data.i);
      std::string wsBaseName;

      if (createFitOutput)
        wsBaseName = wsNames[i].name + "_" + spectrum_index;

      bool histogramFit = getPropertyValue("EvaluationType") == "Histogram";
      bool ignoreInvalidData = getProperty("IgnoreInvalidData");

      // Fit the function
      auto fit = this->createChildAlgorithm("Fit");
      fit->initialize();
      fit->setPropertyValue("EvaluationType",
                            getPropertyValue("EvaluationType"));
      fit->setProperty("Function", ifun);
      fit->setProperty("InputWorkspace", data.ws);
      fit->setProperty("WorkspaceIndex", data.i);
      fit->setPropertyValue("StartX", getPropertyValue("StartX"));
      fit->setPropertyValue("EndX", getPropertyValue("EndX"));
      fit->setProperty("IgnoreInvalidData", ignoreInvalidData);
      fit->setPropertyValue(
          "Minimizer", getMinimizerString(wsNames[i].name, spectrum_index));
      fit->setPropertyValue("CostFunction", getPropertyValue("CostFunction"));
      fit->setPropertyValue("MaxIterations", getPropertyValue("MaxIterations"));
      fit->setPropertyValue("PeakRadius", getPropertyValue("PeakRadius"));
      fit->setProperty("CalcErrors", true);
      fit->setProperty("CreateOutput", createFitOutput);
      if (!histogramFit) {
        fit->setProperty("OutputCompositeMembers", outputCompositeMembers);
        fit->setProperty("ConvolveMembers", outputConvolvedMembers);
        fit->setProperty("Exclude", exclude);
      }
      fit->setProperty("Output", wsBaseName);
      fit->execute();

      if (!fit->isExecuted()) {
        throw std::runtime_error("Fit child algorithm failed: " +
                                 data.ws->getName());
      }

      ifun = fit->getProperty("Function");
      chi2 = fit->getProperty("OutputChi2overDoF");

      if (createFitOutput) {
        MatrixWorkspace_sptr outputFitWorkspace =
            fit->getProperty("OutputWorkspace");
        ITableWorkspace_sptr outputParamWorkspace =
            fit->getProperty("OutputParameters");
        ITableWorkspace_sptr outputCovarianceWorkspace =
            fit->getProperty("OutputNormalisedCovarianceMatrix");
        fitWorkspaces.emplace_back(outputFitWorkspace);
        parameterWorkspaces.emplace_back(outputParamWorkspace);
        covarianceWorkspaces.emplace_back(outputCovarianceWorkspace);
      }
      g_log.debug() << "Fit result " << fit->getPropertyValue("OutputStatus")
                    << ' ' << chi2 << '\n';
    } catch (...) {
      g_log.error("Error in Fit ChildAlgorithm");
      throw;
    }

    // Extract the fitted parameters and put them into the result table
    TableRow row = result->appendRow();
    if (isDataName) {
      row << wsNames[i].name;
    } else {
      row << logValue;
    }

    for (size_t iPar = 0; iPar < ifun->nParams(); ++iPar) {
      row << ifun->getParameter(iPar) << ifun->getError(iPar);
    }
    row << chi2;

    Prog += dProg;
    std::string current = std::to_string(i);
    progress(Prog, ("Fitting Workspace: (" + current + ") - "));
    interruption_point();

    if (individual) {
      for (size_t k = 0; k < initialParams.size(); ++k) {
        ifun->setParameter(k, initialParams[k]);
      }
    }
  }

  if (createFitOutput) {
    // collect output of fit for each spectrum into workspace groups
    WorkspaceGroup_sptr covarianceGroup = boost::make_shared<WorkspaceGroup>();
    for (auto const &workspace : covarianceWorkspaces)
      covarianceGroup->addWorkspace(workspace);
    AnalysisDataService::Instance().addOrReplace(
        m_baseName + "_NormalisedCovarianceMatrices", covarianceGroup);

    WorkspaceGroup_sptr parameterGroup = boost::make_shared<WorkspaceGroup>();
    for (auto const &workspace : parameterWorkspaces)
      parameterGroup->addWorkspace(workspace);
    AnalysisDataService::Instance().addOrReplace(m_baseName + "_Parameters",
                                                 parameterGroup);

    WorkspaceGroup_sptr fitGroup = boost::make_shared<WorkspaceGroup>();
    for (auto const &workspace : fitWorkspaces)
      fitGroup->addWorkspace(workspace);
    AnalysisDataService::Instance().addOrReplace(m_baseName + "_Workspaces",
                                                 fitGroup);
  }

  for (auto &minimizerWorkspace : m_minimizerWorkspaces) {
    const std::string paramName = minimizerWorkspace.first;
    auto groupAlg = this->createChildAlgorithm("GroupWorkspaces");
    groupAlg->initialize();
    groupAlg->setProperty("InputWorkspaces", minimizerWorkspace.second);
    groupAlg->setProperty("OutputWorkspace", m_baseName + "_" + paramName);
    groupAlg->execute();
  }
}

void PlotPeakByLogValue::setWorkspaceIndexAttribute(IFunction_sptr fun,
                                                    int wsIndex) const {
  const std::string attName = "WorkspaceIndex";
  if (fun->hasAttribute(attName)) {
    fun->setAttributeValue(attName, wsIndex);
  }

  API::CompositeFunction_sptr cf =
      boost::dynamic_pointer_cast<API::CompositeFunction>(fun);
  if (cf) {
    for (size_t i = 0; i < cf->nFunctions(); ++i) {
      setWorkspaceIndexAttribute(cf->getFunction(i), wsIndex);
    }
  }
}

std::string PlotPeakByLogValue::getMinimizerString(const std::string &wsName,
                                                   const std::string &wsIndex) {
  std::string format = getPropertyValue("Minimizer");
  std::string wsBaseName = wsName + "_" + wsIndex;
  boost::replace_all(format, "$wsname", wsName);
  boost::replace_all(format, "$wsindex", wsIndex);
  boost::replace_all(format, "$basename", wsBaseName);
  boost::replace_all(format, "$outputname", m_baseName);

  auto minimizer = FuncMinimizerFactory::Instance().createMinimizer(format);
  auto minimizerProps = minimizer->getProperties();
  for (auto &minimizerProp : minimizerProps) {
    auto *wsProp =
        dynamic_cast<Mantid::API::WorkspaceProperty<> *>(minimizerProp);
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

} // namespace Algorithms
} // namespace CurveFitting
} // namespace Mantid
