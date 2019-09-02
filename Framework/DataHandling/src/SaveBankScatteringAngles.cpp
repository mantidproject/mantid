// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/SaveBankScatteringAngles.h"

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidGeometry/Instrument.h"

#include <cmath>
#include <fstream>

namespace {

double radToDeg(const double rad) {
  const double radToDegFactor = 180 / M_PI;
  return rad * radToDegFactor;
}
} // anonymous namespace

namespace Mantid {
namespace DataHandling {

using namespace API;

DECLARE_ALGORITHM(SaveBankScatteringAngles)

const std::string SaveBankScatteringAngles::name() const {
  return "SaveBankScatteringAngles";
}

const std::string SaveBankScatteringAngles::summary() const {
  return "Save scattering angles two-theta and phi for each workspace in a "
         "GroupWorkspace of focused banks";
}

int SaveBankScatteringAngles::version() const { return 1; }

const std::vector<std::string> SaveBankScatteringAngles::seeAlso() const {
  return {"SaveFocusedXYE"};
}

const std::string SaveBankScatteringAngles::category() const {
  return "DataHandling\\Text;Diffraction\\DataHandling";
}

const std::string SaveBankScatteringAngles::PROP_FILENAME = "Filename";

const std::string SaveBankScatteringAngles::PROP_INPUT_WS = "InputWorkspace";

void SaveBankScatteringAngles::init() {
  declareProperty(std::make_unique<WorkspaceProperty<WorkspaceGroup>>(
                      PROP_INPUT_WS, "", Kernel::Direction::Input),
                  "A GroupWorkspace where every sub-workspace is a "
                  "single-spectra focused run corresponding to a particular "
                  "bank");

  const static std::vector<std::string> exts{".txt", ".new"};
  declareProperty(std::make_unique<FileProperty>(PROP_FILENAME, "",
                                                 FileProperty::Save, exts),
                  "The name of the file to save to");
}

void SaveBankScatteringAngles::exec() {
  const std::string filename = getProperty(PROP_FILENAME);
  std::ofstream outFile(filename.c_str());

  if (!outFile) {
    g_log.error(strerror(errno));
    throw Kernel::Exception::FileError("Unable to create file: ", filename);
  }

  outFile << std::fixed << std::setprecision(10);

  const API::WorkspaceGroup_sptr inputWS = getProperty(PROP_INPUT_WS);
  for (int i = 0; i < inputWS->getNumberOfEntries(); ++i) {
    const auto ws = inputWS->getItem(i);
    const auto matrixWS = boost::dynamic_pointer_cast<MatrixWorkspace>(ws);
    const auto &instrument = matrixWS->getInstrument();
    const auto &samplePosition = instrument->getSample()->getPos();
    const auto &sourcePosition = instrument->getSource()->getPos();
    const auto &detector = matrixWS->getDetector(0);
    const auto &detectorPosition = detector->getPos();

    const auto beamVector = samplePosition - sourcePosition;
    const auto detectorVector = detectorPosition - samplePosition;

    const auto twoTheta = radToDeg(beamVector.angle(detectorVector));
    const auto phi = radToDeg(detector->getPhi());
    const auto group = matrixWS->getSpectrum(0).getSpectrumNo();

    outFile << "bank :    " << i << "  "
            << "group:     " << group << "    " << twoTheta << "    " << phi
            << '\n';
  }

  outFile.close();

  if (outFile.fail()) {
    g_log.error(strerror(errno));
    throw Kernel::Exception::FileError("Failed to save file", filename);
  }
}

std::map<std::string, std::string> SaveBankScatteringAngles::validateInputs() {
  std::map<std::string, std::string> issues;

  const API::WorkspaceGroup_sptr inputWS = getProperty(PROP_INPUT_WS);
  for (const auto &ws : *inputWS) {
    const auto matrixWS = boost::dynamic_pointer_cast<MatrixWorkspace>(ws);
    if (matrixWS) {
      if (matrixWS->getNumberHistograms() != 1) {
        issues[PROP_INPUT_WS] = "The workspace " + matrixWS->getName() +
                                " has the wrong number of histograms. It "
                                "should contain data for a single focused "
                                "spectra";
        return issues;
      }
    } else {
      issues[PROP_INPUT_WS] =
          "The workspace " + ws->getName() +
          " is of the wrong type. It should be a MatrixWorkspace";
      return issues;
    }
  }
  return issues;
}

} // namespace DataHandling
} // namespace Mantid
