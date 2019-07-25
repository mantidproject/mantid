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
#include "MantidAPI/IFuncMinimizer.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/ITableWorkspace.h"
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
  declareProperty("Function", "",
                  boost::make_shared<MandatoryValidator<std::string>>(),
                  "The fitting function, common for all workspaces in the "
                  "input WorkspaceGroup");
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
  const std::vector<InputData> wsNames = makeNames();

  const std::vector<double> exclude = getProperty("Exclude");
  std::string fun = getPropertyValue("Function");
  // int wi = getProperty("WorkspaceIndex");
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
  IFunction_sptr ifun = FunctionFactory::Instance().createInitialized(fun);
  if (!ifun) {
    throw std::invalid_argument("Fitting function failed to initialize");
  }

  // for inidividual fittings store the initial parameters
  std::vector<double> initialParams(ifun->nParams());
  if (individual) {
    for (size_t i = 0; i < initialParams.size(); ++i) {
      initialParams[i] = ifun->getParameter(i);
    }
  }

  for (size_t iPar = 0; iPar < ifun->nParams(); ++iPar) {
    result->addColumn("double", ifun->parameterName(iPar));
    result->addColumn("double", ifun->parameterName(iPar) + "_Err");
  }
  result->addColumn("double", "Chi_squared");

  setProperty("OutputWorkspace", result);

  std::vector<MatrixWorkspace_sptr> fitWorkspaces;
  std::vector<ITableWorkspace_sptr> parameterWorkspaces;
  std::vector<ITableWorkspace_sptr> covarianceWorkspaces;

  double dProg = 1. / static_cast<double>(wsNames.size());
  double Prog = 0.;
  for (int i = 0; i < static_cast<int>(wsNames.size()); ++i) {
    InputData data = getWorkspace(wsNames[i]);

    if (!data.ws) {
      g_log.warning() << "Cannot access workspace " << wsNames[i].name << '\n';
      continue;
    }

    if (data.i < 0 && data.indx.empty()) {
      g_log.warning() << "Zero spectra selected for fitting in workspace "
                      << wsNames[i].name << '\n';
      continue;
    }

    int j, jend;
    if (data.i >= 0) {
      j = data.i;
      jend = j + 1;
    } else { // no need to check data.indx.empty()
      j = data.indx.front();
      jend = data.indx.back() + 1;
    }

    if (createFitOutput) {
      covarianceWorkspaces.reserve(covarianceWorkspaces.size() + jend);
      fitWorkspaces.reserve(fitWorkspaces.size() + jend);
      parameterWorkspaces.reserve(parameterWorkspaces.size() + jend);
    }

    dProg /= abs(jend - j);
    for (; j < jend; ++j) {

      // Find the log value: it is either a log-file value or simply the
      // workspace number
      double logValue = 0;
      if (logName.empty() || logName == "axis-1") {
        API::Axis *axis = data.ws->getAxis(1);
        if (dynamic_cast<BinEdgeAxis *>(axis)) {
          double lowerEdge((*axis)(j));
          double upperEdge((*axis)(j + 1));
          logValue = lowerEdge + (upperEdge - lowerEdge) / 2;
        } else
          logValue = (*axis)(j);
      } else if (logName != "SourceName") {
        Kernel::Property *prop = data.ws->run().getLogData(logName);
        if (!prop) {
          throw std::invalid_argument("Log value " + logName +
                                      " does not exist");
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
          setWorkspaceIndexAttribute(ifun, j);
        }

        g_log.debug() << "Fitting " << data.ws->getName() << " index " << j
                      << " with \n";
        g_log.debug() << ifun->asString() << '\n';

        const std::string spectrum_index = std::to_string(j);
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
        fit->setProperty("WorkspaceIndex", j);
        fit->setPropertyValue("StartX", getPropertyValue("StartX"));
        fit->setPropertyValue("EndX", getPropertyValue("EndX"));
        fit->setProperty("IgnoreInvalidData", ignoreInvalidData);
        fit->setPropertyValue(
            "Minimizer", getMinimizerString(wsNames[i].name, spectrum_index));
        fit->setPropertyValue("CostFunction", getPropertyValue("CostFunction"));
        fit->setPropertyValue("MaxIterations",
                              getPropertyValue("MaxIterations"));
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
    } // for(;j < jend;++j)
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

/** Get a workspace identified by an InputData structure.
 * @param data :: InputData with name and either spec or i fields defined.
 * @return InputData structure with the ws field set if everything was OK.
 */
PlotPeakByLogValue::InputData
PlotPeakByLogValue::getWorkspace(const InputData &data) {
  InputData out(data);
  if (API::AnalysisDataService::Instance().doesExist(data.name)) {
    DataObjects::Workspace2D_sptr ws =
        boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
            API::AnalysisDataService::Instance().retrieve(data.name));
    if (ws) {
      out.ws = ws;
    } else {
      return data;
    }
  } else {
    std::ifstream fil(data.name.c_str());
    if (!fil) {
      g_log.warning() << "File " << data.name << " does not exist\n";
      return data;
    }
    fil.close();
    std::string::size_type i = data.name.find_last_of('.');
    if (i == std::string::npos) {
      g_log.warning() << "Cannot open file " << data.name << "\n";
      return data;
    }
    try {
      API::IAlgorithm_sptr load = createChildAlgorithm("Load");
      load->initialize();
      load->setPropertyValue("FileName", data.name);
      load->execute();
      if (load->isExecuted()) {
        API::Workspace_sptr rws = load->getProperty("OutputWorkspace");
        if (rws) {
          DataObjects::Workspace2D_sptr ws =
              boost::dynamic_pointer_cast<DataObjects::Workspace2D>(rws);
          if (ws) {
            out.ws = ws;
          } else {
            API::WorkspaceGroup_sptr gws =
                boost::dynamic_pointer_cast<API::WorkspaceGroup>(rws);
            if (gws) {
              std::string propName =
                  "OUTPUTWORKSPACE_" + std::to_string(data.period);
              if (load->existsProperty(propName)) {
                Workspace_sptr rws1 = load->getProperty(propName);
                out.ws =
                    boost::dynamic_pointer_cast<DataObjects::Workspace2D>(rws1);
              }
            }
          }
        }
      }
    } catch (std::exception &e) {
      g_log.error(e.what());
      return data;
    }
  }

  if (!out.ws)
    return data;

  API::Axis *axis = out.ws->getAxis(1);
  if (axis->isSpectra()) { // spectra axis
    if (out.spec < 0) {
      if (out.i >= 0) {
        out.spec = axis->spectraNo(out.i);
      } else { // i < 0 && spec < 0 => use start and end
        for (size_t i = 0; i < axis->length(); ++i) {
          auto s = double(axis->spectraNo(i));
          if (s >= out.start && s <= out.end) {
            out.indx.push_back(static_cast<int>(i));
          }
        }
      }
    } else {
      for (size_t i = 0; i < axis->length(); ++i) {
        int j = axis->spectraNo(i);
        if (j == out.spec) {
          out.i = static_cast<int>(i);
          break;
        }
      }
    }
    if (out.i < 0 && out.indx.empty()) {
      return data;
    }
  } else { // numeric axis
    out.spec = -1;
    if (out.i >= 0) {
      out.indx.clear();
    } else {
      if (out.i < -1) {
        out.start = (*axis)(0);
        out.end = (*axis)(axis->length() - 1);
      }
      for (size_t i = 0; i < axis->length(); ++i) {
        double s = (*axis)(i);
        if (s >= out.start && s <= out.end) {
          out.indx.push_back(static_cast<int>(i));
        }
      }
    }
  }

  return out;
}

/**
 * Set any WorkspaceIndex attributes in the fitting function. If the function
 * is composite
 * try all its members.
 * @param fun :: The fitting function
 * @param wsIndex :: Value for WorkspaceIndex attributes to set.
 */
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

/// Create a list of input workspace names
std::vector<PlotPeakByLogValue::InputData>
PlotPeakByLogValue::makeNames() const {
  std::vector<InputData> nameList;
  std::string inputList = getPropertyValue("Input");
  int default_wi = getProperty("WorkspaceIndex");
  int default_spec = getProperty("Spectrum");
  double start = 0;
  double end = 0;

  using tokenizer = Mantid::Kernel::StringTokenizer;
  tokenizer names(inputList, ";",
                  tokenizer::TOK_IGNORE_EMPTY | tokenizer::TOK_TRIM);
  for (const auto &input : names) {
    tokenizer params(input, ",", tokenizer::TOK_TRIM);
    std::string name = params[0];
    int wi = default_wi;
    int spec = default_spec;
    if (params.count() > 1) {
      std::string index =
          params[1]; // spectrum or workspace index with a prefix
      if (index.size() > 2 && index.substr(0, 2) == "sp") { // spectrum number
        spec = boost::lexical_cast<int>(index.substr(2));
        wi = -1;                                        // undefined yet
      } else if (index.size() > 1 && index[0] == 'i') { // workspace index
        wi = boost::lexical_cast<int>(index.substr(1));
        spec = -1; // undefined yet
      } else if (!index.empty() && index[0] == 'v') {
        if (index.size() > 1) { // there is some text after 'v'
          tokenizer range(index.substr(1), ":",
                          tokenizer::TOK_IGNORE_EMPTY | tokenizer::TOK_TRIM);
          if (range.count() < 1) {
            wi = -2; // means use the whole range
          } else if (range.count() == 1) {
            try {
              start = boost::lexical_cast<double>(range[0]);
            } catch (boost::bad_lexical_cast &) {
              throw std::runtime_error(
                  std::string("Provided incorrect range values. Range is "
                              "specfifed by start_value:stop_value, but "
                              "provided ") +
                  range[0]);
            }

            end = start;
            wi = -1;
            spec = -1;
          } else if (range.count() > 1) {
            try {
              start = boost::lexical_cast<double>(range[0]);
              end = boost::lexical_cast<double>(range[1]);
            } catch (boost::bad_lexical_cast &) {
              throw std::runtime_error(
                  std::string("Provided incorrect range values. Range is "
                              "specfifed by start_value:stop_value, but "
                              "provided ") +
                  range[0] + std::string(" and ") + range[1]);
            }
            if (start > end)
              std::swap(start, end);
            wi = -1;
            spec = -1;
          }
        } else {
          wi = -2;
        }
      } else {
        wi = default_wi;
      }
    }
    int period = 1;
    try {
      if (params.count() > 2 && !params[2].empty()) {
        period = boost::lexical_cast<int>(params[2]);
      }
    } catch (boost::bad_lexical_cast &) {
      throw std::runtime_error("Incorrect value for a period: " + params[2]);
    }
    if (API::AnalysisDataService::Instance().doesExist(name)) {
      API::Workspace_sptr ws =
          API::AnalysisDataService::Instance().retrieve(name);
      API::WorkspaceGroup_sptr wsg =
          boost::dynamic_pointer_cast<API::WorkspaceGroup>(ws);
      if (wsg) {
        const std::vector<std::string> wsNames = wsg->getNames();
        for (const auto &wsName : wsNames) {
          nameList.emplace_back(InputData(wsName, wi, -1, period, start, end));
        }
        continue;
      }
    }
    nameList.emplace_back(name, wi, spec, period, start, end);
  }
  return nameList;
}

/**
 * Formats the minimizer string for a given spectrum from a given workspace.
 *
 * @param wsName Name of workspace being fitted
 * @param wsIndex Index of spectrum being fitted
 * @return Formatted minimizer string
 */
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
        m_minimizerWorkspaces[wsPropName].push_back(wsPropValue);
      }
    }
  }

  return format;
}

} // namespace Algorithms
} // namespace CurveFitting
} // namespace Mantid
