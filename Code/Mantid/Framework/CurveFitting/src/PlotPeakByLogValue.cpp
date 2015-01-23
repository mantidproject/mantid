//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <cmath>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <Poco/StringTokenizer.h>
#include <boost/lexical_cast.hpp>

#include "MantidCurveFitting/PlotPeakByLogValue.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FuncMinimizerFactory.h"
#include "MantidAPI/CostFunctionFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"

namespace Mantid {
namespace CurveFitting {

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
  declareProperty(new WorkspaceProperty<ITableWorkspace>("OutputWorkspace", "",
                                                         Direction::Output),
                  "The output TableWorkspace");
  declareProperty("Function", "",
                  boost::make_shared<MandatoryValidator<std::string>>(),
                  "The fitting function, common for all workspaces in the "
                  "input WorkspaceGroup");
  declareProperty("LogValue", "", "Name of the log value to plot the "
                                  "parameters against. Default: use spectra "
                                  "numbers.");
  declareProperty("StartX", EMPTY_DBL(), "A value of x in, or on the low x "
                                         "boundary of, the first bin to "
                                         "include in\n"
                                         "the fit (default lowest value of x)");
  declareProperty("EndX", EMPTY_DBL(), "A value in, or on the high x boundary "
                                       "of, the last bin the fitting range\n"
                                       "(default the highest value of x)");

  std::vector<std::string> fitOptions;
  fitOptions.push_back("Sequential");
  fitOptions.push_back("Individual");
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

  std::vector<std::string> minimizerOptions =
      FuncMinimizerFactory::Instance().getKeys();
  declareProperty("Minimizer", "Levenberg-Marquardt",
                  boost::make_shared<StringListValidator>(minimizerOptions),
                  "Minimizer to use for fitting. Minimizers available are "
                  "'Levenberg-Marquardt', 'Simplex', \n"
                  "'Conjugate gradient (Fletcher-Reeves imp.)', 'Conjugate "
                  "gradient (Polak-Ribiere imp.)' and 'BFGS'",
                  Direction::InOut);

  std::vector<std::string> costFuncOptions =
      CostFunctionFactory::Instance().getKeys();
  declareProperty("CostFunction", "Least squares",
                  boost::make_shared<StringListValidator>(costFuncOptions),
                  "Cost functions to use for fitting. Cost functions available "
                  "are 'Least squares' and 'Ignore positive peaks'",
                  Direction::InOut);

  declareProperty("CreateOutput", false, "Set to true to create output "
                                         "workspaces with the results of the "
                                         "fit(default is false).");

  declareProperty("OutputCompositeMembers", false,
                  "If true and CreateOutput is true then the value of each "
                  "member of a Composite Function is also output.");

