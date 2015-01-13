//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <cmath>
#include <vector>
#include <iostream>
#include <iomanip>
#include <sstream>

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/ScopedWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAlgorithms/PlotAsymmetryByLogValue.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>

namespace // anonymous
    {

/**
 * Convert a log property to a double value.
 *
 * @param property :: Pointer to a TimeSeriesProperty.
 * @param value :: Returned double value.
 * @return :: True if successful
 */
template <typename T>
bool convertLogToDouble(const Mantid::Kernel::Property *property,
                        double &value) {
  const Mantid::Kernel::TimeSeriesProperty<T> *log =
      dynamic_cast<const Mantid::Kernel::TimeSeriesProperty<T> *>(property);
  if (log) {
    value = static_cast<double>(log->lastValue());
    return true;
  }
  auto tlog =
      dynamic_cast<const Mantid::Kernel::PropertyWithValue<T> *>(property);
  if (tlog) {
    value = static_cast<double>(*tlog);
    return true;
  }
  return false;
}

} // anonymous

namespace Mantid {
namespace Algorithms {

using namespace Kernel;
using namespace API;
using namespace DataObjects;

// Register the class into the algorithm factory
DECLARE_ALGORITHM(PlotAsymmetryByLogValue)

/** Initialisation method. Declares properties to be used in algorithm.
*
*/
void PlotAsymmetryByLogValue::init() {
  std::string nexusExt(".nxs");

  declareProperty(
      new FileProperty("FirstRun", "", FileProperty::Load, nexusExt),
      "The name of the first workspace in the series.");
  declareProperty(new FileProperty("LastRun", "", FileProperty::Load, nexusExt),
                  "The name of the last workspace in the series.");
  declareProperty(
      new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
      "The name of the output workspace containing the resulting asymmetries.");
  declareProperty("LogValue", "",
                  boost::make_shared<MandatoryValidator<std::string>>(),
                  "The name of the log values which will be used as the x-axis "
                  "in the output workspace.");
  declareProperty("Red", 1, "The period number for the 'red' data.");
  declareProperty("Green", EMPTY_INT(),
                  "The period number for the 'green' data.");

  std::vector<std::string> options;
  options.push_back("Integral");
  options.push_back("Differential");
  declareProperty("Type", "Integral",
                  boost::make_shared<StringListValidator>(options),
                  "The calculation type: 'Integral' or 'Differential'.");

  declareProperty(
      "TimeMin", EMPTY_DBL(),
      "The beginning of the time interval used in the calculations.");
  declareProperty("TimeMax", EMPTY_DBL(),
                  "The end of the time interval used in the calculations.");

  declareProperty(new ArrayProperty<int>("ForwardSpectra"),
                  "The list of spectra for the forward group. If not specified "
                  "the following happens. The data will be grouped according "
                  "to grouping information in the data, if available. The "
                  "forward will use the first of these groups.");
  declareProperty(new ArrayProperty<int>("BackwardSpectra"),
                  "The list of spectra for the backward group. If not "
                  "specified the following happens. The data will be grouped "
                  "according to grouping information in the data, if "
                  "available. The backward will use the second of these "
                  "groups.");

  std::vector<std::string> deadTimeCorrTypes;
  deadTimeCorrTypes.push_back("None");
  deadTimeCorrTypes.push_back("FromRunData");
  deadTimeCorrTypes.push_back("FromSpecifiedFile");

  declareProperty("DeadTimeCorrType", deadTimeCorrTypes[0],
                  boost::make_shared<StringListValidator>(deadTimeCorrTypes),
                  "Type of Dead Time Correction to apply.");

  declareProperty(new FileProperty("DeadTimeCorrFile", "",
                                   FileProperty::OptionalLoad, nexusExt),
                  "Custom file with Dead Times. Will be used only if "
                  "appropriate DeadTimeCorrType is set.");
}

/**
*   Executes the algorithm
*/
void PlotAsymmetryByLogValue::exec() {
  m_forward_list = getProperty("ForwardSpectra");
  m_backward_list = getProperty("BackwardSpectra");
  m_autogroup = (m_forward_list.size() == 0 && m_backward_list.size() == 0);

  std::string logName = getProperty("LogValue");

  int red = getProperty("Red");
  int green = getProperty("Green");

  std::string stype = getProperty("Type");
  m_int = stype == "Integral";

  std::string firstFN = getProperty("FirstRun");
  std::string lastFN = getProperty("LastRun");

  std::string ext = firstFN.substr(firstFN.find_last_of("."));

  firstFN.erase(firstFN.size() - 4);
  lastFN.erase(lastFN.size() - 4);

  std::string fnBase = firstFN;
  size_t i = fnBase.size() - 1;
  while (isdigit(fnBase[i]))
    i--;
  if (i == fnBase.size() - 1) {
    g_log.error("File name must end with a number.");
    throw Exception::FileError("File name must end with a number.", firstFN);
  }
  fnBase.erase(i + 1);

  firstFN.erase(0, fnBase.size());
  lastFN.erase(0, fnBase.size());

  size_t is = atoi(firstFN.c_str()); // starting run number
  size_t ie = atoi(lastFN.c_str());  // last run number
  int w = static_cast<int>(firstFN.size());

  // The number of runs
  size_t npoints = ie - is + 1;

  // Create the 2D workspace for the output
  int nplots = green != EMPTY_INT() ? 4 : 1;
  MatrixWorkspace_sptr outWS = WorkspaceFactory::Instance().create(
      "Workspace2D",
      nplots,  //  the number of plots
      npoints, //  the number of data points on a plot
      npoints  //  it's not a histogram
      );
  TextAxis *tAxis = new TextAxis(nplots);
  if (nplots == 1) {
    tAxis->setLabel(0, "Asymmetry");
  } else {
    tAxis->setLabel(0, "Red-Green");
    tAxis->setLabel(1, "Red");
    tAxis->setLabel(2, "Green");
    tAxis->setLabel(3, "Red+Green");
  }
  outWS->replaceAxis(1, tAxis);

  const std::string dtcType = getPropertyValue("DeadTimeCorrType");

  Workspace_sptr customDeadTimes;

  if (dtcType == "FromSpecifiedFile") {
    IAlgorithm_sptr loadDeadTimes = createChildAlgorithm("LoadNexusProcessed");
    loadDeadTimes->initialize();
    loadDeadTimes->setPropertyValue("Filename",
                                    getPropertyValue("DeadTimeCorrFile"));
    loadDeadTimes->execute();

    customDeadTimes = loadDeadTimes->getProperty("OutputWorkspace");
  }

  Progress progress(this, 0, 1, ie - is + 2);
  for (size_t i = is; i <= ie; i++) {
    std::ostringstream fn, fnn;
    fnn << std::setw(w) << std::setfill('0') << i;
    fn << fnBase << fnn.str() << ext;

    IAlgorithm_sptr load = createChildAlgorithm("LoadMuonNexus");
    load->initialize();
    load->setPropertyValue("Filename", fn.str());
    load->execute();

    Workspace_sptr loadedWs = load->getProperty("OutputWorkspace");

    if (dtcType != "None") {
      IAlgorithm_sptr applyCorr =
          AlgorithmManager::Instance().create("ApplyDeadTimeCorr");
      applyCorr->setLogging(false);
      applyCorr->setRethrows(true);

      ScopedWorkspace ws(loadedWs);
      applyCorr->setPropertyValue("InputWorkspace", ws.name());
      applyCorr->setPropertyValue("OutputWorkspace", ws.name());

      ScopedWorkspace deadTimes;

      if (dtcType == "FromSpecifiedFile") {
        deadTimes.set(customDeadTimes);
      } else {
        deadTimes.set(load->getProperty("DeadTimeTable"));
      }

      applyCorr->setPropertyValue("DeadTimeTable", deadTimes.name());
      applyCorr->execute();

      // Workspace should've been replaced in the ADS by ApplyDeadTimeCorr, so
      // need to
      // re-assign it
      loadedWs = ws.retrieve();
    }

    if (m_autogroup) {
      Workspace_sptr loadedDetGrouping =
          load->getProperty("DetectorGroupingTable");

      if (!loadedDetGrouping)
        throw std::runtime_error("No grouping info in the file.\n\nPlease "
                                 "specify grouping manually");

      // Could be groups of workspaces, so need to work with ADS
      ScopedWorkspace inWS(loadedWs);
      ScopedWorkspace grouping(loadedDetGrouping);
      ScopedWorkspace outWS;

      try {
        IAlgorithm_sptr applyGrouping =
            AlgorithmManager::Instance().create("MuonGroupDetectors");
        applyGrouping->setLogging(false);
        applyGrouping->setRethrows(true);

        applyGrouping->setPropertyValue("InputWorkspace", inWS.name());
        applyGrouping->setPropertyValue("DetectorGroupingTable",
                                        grouping.name());
        applyGrouping->setPropertyValue("OutputWorkspace", outWS.name());
        applyGrouping->execute();

        loadedWs = outWS.retrieve();
      } catch (...) {
        throw std::runtime_error(
            "Unable to group detectors.\n\nPlease specify grouping manually.");
      }
    }

    WorkspaceGroup_sptr loadedGroup =
        boost::dynamic_pointer_cast<WorkspaceGroup>(loadedWs);

    if (!loadedGroup) {
      Workspace2D_sptr loadedWs2D =
          boost::dynamic_pointer_cast<Workspace2D>(loadedWs);

      double Y, E;
      calcIntAsymmetry(loadedWs2D, Y, E);
      outWS->dataY(0)[i - is] = Y;
      outWS->dataX(0)[i - is] = getLogValue(*loadedWs2D, logName);
      outWS->dataE(0)[i - is] = E;
    } else {
      DataObjects::Workspace2D_sptr ws_red;
      DataObjects::Workspace2D_sptr ws_green;

      // Run through the periods of the loaded file and do calculations on the
      // selected ones
      for (int mi = 0; mi < loadedGroup->getNumberOfEntries(); mi++) {
        Workspace2D_sptr memberWs =
            boost::dynamic_pointer_cast<Workspace2D>(loadedGroup->getItem(mi));

        int period = mi + 1;

        // Do only one period
        if (green == EMPTY_INT() && period == red) {
          ws_red = memberWs;
          double Y, E;
          calcIntAsymmetry(ws_red, Y, E);
          outWS->dataY(0)[i - is] = Y;
          outWS->dataX(0)[i - is] = getLogValue(*ws_red, logName);
          outWS->dataE(0)[i - is] = E;
        } else // red & green
        {
          if (period == red)
            ws_red = memberWs;
          if (period == green)
            ws_green = memberWs;
        }
      }

      // red & green claculation
      if (green != EMPTY_INT()) {
        if (!ws_red || !ws_green)
          throw std::invalid_argument("Red or green period is out of range");
        double Y, E;
        double Y1, E1;
        double logValue = getLogValue(*ws_red, logName);
        calcIntAsymmetry(ws_red, Y, E);
        calcIntAsymmetry(ws_green, Y1, E1);
        outWS->dataY(1)[i - is] = Y;
        outWS->dataX(1)[i - is] = logValue;
        outWS->dataE(1)[i - is] = E;

        outWS->dataY(2)[i - is] = Y1;
        outWS->dataX(2)[i - is] = logValue;
        outWS->dataE(2)[i - is] = E1;

        outWS->dataY(3)[i - is] = Y + Y1;
        outWS->dataX(3)[i - is] = logValue;
        outWS->dataE(3)[i - is] = sqrt(E * E + E1 * E1);

        // move to last for safety since some grouping takes place in the
        // calcIntAsymmetry call below
        calcIntAsymmetry(ws_red, ws_green, Y, E);
        outWS->dataY(0)[i - is] = Y;
        outWS->dataX(0)[i - is] = logValue;
        outWS->dataE(0)[i - is] = E;
      } else if (!ws_red)
        throw std::invalid_argument("Red period is out of range");
    }
    progress.report();
  }

  outWS->getAxis(0)->title() = logName;
  outWS->setYUnitLabel("Asymmetry");

  // Assign the result to the output workspace property
  setProperty("OutputWorkspace", outWS);
}

/**  Calculate the integral asymmetry for a workspace.
*   The calculation is done by MuonAsymmetryCalc and SimpleIntegration
* algorithms.
*   @param ws :: The workspace
*   @param Y :: Reference to a variable receiving the value of asymmetry
*   @param E :: Reference to a variable receiving the value of the error
*/
void PlotAsymmetryByLogValue::calcIntAsymmetry(API::MatrixWorkspace_sptr ws,
                                               double &Y, double &E) {
  Property *startXprop = getProperty("TimeMin");
  Property *endXprop = getProperty("TimeMax");
  bool setX = !startXprop->isDefault() && !endXprop->isDefault();
  double startX(0.0), endX(0.0);
  if (setX) {
    startX = getProperty("TimeMin");
    endX = getProperty("TimeMax");
  }
  if (!m_int) { //  "Differential asymmetry"
    IAlgorithm_sptr asym = createChildAlgorithm("AsymmetryCalc");
    asym->initialize();
    asym->setProperty("InputWorkspace", ws);
    asym->setPropertyValue("OutputWorkspace", "tmp");
    if (!m_autogroup) {
      asym->setProperty("ForwardSpectra", m_forward_list);
      asym->setProperty("BackwardSpectra", m_backward_list);
    }
    asym->execute();
    MatrixWorkspace_sptr asymWS = asym->getProperty("OutputWorkspace");

    IAlgorithm_sptr integr = createChildAlgorithm("Integration");
    integr->setProperty("InputWorkspace", asymWS);
    integr->setPropertyValue("OutputWorkspace", "tmp");
    if (setX) {
      integr->setProperty("RangeLower", startX);
      integr->setProperty("RangeUpper", endX);
    }
    integr->execute();
    API::MatrixWorkspace_sptr out = integr->getProperty("OutputWorkspace");

    Y = out->readY(0)[0];
    E = out->readE(0)[0];
  } else {
    //  "Integral asymmetry"
    IAlgorithm_sptr integr = createChildAlgorithm("Integration");
    integr->setProperty("InputWorkspace", ws);
    integr->setPropertyValue("OutputWorkspace", "tmp");
    if (setX) {
      integr->setProperty("RangeLower", startX);
      integr->setProperty("RangeUpper", endX);
    }
    integr->execute();
    API::MatrixWorkspace_sptr intWS = integr->getProperty("OutputWorkspace");

    IAlgorithm_sptr asym = createChildAlgorithm("AsymmetryCalc");
    asym->initialize();
    asym->setProperty("InputWorkspace", intWS);
    asym->setPropertyValue("OutputWorkspace", "tmp");
    if (!m_autogroup) {
      asym->setProperty("ForwardSpectra", m_forward_list);
      asym->setProperty("BackwardSpectra", m_backward_list);
    }
    asym->execute();
    MatrixWorkspace_sptr out = asym->getProperty("OutputWorkspace");

    Y = out->readY(0)[0];
    E = out->readE(0)[0];
  }
}

/**  Calculate the integral asymmetry for a workspace (red & green).
*   The calculation is done by MuonAsymmetryCalc and SimpleIntegration
* algorithms.
*   @param ws_red :: The red workspace
*   @param ws_green :: The green workspace
*   @param Y :: Reference to a variable receiving the value of asymmetry
*   @param E :: Reference to a variable receiving the value of the error
*/
void
PlotAsymmetryByLogValue::calcIntAsymmetry(API::MatrixWorkspace_sptr ws_red,
                                          API::MatrixWorkspace_sptr ws_green,
                                          double &Y, double &E) {
  if (!m_autogroup) {
    groupDetectors(ws_red, m_backward_list);
    groupDetectors(ws_red, m_forward_list);
    groupDetectors(ws_green, m_backward_list);
    groupDetectors(ws_green, m_forward_list);
  }

  Property *startXprop = getProperty("TimeMin");
  Property *endXprop = getProperty("TimeMax");
  bool setX = !startXprop->isDefault() && !endXprop->isDefault();
  double startX(0.0), endX(0.0);
  if (setX) {
    startX = getProperty("TimeMin");
    endX = getProperty("TimeMax");
  }
  if (!m_int) { //  "Differential asymmetry"

    API::MatrixWorkspace_sptr tmpWS = API::WorkspaceFactory::Instance().create(
        ws_red, 1, ws_red->readX(0).size(), ws_red->readY(0).size());

    for (size_t i = 0; i < tmpWS->dataY(0).size(); i++) {
      double FNORM = ws_green->readY(0)[i] + ws_red->readY(0)[i];
      FNORM = FNORM != 0.0 ? 1.0 / FNORM : 1.0;
      double BNORM = ws_green->readY(1)[i] + ws_red->readY(1)[i];
      BNORM = BNORM != 0.0 ? 1.0 / BNORM : 1.0;
      double ZF = (ws_green->readY(0)[i] - ws_red->readY(0)[i]) * FNORM;
      double ZB = (ws_green->readY(1)[i] - ws_red->readY(1)[i]) * BNORM;
      tmpWS->dataY(0)[i] = ZB - ZF;
      tmpWS->dataE(0)[i] = (1.0 + ZF * ZF) * FNORM + (1.0 + ZB * ZB) * BNORM;
    }

    IAlgorithm_sptr integr = createChildAlgorithm("Integration");
    integr->setProperty("InputWorkspace", tmpWS);
    integr->setPropertyValue("OutputWorkspace", "tmp");
    if (setX) {
      integr->setProperty("RangeLower", startX);
      integr->setProperty("RangeUpper", endX);
    }
    integr->execute();
    MatrixWorkspace_sptr out = integr->getProperty("OutputWorkspace");

    Y = out->readY(0)[0] / static_cast<double>(tmpWS->dataY(0).size());
    E = out->readE(0)[0] / static_cast<double>(tmpWS->dataY(0).size());
  } else {
    //  "Integral asymmetry"
    IAlgorithm_sptr integr = createChildAlgorithm("Integration");
    integr->setProperty("InputWorkspace", ws_red);
    integr->setPropertyValue("OutputWorkspace", "tmp");
    if (setX) {
      integr->setProperty("RangeLower", startX);
      integr->setProperty("RangeUpper", endX);
    }
    integr->execute();
    API::MatrixWorkspace_sptr intWS_red =
        integr->getProperty("OutputWorkspace");

    integr = createChildAlgorithm("Integration");
    integr->setProperty("InputWorkspace", ws_green);
    integr->setPropertyValue("OutputWorkspace", "tmp");
    if (setX) {
      integr->setProperty("RangeLower", startX);
      integr->setProperty("RangeUpper", endX);
    }
    integr->execute();
    API::MatrixWorkspace_sptr intWS_green =
        integr->getProperty("OutputWorkspace");

    double YIF = (intWS_green->readY(0)[0] - intWS_red->readY(0)[0]) /
                 (intWS_green->readY(0)[0] + intWS_red->readY(0)[0]);
    double YIB = (intWS_green->readY(1)[0] - intWS_red->readY(1)[0]) /
                 (intWS_green->readY(1)[0] + intWS_red->readY(1)[0]);

    Y = YIB - YIF;

    double VARIF =
        (1.0 + YIF * YIF) / (intWS_green->readY(0)[0] + intWS_red->readY(0)[0]);
    double VARIB =
        (1.0 + YIB * YIB) / (intWS_green->readY(1)[0] + intWS_red->readY(1)[0]);

    E = sqrt(VARIF + VARIB);
  }
}

/**  Group detectors in the workspace.
 *  @param ws :: A local workspace
 *  @param spectraList :: A list of spectra to group.
 */
void
PlotAsymmetryByLogValue::groupDetectors(API::MatrixWorkspace_sptr &ws,
                                        const std::vector<int> &spectraList) {
  API::IAlgorithm_sptr group = createChildAlgorithm("GroupDetectors");
  group->setProperty("InputWorkspace", ws);
  group->setProperty("SpectraList", spectraList);
  group->setProperty("KeepUngroupedSpectra", true);
  group->execute();
  ws = group->getProperty("OutputWorkspace");
}

/**
 * Get log value from a workspace. Convert to double if possible.
 *
 * @param ws :: The input workspace.
 * @param logName :: Name of the log file.
 * @return :: Log value.
 * @throw :: std::invalid_argument if the log cannot be converted to a double or
 *doesn't exist.
 */
double PlotAsymmetryByLogValue::getLogValue(MatrixWorkspace &ws,
                                            const std::string &logName) {
  auto *property = ws.run().getLogData(logName);
  if (!property) {
    throw std::invalid_argument("Log " + logName + " does not exist.");
  }

  double value = 0;
  // try different property types
  if (convertLogToDouble<double>(property, value))
    return value;
  if (convertLogToDouble<float>(property, value))
    return value;
  if (convertLogToDouble<int>(property, value))
    return value;
  if (convertLogToDouble<long>(property, value))
    return value;
  if (convertLogToDouble<long long>(property, value))
    return value;
  if (convertLogToDouble<unsigned int>(property, value))
    return value;
  if (convertLogToDouble<unsigned long>(property, value))
    return value;
  if (convertLogToDouble<unsigned long long>(property, value))
    return value;
  // try if it's a string and can be lexically cast to double
  auto slog =
      dynamic_cast<const Mantid::Kernel::PropertyWithValue<std::string> *>(
          property);
  if (slog) {
    try {
      value = boost::lexical_cast<double>(slog->value());
      return value;
    } catch (std::exception &) {
      // do nothing, goto throw
    }
  }

  throw std::invalid_argument("Log " + logName +
                              " cannot be converted to a double type.");
}
} // namespace Algorithm
} // namespace Mantid
