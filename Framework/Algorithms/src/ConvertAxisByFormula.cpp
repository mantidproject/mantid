// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/ConvertAxisByFormula.h"
#include "MantidAPI/RefAxis.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidGeometry/muParser_Silent.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/UnitFactory.h"

#include <algorithm>
#include <memory>
#include <sstream>

namespace Mantid::Algorithms {

using namespace Kernel;
using namespace API;

// Register the class into the algorithm factory
DECLARE_ALGORITHM(ConvertAxisByFormula)

const std::string ConvertAxisByFormula::name() const { return ("ConvertAxisByFormula"); }

int ConvertAxisByFormula::version() const { return (1); }

const std::string ConvertAxisByFormula::category() const { return "Transforms\\Axes"; }

/** Initialisation method. Declares properties to be used in algorithm.
 *
 */
void ConvertAxisByFormula::init() {
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("InputWorkspace", "", Direction::Input),
                  "Name of the input workspace");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "Name of the output workspace");

  std::vector<std::string> axisOptions;
  axisOptions.emplace_back("X");
  axisOptions.emplace_back("Y");
  declareProperty("Axis", "X", std::make_shared<StringListValidator>(axisOptions), "The axis to modify");

  declareProperty("Formula", "",
                  "The formula to use to convert the values, x "
                  "or y may be used to refer to the axis "
                  "values.  l1, l2, twotheta and signedtwotheta"
                  "may be used to provide values from the "
                  "instrument geometry.");
  declareProperty("AxisTitle", "", "The label of he new axis. If not set then the title will not change.");
  declareProperty("AxisUnits", "", "The units of the new axis. If not set then the unit will not change");
}

/** Execution of the algorithm
 *
 */
