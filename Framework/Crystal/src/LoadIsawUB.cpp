// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCrystal/LoadIsawUB.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/Sample.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/Strings.h"
#include <fstream>

using namespace Mantid::Kernel::Strings;
using Mantid::Geometry::OrientedLattice;
using Mantid::Kernel::DblMatrix;

namespace Mantid::Crystal {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadIsawUB)

using namespace Mantid::Kernel;
using namespace Mantid::API;

/** Initialize the algorithm's properties.
 */
void LoadIsawUB::init() {
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("InputWorkspace", "", Direction::InOut),
                  "An input workspace to which to add the lattice information.");

  const std::vector<std::string> exts{".mat", ".ub", ".txt"};
  declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::Load, exts),
                  "Path to an ISAW-style UB matrix text file.");

  declareProperty("CheckUMatrix", true,
                  "If True (default) then a check is "
                  "performed to ensure the U matrix is a "
                  "proper rotation matrix");
}

/** Execute the algorithm.
 */
void LoadIsawUB::exec() {
  std::string Filename = getProperty("Filename");

  // Open the file
  std::ifstream in(Filename.c_str());

  Kernel::DblMatrix ub(3, 3);
  std::string s;
  double val;

  // Read the ISAW UB matrix
  for (size_t row = 0; row < 3; row++) {
    for (size_t col = 0; col < 3; col++) {
      s = getWord(in, true);
      if (!convert(s, val))
        throw std::runtime_error("The string '" + s + "' in the file was not understood as a number.");
      ub[row][col] = val;
    }
    readToEndOfLine(in, true);
  }

  readModulatedUB(in, ub);
}
void LoadIsawUB::readModulatedUB(std::ifstream &in, DblMatrix &ub) {
  int ModDim = 0;
  Kernel::DblMatrix modub(3, 3);
  Kernel::DblMatrix ModVecErr(3, 3);
  int maxorder = 0;
  bool crossterm = false;
  std::string s;
  double val;
  s = getWord(in, true);
  if (!convert(s, val)) {
    readToEndOfLine(in, true);
    for (size_t row = 0; row < 3; row++) {
      for (size_t col = 0; col < 3; col++) {
        s = getWord(in, true);
        if (!convert(s, val))
          throw std::runtime_error("The string '" + s + "' in the file was not understood as a number.");
        modub[row][col] = val;
      }
      readToEndOfLine(in, true);
      if (modub[row][0] != 0.0 || modub[row][1] != 0.0 || modub[row][2] != 0.0)
        ModDim++;
    }
  }

  readToEndOfLine(in, true);

  double latVals[6];
  for (double &latVal : latVals) {
    s = getWord(in, true);
    if (!convert(s, val))
      throw std::runtime_error("The string '" + s + "' in the file was not understood as a number.");
    latVal = val;
  }

  if (ModDim > 0) {
    readToEndOfLine(in, true);
    readToEndOfLine(in, true);
    for (int i = 0; i < ModDim; i++) {
      readToEndOfLine(in, true);
      for (int j = 0; j < 4; j++)
        s = getWord(in, true);
      for (int j = 0; j < 3; j++) {
        s = getWord(in, true);
        if (!convert(s, val))
          throw std::runtime_error("The string '" + s + "' in the file was not understood as a number.");
        ModVecErr[i][j] = val;
      }
      readToEndOfLine(in, true);
    }

    readToEndOfLine(in, true);
    for (int j = 0; j < 3; j++)
      s = getWord(in, true);
    if (!convert(s, val))
      throw std::runtime_error("The string '" + s + "' in the file was not understood as a number.");
    maxorder = static_cast<int>(val);
    readToEndOfLine(in, true);
    for (int j = 0; j < 3; j++)
      s = getWord(in, true);
    bool valBool;
    if (!convert(s, valBool))
      throw std::runtime_error("The string '" + s + "' in the file was not understood as a number.");
    crossterm = valBool;
  }
  // Adjust the UB by transposing
  ub = ub.Transpose();
  modub = modub.Transpose();

  /* The method in OrientedLattice gets both the lattice parameters and the U
   * matrix from the UB matrix.
   * This is compatible (same results) with the ISAW lattice parameters */
  auto latt = std::make_unique<OrientedLattice>();
  latt->setUB(ub);
  latt->setError(latVals[0], latVals[1], latVals[2], latVals[3], latVals[4], latVals[5]);
  latt->setModUB(modub);

  for (int i = 0; i < ModDim; i++)
    latt->setModerr(i, ModVecErr[i][0], ModVecErr[i][1], ModVecErr[i][2]);

  latt->setMaxOrder(maxorder);
  latt->setCrossTerm(crossterm);

  DblMatrix U = latt->getU();

  // Swap rows around to accound for IPNS convention
  DblMatrix U2 = U;
  // Swap rows around
  for (size_t r = 0; r < 3; r++) {
    U2[2][r] = U[0][r];
    U2[1][r] = U[2][r];
    U2[0][r] = U[1][r];
  }
  U = U2;
  const bool checkU = getProperty("CheckUMatrix");
  latt->setU(U, !checkU);

  // In and Out workspace.
  Workspace_sptr ws1 = getProperty("InputWorkspace");

  ExperimentInfo_sptr ws;
  MultipleExperimentInfos_sptr MDWS = std::dynamic_pointer_cast<MultipleExperimentInfos>(ws1);
  if (MDWS != nullptr) {
    ws = MDWS->getExperimentInfo(0);
  } else {
    ws = std::dynamic_pointer_cast<ExperimentInfo>(ws1);
  }
  if (!ws)
    throw std::invalid_argument("Must specify either a MatrixWorkspace or a "
                                "PeaksWorkspace or a MDWorkspace.");

  // Save a copy of it to every experiment info in MD workspaces
  if ((MDWS != nullptr) && (MDWS->getNumExperimentInfo() > 1)) {
    for (uint16_t i = 1; i < MDWS->getNumExperimentInfo(); i++) {
      ws = MDWS->getExperimentInfo(i);
      ws->mutableSample().setOrientedLattice(std::make_unique<OrientedLattice>(*latt));
    }
    ws = MDWS->getExperimentInfo(0);
  }
  // Save it into the main workspace
  ws->mutableSample().setOrientedLattice(std::move(latt));

  this->setProperty("InputWorkspace", ws1);
}

} // namespace Mantid::Crystal
