// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
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
auto constexpr MISSING_Y_VALUE = 1.0;
namespace Prop {
std::string const INPUT_WORKSPACE("InputWorkspace");
std::string const FLOOD_WORKSPACE("FloodWorkspace");
std::string const OUTPUT_WORKSPACE("OutputWorkspace");
} // namespace Prop

/** The division operation on event workspaces can make
 *   a mixture of TOF and WEIGHTED events.
 *   This function will switch all events to WEIGHTED.
 *   @param ws Pointer to matrix workspace
 */
void correctEvents(MatrixWorkspace *ws) {
  auto eventWS = dynamic_cast<IEventWorkspace *>(ws);
  if (eventWS) {
    auto const nSpec = eventWS->getNumberHistograms();
    for (size_t i = 0; i < nSpec; ++i) {
      eventWS->getSpectrum(i).switchTo(EventType::WEIGHTED);
    }
  }
}

/** Refactors spectra containing flat or missing flood data back to 1 after rebinning
 *  @param flood Reference to flood Workspace with selected spectra to refactor
 *  @param indexList Vector containing missing or incorrect spectra to which it is assigned a flat flood
 */
void resetMissingSpectra(MatrixWorkspace_sptr &flood, const std::vector<int64_t> &indexList) {
  auto const &floodBlockSize = flood->blocksize();

  for (auto i = 0; auto const &specNum : indexList) {
    if (specNum < 0) {
      flood->mutableY(i).assign(floodBlockSize, MISSING_Y_VALUE);
    }
    i++;
  }
}

/** Make sure that the returned flood workspace match the input workspace
 * in number and order of the spectra.
 *
 *  @param input Input Workspace
 *  @param flood Flood Workspace
 *  @param indexList Vector containing missing or incorrect spectrum to which a flat flood is assigned
 *  @return A flood matrix workspace equal to input in number and order of spectra
 */
MatrixWorkspace_sptr makeEqualSizes(const MatrixWorkspace_sptr &input, const MatrixWorkspace_sptr &flood,
                                    const std::vector<int64_t> &indexList) {
  auto newFlood = WorkspaceFactory::Instance().create(flood, input->getNumberHistograms());
  auto const floodBlocksize = flood->blocksize();
  const ISpectrum *missingSpectrum = nullptr;
  for (size_t i = 0; i < indexList.size(); ++i) {
    auto const j = (indexList)[i];
    if (j < 0) {
      if (missingSpectrum) {
        newFlood->getSpectrum(i).copyDataFrom(*missingSpectrum);
      } else {
        newFlood->mutableY(i).assign(floodBlocksize, MISSING_Y_VALUE);
        newFlood->mutableE(i).assign(floodBlocksize, 0.0);
        missingSpectrum = &newFlood->getSpectrum(i);
      }
    } else {
      newFlood->getSpectrum(i).copyDataFrom(flood->getSpectrum(j));
    }
  }
  return newFlood;
}
} // namespace

namespace Mantid::Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ApplyFloodWorkspace)

const std::string ApplyFloodWorkspace::name() const { return "ApplyFloodWorkspace"; }

const std::string ApplyFloodWorkspace::summary() const {
  return "Algorithm to apply a flood correction to a workspace.";
}

int ApplyFloodWorkspace::version() const { return 1; }

const std::vector<std::string> ApplyFloodWorkspace::seeAlso() const {
  return {"ReflectometryReductionOneAuto", "CreateFloodWorkspace"};
}

const std::string ApplyFloodWorkspace::category() const { return "Reflectometry\\ISIS"; }

void ApplyFloodWorkspace::init() {

  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(Prop::INPUT_WORKSPACE, "", Direction::Input),
                  "The workspace to correct.");

  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(Prop::FLOOD_WORKSPACE, "", Direction::Input),
                  "The flood workspace.");

  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(Prop::OUTPUT_WORKSPACE, "", Direction::Output),
                  "The corrected workspace.");
}

void ApplyFloodWorkspace::exec() {
  MatrixWorkspace_sptr input = getProperty(Prop::INPUT_WORKSPACE);
  MatrixWorkspace_sptr flood = getProperty(Prop::FLOOD_WORKSPACE);

  auto indexTable = std::make_shared<std::vector<int64_t>>();
  if (input->size() != flood->size()) {
    indexTable = BinaryOperation::buildBinaryOperationTable(input, flood);
    flood = makeEqualSizes(input, flood, *indexTable);
  }

  auto const inputXUnitId = input->getAxis(0)->unit()->unitID();
  bool const doConvertUnits = ((flood->getAxis(0)->unit()->unitID() != inputXUnitId) && (inputXUnitId != "Empty"));
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
    resetMissingSpectra(flood, *indexTable);
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

} // namespace Mantid::Algorithms
