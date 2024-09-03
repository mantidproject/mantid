// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidReflectometry/SpecularReflectionCalculateTheta.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidKernel/PropertyWithValue.h"

#include <cmath>

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::API;

namespace Mantid::Reflectometry {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SpecularReflectionCalculateTheta)

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string SpecularReflectionCalculateTheta::name() const { return "SpecularReflectionCalculateTheta"; }

/// Algorithm's version for identification. @see Algorithm::version
int SpecularReflectionCalculateTheta::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string SpecularReflectionCalculateTheta::category() const { return "Reflectometry"; }

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void SpecularReflectionCalculateTheta::init() {
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("InputWorkspace", "", Direction::Input),
                  "An Input workspace to calculate the specular relection theta on.");
  this->initCommonProperties();
  declareProperty(std::make_unique<PropertyWithValue<double>>("TwoTheta", Mantid::EMPTY_DBL(), Direction::Output),
                  "Calculated two theta scattering angle in degrees.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void SpecularReflectionCalculateTheta::exec() {

  // This algorithm expects detectors to actually be at theta rather than
  // twoTheta (for historical reasons), so we need to multiply by 2 to get the
  // real twoTheta. v2 of this algorithm works with detectors at twoTheta.
  const double twoTheta = 2 * calculateTwoTheta();

  std::stringstream strstream;
  strstream << "Recalculated two theta as: " << twoTheta;

  this->g_log.information(strstream.str());

  this->setProperty("TwoTheta", twoTheta);
}

} // namespace Mantid::Reflectometry