void ConvertAxisByFormula::exec() {
  // get the property values
  MatrixWorkspace_sptr inputWs = getProperty("InputWorkspace");
  std::string axis = getProperty("Axis");
  std::string formula = getProperty("Formula");
  std::string axisTitle = getProperty("AxisTitle");
  std::string axisUnits = getProperty("AxisUnits");

  // Just overwrite if the change is in place
  MatrixWorkspace_sptr outputWs = getProperty("OutputWorkspace");
  if (outputWs != inputWs) {
    auto duplicate = createChildAlgorithm("CloneWorkspace", 0.0, 0.6);
    duplicate->initialize();
    duplicate->setProperty<Workspace_sptr>("InputWorkspace", std::dynamic_pointer_cast<Workspace>(inputWs));
    duplicate->execute();
    Workspace_sptr temp = duplicate->getProperty("OutputWorkspace");
    outputWs = std::dynamic_pointer_cast<MatrixWorkspace>(temp);

    setProperty("OutputWorkspace", outputWs);
  }

  // Get the axes - assume X
  Axis *axisPtr = outputWs->getAxis(0);
  Axis *otherAxisPtr = outputWs->getAxis(1);
  if (axis == "Y") {
    std::swap(axisPtr, otherAxisPtr);
  }

  if (!axisPtr->isNumeric()) {
    throw std::invalid_argument("This algorithm only operates on numeric axes");
  }

  bool isRaggedBins = false;
  bool isRefAxis = false;
  auto *refAxisPtr = dynamic_cast<RefAxis *>(axisPtr);
  if (refAxisPtr != nullptr) {
    isRaggedBins = !outputWs->isCommonBins();
    isRefAxis = true;
  }

  // validate - if formula contains geometry variables then the other axis must
  // be a spectraAxis
  auto *spectrumAxisPtr = dynamic_cast<SpectraAxis *>(otherAxisPtr);
  std::vector<Variable_ptr> variables;
  variables.reserve(8);
  // axis value lookups
  variables.emplace_back(std::make_shared<Variable>("x", false));
  variables.emplace_back(std::make_shared<Variable>("X", false));
  variables.emplace_back(std::make_shared<Variable>("y", false));
  variables.emplace_back(std::make_shared<Variable>("Y", false));
  // geometry lookups
  variables.emplace_back(std::make_shared<Variable>("twotheta", true));
  variables.emplace_back(std::make_shared<Variable>("signedtwotheta", true));
  variables.emplace_back(std::make_shared<Variable>("l1", true));
  variables.emplace_back(std::make_shared<Variable>("l2", true));

  bool isGeometryRequired = false;
  for (auto variablesIter = variables.begin(); variablesIter != variables.end();) {
    if (formula.find((*variablesIter)->name) != std::string::npos) {
      if ((*variablesIter)->isGeometric) {
        if (!spectrumAxisPtr) {
          throw std::invalid_argument("To use geometry values in the equation, "
                                      "the axis not being converted must be a "
                                      "Spectra Axis.");
        }
        isGeometryRequired = true;
      }
      ++variablesIter;
    } else {
      // remove those that are not used
      variablesIter = variables.erase(variablesIter);
    }
  }

  // Create muparser
  mu::Parser p;
  try {
    // set parameter lookups for the axis value
    for (const auto &variable : variables) {
      p.DefineVar(variable->name, &(variable->value));
    }
    // set some constants
    p.DefineConst("pi", M_PI);
    p.DefineConst("h", PhysicalConstants::h);
    p.DefineConst("h_bar", PhysicalConstants::h_bar);
    p.DefineConst("g", PhysicalConstants::g);
    p.DefineConst("mN", PhysicalConstants::NeutronMass);
    p.DefineConst("mNAMU", PhysicalConstants::NeutronMassAMU);
    p.SetExpr(formula);
  } catch (mu::Parser::exception_type &e) {
    std::stringstream ss;
    ss << "Cannot process the formula"
       << ". Muparser error message is: " << e.GetMsg();
    throw std::invalid_argument(ss.str());
  }
  if (isRefAxis) {
    if ((isRaggedBins) || (isGeometryRequired)) {
      // ragged bins or geometry used - we have to calculate for every spectra
      size_t numberOfSpectra_i = outputWs->getNumberHistograms();
      auto &spectrumInfo = outputWs->mutableSpectrumInfo();

      size_t failedDetectorCount = 0;
      Progress prog(this, 0.6, 1.0, numberOfSpectra_i);
      for (size_t i = 0; i < numberOfSpectra_i; ++i) {
        try {
          MantidVec &vec = outputWs->dataX(i);
          setGeometryValues(spectrumInfo, i, variables);
          calculateValues(p, vec, variables);
        } catch (std::runtime_error &)
        // two possible exceptions runtime error and NotFoundError
        // both handled the same way
        {
          // could not find the geometry info for this spectra
          outputWs->getSpectrum(i).clearData();
          spectrumInfo.setMasked(i, true);
          failedDetectorCount++;
        }
        prog.report();
      }
      if (failedDetectorCount != 0) {
        g_log.warning() << "Unable to calculate sample-detector distance for " << failedDetectorCount
                        << " spectra. Masking spectrum.\n";
      }
    } else {
      // common bins - we only have to calculate once

      // Calculate the new (common) X values
      MantidVec &vec = outputWs->dataX(0);
      calculateValues(p, vec, variables);

      // copy xVals to every spectra
      auto numberOfSpectra_i = static_cast<int64_t>(outputWs->getNumberHistograms()); // cast to make openmp happy
      auto xVals = outputWs->refX(0);
      Progress prog(this, 0.6, 1.0, numberOfSpectra_i);
      PARALLEL_FOR_IF(Kernel::threadSafe(*outputWs))
      for (int64_t j = 1; j < numberOfSpectra_i; ++j) {
        PARALLEL_START_INTERRUPT_REGION
        outputWs->setX(j, xVals);
        prog.report();
        PARALLEL_END_INTERRUPT_REGION
      }
      PARALLEL_CHECK_INTERRUPT_REGION
    }
  } else {
    size_t axisLength = axisPtr->length();
    for (size_t i = 0; i < axisLength; ++i) {
      setAxisValue(axisPtr->getValue(i), variables);
      axisPtr->setValue(i, evaluateResult(p));
    }
  }

  // If the units conversion has flipped the ascending direction of X, reverse
  // all the vectors
  size_t midSpectra = outputWs->getNumberHistograms() / 2;
  if (!outputWs->dataX(0).empty() && (outputWs->dataX(0).front() > outputWs->dataX(0).back() ||
                                      outputWs->dataX(midSpectra).front() > outputWs->dataX(midSpectra).back())) {
    g_log.information("Reversing data within the workspace to keep the axes in "
                      "increasing order.");
    this->reverse(outputWs);
  }

  // Set the Unit of the Axis
  if ((!axisUnits.empty()) || (!axisTitle.empty())) {
    try {
      axisPtr->unit() = UnitFactory::Instance().create(axisUnits);
    } catch (Exception::NotFoundError &) {
      if (axisTitle.empty()) {
        axisTitle = axisPtr->unit()->caption();
      }
      if (axisUnits.empty()) {
        axisUnits = axisPtr->unit()->label();
      }
      axisPtr->unit() = std::make_shared<Units::Label>(axisTitle, axisUnits);
    }
  }
}

void ConvertAxisByFormula::setAxisValue(const double value, const std::vector<Variable_ptr> &variables) {
  for (const auto &variable : variables) {
    if (!variable->isGeometric) {
      variable->value = value;
    }
  }
}

void ConvertAxisByFormula::calculateValues(mu::Parser &p, MantidVec &vec, std::vector<Variable_ptr> variables) {
  MantidVec::iterator iter;
  for (iter = vec.begin(); iter != vec.end(); ++iter) {
    setAxisValue(*iter, variables);
    *iter = evaluateResult(p);
  }
}

void ConvertAxisByFormula::setGeometryValues(const API::SpectrumInfo &specInfo, const size_t index,
                                             const std::vector<Variable_ptr> &variables) {
  for (const auto &variable : variables) {
    if (variable->isGeometric) {
      if (variable->name == "twotheta") {
        variable->value = specInfo.twoTheta(index);
      } else if (variable->name == "signedtwotheta") {
        variable->value = specInfo.signedTwoTheta(index);
      } else if (variable->name == "l1") {
        variable->value = specInfo.l1();
      } else if (variable->name == "l2") {
        variable->value = specInfo.l2(index);
      }
    }
  }
}

double ConvertAxisByFormula::evaluateResult(mu::Parser &p) {
  double result;
  try {
    result = p.Eval();
  } catch (mu::Parser::exception_type &e) {
    std::stringstream ss;
    ss << "Failed while processing axis values"
       << ". Muparser error message is: " << e.GetMsg();
    throw std::invalid_argument(ss.str());
  }
  return result;
}

} // namespace Mantid::Algorithms
