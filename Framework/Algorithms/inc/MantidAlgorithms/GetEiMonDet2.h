#ifndef MANTID_ALGORITHMS_GETEIMONDET2_H_
#define MANTID_ALGORITHMS_GETEIMONDET2_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

namespace Mantid {
namespace Algorithms {
/** TODO rewrite
 * Requires an estimate for the initial neutron energy which it uses to
  search for monitor peaks and from these calculate an accurate energy

    Required Properties:
    <UL>
    <LI>InputWorkspace - The X units of this workspace must be time of flight
  with times in micro-seconds</LI>
    <LI>Monitor1ID - The detector ID of the first monitor</LI>
    <LI>Monitor2ID - The detector ID of the second monitor</LI>
    <LI>EnergyEstimate - An approximate value for the typical incident energy,
  energy of neutrons leaving the source (meV)</LI>
    <LI>IncidentEnergy - The calculated energy</LI>
    </UL>

    @author Antti Soininen Institut Laue-Langevin
    @date XX/08/2016

    Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak
  Ridge National Laboratory & European Spallation Source

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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport GetEiMonDet2 : public Mantid::API::Algorithm {
public:
  /// Initialize the algorithm
  void init() override;
  /// Execute the algorithm
  void exec() override;

  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "GetEiMonDet"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    // TODO rewrite
    return "Calculates the kinetic energy of neutrons leaving the source based "
           "on the time it takes for them to travel between a monitor and some "
           "detectors.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 2; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Inelastic\\Ei"; }

private:
  void averageDetectorDistanceAndTOF(const std::vector<size_t> &detectorIndices, double &sampleToDetectorDistance, double &detectorEPP) const;
  double computeTOF(const double distance, const double detectorEPP, const double monitorEPP) const;
  void monitorDistanceAndTOF(const size_t monitorIndex, double &monitorToSampleDistance, double &monitorEPP) const;
  void parseIndices(std::vector<size_t> &detectorIndices, size_t &monitorIndex) const;
  void sanitizeIndices(std::vector<size_t> &detectorIndices, size_t monitorIndex) const;
  Mantid::API::MatrixWorkspace_const_sptr m_detectorWs;
  Mantid::API::ITableWorkspace_const_sptr m_detectorEPPTable;
  Mantid::API::MatrixWorkspace_const_sptr m_monitorWs;
  Mantid::API::ITableWorkspace_const_sptr m_monitorEPPTable;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_GETEIMONDET2_H_*/
