// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCrystal/AddPeakHKL.h"

#include "MantidGeometry/Crystal/IPeak.h"

#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidKernel/ArrayLengthValidator.h"
#include "MantidKernel/ArrayProperty.h"

namespace Mantid {
namespace Crystal {

using namespace Mantid::Kernel;
using namespace Mantid::API;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(AddPeakHKL)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string AddPeakHKL::name() const { return "AddPeakHKL"; }

/// Algorithm's version for identification. @see Algorithm::version
int AddPeakHKL::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string AddPeakHKL::category() const { return "Crystal\\Peaks"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string AddPeakHKL::summary() const {
  return "Add a peak in the hkl frame";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void AddPeakHKL::init() {
  declareProperty(make_unique<WorkspaceProperty<Mantid::API::IPeaksWorkspace>>(
                      "Workspace", "", Direction::InOut),
                  "An input workspace.");
  // clang-format off
    declareProperty(make_unique<ArrayProperty<double>>("HKL", boost::make_shared<ArrayLengthValidator<double> > (3)), "HKL point to add");
  // clang-format on
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void AddPeakHKL::exec() {
  IPeaksWorkspace_sptr peakWS = this->getProperty("Workspace");
  const std::vector<double> hklValue = this->getProperty("HKL");
  Mantid::Geometry::IPeak *peak =
      peakWS->createPeakHKL(V3D(hklValue[0], hklValue[1], hklValue[2]));
  peakWS->addPeak(*peak);
  delete peak;
}

} // namespace Crystal
} // namespace Mantid
