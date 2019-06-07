// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/ApplyFloodWorkspace.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/IEventWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAlgorithms/BinaryOperation.h"
#include "MantidKernel/Unit.h"

using namespace Mantid::Kernel;
using namespace Mantid::Algorithms;
using namespace Mantid::API;

namespace {
namespace Prop {
std::string const INPUT_WORKSPACE("InputWorkspace");
std::string const FLOOD_WORKSPACE("FloodWorkspace");
std::string const OUTPUT_WORKSPACE("OutputWorkspace");
} // namespace Prop

/// The division operation on event workspaces can make
/// a mixture of TOF and WEIGHTED events. This function
/// will switch all events to WEIGHTED.
void correctEvents(MatrixWorkspace *ws) {
  auto eventWS = dynamic_cast<IEventWorkspace *>(ws);
  if (eventWS) {
    auto const nSpec = eventWS->getNumberHistograms();
    for (size_t i = 0; i < nSpec; ++i) {
      eventWS->getSpectrum(i).switchTo(EventType::WEIGHTED);
    }
  }
}

/// Make sure that the returned flood workspace match the input workspace
/// in number and order of the spectra.
MatrixWorkspace_sptr makeEqualSizes(const MatrixWorkspace_sptr &input,
                                    const MatrixWorkspace_sptr &flood) {
  auto newFlood =
      WorkspaceFactory::Instance().create(flood, input->getNumberHistograms());
  auto const table = BinaryOperation::buildBinaryOperationTable(input, flood);
  auto const floodBlocksize = flood->blocksize();
  const ISpectrum *missingSpectrum = nullptr;
  for (size_t i = 0; i < table->size(); ++i) {
    auto const j = (*table)[i];
    if (j < 0) {
      if (missingSpectrum) {
        newFlood->getSpectrum(i).copyDataFrom(*missingSpectrum);
      } else {
        newFlood->dataY(i).assign(floodBlocksize, 1.0);
        newFlood->dataE(i).assign(floodBlocksize, 0.0);
        missingSpectrum = &newFlood->getSpectrum(i);
      }
    } else {
      newFlood->getSpectrum(i).copyDataFrom(flood->getSpectrum(j));
    }
  }
  return newFlood;
}
} // namespace

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ApplyFloodWorkspace)

const std::string ApplyFloodWorkspace::name() const {
  return "ApplyFloodWorkspace";
}

const std::string ApplyFloodWorkspace::summary() const {
  return "Algorithm to apply a flood correction to a workspace.";
}

int ApplyFloodWorkspace::version() const { return 1; }

const std::vector<std::string> ApplyFloodWorkspace::seeAlso() const {
  return {"ReflectometryReductionOneAuto", "CreateFloodWorkspace"};
}

const std::string ApplyFloodWorkspace::category() const {
  return "Reflectometry\\ISIS";
}

void ApplyFloodWorkspace::init() {

  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      Prop::INPUT_WORKSPACE, "", Direction::Input),
                  "The workspace to correct.");

  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      Prop::FLOOD_WORKSPACE, "", Direction::Input),
                  "The flood workspace.");

  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      Prop::OUTPUT_WORKSPACE, "", Direction::Output),
                  "The corrected workspace.");
}

void ApplyFloodWorkspace::exec() {
  MatrixWorkspace_sptr input = getProperty(Prop::INPUT_WORKSPACE);
  MatrixWorkspace_sptr flood = getProperty(Prop::FLOOD_WORKSPACE);

  if (input->size() != flood->size()) {
    flood = makeEqualSizes(input, flood);
  }

  auto const inputXUnitId = input->getAxis(0)->unit()->unitID();
  bool const doConvertUnits =
      flood->getAxis(0)->unit()->unitID() != inputXUnitId;
  bool const doRebin = flood->blocksize() > 1;

  if (doRebin) {
    if (doConvertUnits) {
      auto convert = createChildAlgorithm("ConvertUnits", 0, 1);
      convert->setProperty("InputWorkspace", flood);
      convert->setProperty("Target", inputXUnitId);
      convert->setProperty("OutputWorkspace", "dummy");
      convert->execute();
      flood = convert->getProperty("OutputWorkspace");
    }
    auto rebin = createChildAlgorithm("RebinToWorkspace", 0, 1);
    rebin->setProperty("WorkspaceToRebin", flood);
    rebin->setProperty("WorkspaceToMatch", input);
    rebin->setProperty("OutputWorkspace", "dummy");
    rebin->execute();
    flood = rebin->getProperty("OutputWorkspace");
  }

  auto divide = createChildAlgorithm("Divide", 0, 1);
  divide->setProperty("LHSWorkspace", input);
  divide->setProperty("RHSWorkspace", flood);
  divide->setProperty("OutputWorkspace", "dummy");
  divide->execute();
  MatrixWorkspace_sptr output = divide->getProperty("OutputWorkspace");
  correctEvents(output.get());
  setProperty(Prop::OUTPUT_WORKSPACE, output);
}

} // namespace Algorithms
} // namespace Mantid
