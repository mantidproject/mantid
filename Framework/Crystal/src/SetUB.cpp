// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCrystal/SetUB.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/Sample.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidKernel/ArrayLengthValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using Mantid::Geometry::OrientedLattice;

namespace Mantid::Crystal {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SetUB)

/// Algorithm's name for identification. @see Algorithm::name
const std::string SetUB::name() const { return "SetUB"; }

/// Algorithm's version for identification. @see Algorithm::version
int SetUB::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string SetUB::category() const { return "Crystal\\UBMatrix"; }

/** Initialize the algorithm's properties.
 */
void SetUB::init() {
  auto mustBePositive = std::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  auto reasonableAngle = std::make_shared<BoundedValidator<double>>();
  reasonableAngle->setLower(5.0);
  reasonableAngle->setUpper(175.0);
  auto mustBe3D = std::make_shared<ArrayLengthValidator<double>>(3);
  auto threeVthree = std::make_shared<ArrayLengthValidator<double>>(9);
  std::vector<double> zeroes(9, 0.), u0(3, 0), v0(3, 0);
  u0[0] = 1.;
  v0[1] = 1.;
  this->declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("Workspace", "", Direction::InOut),
                        "An input workspace.");
  this->declareProperty(
      std::make_unique<PropertyWithValue<double>>("a", 1.0, mustBePositive->clone(), Direction::Input),
      "Lattice parameter a");
  this->declareProperty(
      std::make_unique<PropertyWithValue<double>>("b", 1.0, mustBePositive->clone(), Direction::Input),
      "Lattice parameter b");
  this->declareProperty(
      std::make_unique<PropertyWithValue<double>>("c", 1.0, std::move(mustBePositive), Direction::Input),
      "Lattice parameter c");
  this->declareProperty(
      std::make_unique<PropertyWithValue<double>>("alpha", 90.0, reasonableAngle->clone(), Direction::Input),
      "Lattice parameter alpha (degrees)");
  this->declareProperty(
      std::make_unique<PropertyWithValue<double>>("beta", 90.0, reasonableAngle->clone(), Direction::Input),
      "Lattice parameter beta (degrees)");
  this->declareProperty(
      std::make_unique<PropertyWithValue<double>>("gamma", 90.0, std::move(reasonableAngle), Direction::Input),
      "Lattice parameter gamma(degrees) ");
  this->declareProperty(std::make_unique<ArrayProperty<double>>("u", std::move(u0), mustBe3D->clone()),
                        "Vector along k_i, when goniometer is at 0");
  this->declareProperty(std::make_unique<ArrayProperty<double>>("v", std::move(v0), std::move(mustBe3D)),
                        "In plane vector perpendicular to k_i, when goniometer is at 0");
  this->declareProperty(std::make_unique<ArrayProperty<double>>("UB", std::move(zeroes), threeVthree), "UB Matrix");
  this->declareProperty(std::make_unique<PropertyWithValue<int>>("MDSampleNumber", EMPTY_INT(), Direction::Input),
                        "For an MD workspace, the sample number to wich to "
                        "attach an oriented lattice (starting from 0). No "
                        "number, or negative number, means that it will copy "
                        "to all samples");
}

/** Execute the algorithm.
  @throws :: |B.u|=0 or |B.v|=0 or u||v
 */
void SetUB::exec() {
  std::unique_ptr<Mantid::Geometry::OrientedLattice> lattice;
  std::vector<double> UBvec = getProperty("UB");
  Mantid::Kernel::DblMatrix UBMatrix(UBvec), zeroMatrix(3, 3);
  if (UBMatrix == zeroMatrix) {
    double a, b, c, alpha, beta, gamma;
    a = getProperty("a");
    b = getProperty("b");
    c = getProperty("c");
    alpha = getProperty("alpha");
    beta = getProperty("beta");
    gamma = getProperty("gamma");
    std::vector<double> u = getProperty("u");
    std::vector<double> v = getProperty("v");

    lattice = std::make_unique<OrientedLattice>(a, b, c, alpha, beta, gamma);
    lattice->setUFromVectors(Mantid::Kernel::V3D(u[0], u[1], u[2]), Mantid::Kernel::V3D(v[0], v[1], v[2]));
  } else {
    if (UBMatrix.determinant() == 0)
      throw std::invalid_argument("UB matrix determinant is 0");
    else {
      lattice = std::make_unique<OrientedLattice>();
      lattice->setUB(UBMatrix);
    }
  }

  // now attach the oriented lattice to the workspace
  Workspace_sptr ws = this->getProperty("Workspace");

  // Sample copy;
  MultipleExperimentInfos_sptr mdws = std::dynamic_pointer_cast<MultipleExperimentInfos>(ws);
  if (mdws != nullptr) {
    int sampleNumber = getProperty("MDSampleNumber");
    if ((sampleNumber == EMPTY_INT()) || (sampleNumber < 0)) // copy to all samples
    {
      for (uint16_t i = 0; i < mdws->getNumExperimentInfo(); i++) {
        mdws->getExperimentInfo(i)->mutableSample().setOrientedLattice(std::make_unique<OrientedLattice>(*lattice));
      }
    } else // copy to a single sample
    {
      if (static_cast<uint16_t>(sampleNumber) > (mdws->getNumExperimentInfo() - 1)) {
        g_log.warning() << "Number greater than the number of last sample in "
                           "the workspace ("
                        << (mdws->getNumExperimentInfo() - 1) << "). Will use sample number 0 instead\n";
        sampleNumber = 0;
      }
      mdws->getExperimentInfo(static_cast<uint16_t>(sampleNumber))
          ->mutableSample()
          .setOrientedLattice(std::move(lattice));
    }
  } else // peaks workspace or matrix workspace
  {
    ExperimentInfo_sptr ei = std::dynamic_pointer_cast<ExperimentInfo>(ws);
    if (!ei)
      throw std::invalid_argument("Wrong type of workspace");
    ei->mutableSample().setOrientedLattice(std::move(lattice));
  }
  this->setProperty("Workspace", ws);
}

} // namespace Mantid::Crystal