  declareProperty(new Kernel::PropertyWithValue<bool>("ConvolveMembers", false),
                  "If true and OutputCompositeMembers is true members of any "
                  "Convolution are output convolved\n"
                  "with corresponding resolution");
}

/**
*   Executes the algorithm
*/
void PlotPeakByLogValue::exec() {

  // Create a list of the input workspace
  const std::vector<InputData> wsNames = makeNames();

  std::string fun = getPropertyValue("Function");
  // int wi = getProperty("WorkspaceIndex");
  std::string logName = getProperty("LogValue");
  bool individual = getPropertyValue("FitType") == "Individual";
  bool passWSIndexToFunction = getProperty("PassWSIndexToFunction");
  bool createFitOutput = getProperty("CreateOutput");
  bool outputCompositeMembers = getProperty("OutputCompositeMembers");
  bool outputConvolvedMembers = getProperty("ConvolveMembers");
  std::string baseName = getPropertyValue("OutputWorkspace");

  bool isDataName = false; // if true first output column is of type string and
                           // is the data source name
  ITableWorkspace_sptr result =
      WorkspaceFactory::Instance().createTable("TableWorkspace");
  if (logName == "SourceName") {
    result->addColumn("str", "Source name");
    isDataName = true;
  } else if (logName.empty()) {
    result->addColumn("double", "axis-1");
  } else {
    result->addColumn("double", logName);
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

  std::vector<std::string> covariance_workspaces;
  std::vector<std::string> fit_workspaces;
  std::vector<std::string> parameter_workspaces;

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
      covariance_workspaces.reserve(covariance_workspaces.size() + jend);
      fit_workspaces.reserve(fit_workspaces.size() + jend);
      parameter_workspaces.reserve(parameter_workspaces.size() + jend);
    }

    dProg /= abs(jend - j);
    for (; j < jend; ++j) {

      // Find the log value: it is either a log-file value or simply the
      // workspace number
      double logValue = 0;
      if (logName.empty()) {
        API::Axis *axis = data.ws->getAxis(1);
        logValue = (*axis)(j);
      } else if (logName != "SourceName") {
        Kernel::Property *prop = data.ws->run().getLogData(logName);
        if (!prop) {
          throw std::invalid_argument("Log value " + logName +
                                      " does not exist");
        }
        TimeSeriesProperty<double> *logp =
            dynamic_cast<TimeSeriesProperty<double> *>(prop);
        logValue = logp->lastValue();
      }

      double chi2;

      try {
        if (passWSIndexToFunction) {
          setWorkspaceIndexAttribute(ifun, j);
        }

        g_log.debug() << "Fitting " << data.ws->name() << " index " << j
                      << " with " << std::endl;
        g_log.debug() << ifun->asString() << std::endl;

        std::string spectrum_index = boost::lexical_cast<std::string>(j);
        std::string wsBaseName = "";

        if (createFitOutput)
          wsBaseName = wsNames[i].name + "_" + spectrum_index;

        // Fit the function
        API::IAlgorithm_sptr fit =
            AlgorithmManager::Instance().createUnmanaged("Fit");
        fit->initialize();
        fit->setProperty("Function", ifun);
        fit->setProperty("InputWorkspace", data.ws);
        fit->setProperty("WorkspaceIndex", j);
        fit->setPropertyValue("StartX", getPropertyValue("StartX"));
        fit->setPropertyValue("EndX", getPropertyValue("EndX"));
        fit->setPropertyValue("Minimizer", getPropertyValue("Minimizer"));
        fit->setPropertyValue("CostFunction", getPropertyValue("CostFunction"));
        fit->setProperty("CalcErrors", true);
        fit->setProperty("CreateOutput", createFitOutput);
        fit->setProperty("OutputCompositeMembers", outputCompositeMembers);
        fit->setProperty("ConvolveMembers", outputConvolvedMembers);
        fit->setProperty("Output", wsBaseName);
        fit->execute();

        if (!fit->isExecuted()) {
          throw std::runtime_error("Fit child algorithm failed: " +
                                   data.ws->name());
        }

        ifun = fit->getProperty("Function");
        chi2 = fit->getProperty("OutputChi2overDoF");

        if (createFitOutput) {
          covariance_workspaces.push_back(wsBaseName +
                                          "_NormalisedCovarianceMatrix");
          parameter_workspaces.push_back(wsBaseName + "_Parameters");
          fit_workspaces.push_back(wsBaseName + "_Workspace");
        }

        g_log.debug() << "Fit result " << fit->getPropertyValue("OutputStatus")
                      << ' ' << chi2 << std::endl;

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
      progress(Prog);
      interruption_point();

      if (individual) {
        for (size_t i = 0; i < initialParams.size(); ++i) {
          ifun->setParameter(i, initialParams[i]);
        }
      }

    } // for(;j < jend;++j)
  }

  if (createFitOutput) {
    // collect output of fit for each spectrum into workspace groups
    API::IAlgorithm_sptr groupAlg =
        AlgorithmManager::Instance().createUnmanaged("GroupWorkspaces");
    groupAlg->initialize();
    groupAlg->setProperty("InputWorkspaces", covariance_workspaces);
    groupAlg->setProperty("OutputWorkspace",
                          baseName + "_NormalisedCovarianceMatrices");
    groupAlg->execute();

    groupAlg = AlgorithmManager::Instance().createUnmanaged("GroupWorkspaces");
    groupAlg->initialize();
    groupAlg->setProperty("InputWorkspaces", parameter_workspaces);
    groupAlg->setProperty("OutputWorkspace", baseName + "_Parameters");
    groupAlg->execute();

    groupAlg = AlgorithmManager::Instance().createUnmanaged("GroupWorkspaces");
    groupAlg->initialize();
    groupAlg->setProperty("InputWorkspaces", fit_workspaces);
    groupAlg->setProperty("OutputWorkspace", baseName + "_Workspaces");
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
                  "OUTPUTWORKSPACE_" +
                  boost::lexical_cast<std::string>(data.period);
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
          double s = double(axis->spectraNo(i));
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

  typedef Poco::StringTokenizer tokenizer;
  tokenizer names(inputList, ";",
                  tokenizer::TOK_IGNORE_EMPTY | tokenizer::TOK_TRIM);
  for (tokenizer::Iterator it = names.begin(); it != names.end(); ++it) {
    tokenizer params(*it, ",", tokenizer::TOK_TRIM);
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
      } else if (index.size() > 0 && index[0] == 'v') {
        if (index.size() > 1) { // there is some text after 'v'
          tokenizer range(index.substr(1), ":",
                          tokenizer::TOK_IGNORE_EMPTY | tokenizer::TOK_TRIM);
          if (range.count() < 1) {
            wi = -2; // means use the whole range
          } else if (range.count() == 1) {
            start = boost::lexical_cast<double>(range[0]);
            end = start;
            wi = -1;
            spec = -1;
          } else if (range.count() > 1) {
            start = boost::lexical_cast<double>(range[0]);
            end = boost::lexical_cast<double>(range[1]);
            if (start > end)
              std::swap(start, end);
            wi = -1;
            spec = -1;
          }
        } else {
          wi = -2;
        }
      } else { // error
        // throw std::invalid_argument("Malformed spectrum identifier
        // ("+index+"). "
        //  "It must be either \"sp\" followed by a number for a spectrum number
        //  or"
        //  "\"i\" followed by a number for a workspace index.");
        wi = default_wi;
      }
    }
    int period = (params.count() > 2) ? boost::lexical_cast<int>(params[2]) : 1;
    if (API::AnalysisDataService::Instance().doesExist(name)) {
      API::Workspace_sptr ws =
          API::AnalysisDataService::Instance().retrieve(name);
      API::WorkspaceGroup_sptr wsg =
          boost::dynamic_pointer_cast<API::WorkspaceGroup>(ws);
      if (wsg) {
        std::vector<std::string> wsNames = wsg->getNames();
        for (std::vector<std::string>::iterator i = wsNames.begin();
             i != wsNames.end(); ++i) {
          nameList.push_back(InputData(*i, wi, -1, period, start, end));
        }
        continue;
      }
    }
    nameList.push_back(InputData(name, wi, spec, period, start, end));
  }
  return nameList;
}

} // namespace CurveFitting
} // namespace Mantid
