#ifndef MANTID_ALGORITHMS_CREATESAMPLEWORKSPACE_H_
#define MANTID_ALGORITHMS_CREATESAMPLEWORKSPACE_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidKernel/PseudoRandomNumberGenerator.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Algorithms {

/** CreateSampleWorkspace : This algorithm is intended for the creation of
  sample workspaces for usage examples and other situations

  Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport CreateSampleWorkspace : public API::Algorithm {
public:
  CreateSampleWorkspace();
  ~CreateSampleWorkspace() override;

  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"CreateWorkspace"};
  }
  const std::string category() const override;
  /// Algorithm's summary
  const std::string summary() const override {
    return "Creates sample workspaces for usage examples and other situations.";
  }

private:
  void init() override;
  void exec() override;

  DataObjects::EventWorkspace_sptr
  createEventWorkspace(int numPixels, int numBins, int numMonitors,
                       int numEvents, double x0, double binDelta,
                       Geometry::Instrument_sptr inst,
                       const std::string &functionString, bool isRandom);
  API::MatrixWorkspace_sptr
  createHistogramWorkspace(int numPixels, int numBins, int numMonitors,
                           double x0, double binDelta,
                           Geometry::Instrument_sptr inst,
                           const std::string &functionString, bool isRandom);
  API::MatrixWorkspace_sptr createScanningWorkspace(
      int numBins, double x0, double binDelta, Geometry::Instrument_sptr inst,
      const std::string &functionString, bool isRandom, int numScanPoints);
  Geometry::Instrument_sptr createTestInstrumentRectangular(
      API::Progress &progress, int numBanks, int numMonitors, int pixels,
      double pixelSpacing, const double bankDistanceFromSample,
      const double sourceSampleDistance);
  Geometry::IObject_sptr createCappedCylinder(double radius, double height,
                                              const Kernel::V3D &baseCentre,
                                              const Kernel::V3D &axis,
                                              const std::string &id);
  Geometry::IObject_sptr createSphere(double radius, const Kernel::V3D &centre,
                                      const std::string &id);
  std::vector<double> evalFunction(const std::string &functionString,
                                   const std::vector<double> &xVal,
                                   double noiseScale);
  void replaceAll(std::string &str, const std::string &from,
                  const std::string &to);
  void addChopperParameters(API::MatrixWorkspace_sptr &ws);

  /// A pointer to the random number generator
  Kernel::PseudoRandomNumberGenerator *m_randGen;
  std::map<std::string, std::string> m_preDefinedFunctionmap;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_CREATESAMPLEWORKSPACE_H_ */
