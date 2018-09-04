#ifndef MANTID_ALGORITHMS_GETEIMONDET3_H_
#define MANTID_ALGORITHMS_GETEIMONDET3_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

namespace Mantid {
namespace Indexing {
class SpectrumIndexSet;
}
namespace Algorithms {

/** Estimates the incident neutron energy from the time of flight
    between a monitor and a set of detectors.

    Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak
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
class DLLExport GetEiMonDet3 : public Mantid::API::Algorithm {
public:
  /// Returns algorithm's name for identification

  const std::string name() const override;

  /// Returns a summary of algorithm's purpose
  const std::string summary() const override;
  /// Returns algorithm's version for identification
  int version() const override;
  const std::vector<std::string> seeAlso() const override;

  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override;

private:
  /// Initializes the algorithm
  void init() override;

  /// Executes the algorithm
  void exec() override;
  /// Calculates the average sample-to-detector distance and TOF
  void averageDetectorDistanceAndTOF(
      const Indexing::SpectrumIndexSet &detectorIndices,
      double &sampleToDetectorDistance, double &detectorEPP);

  /// Calculates the total TOF from monitor to detectors
  double computeTOF(const double detectorEPP, const double monitorEPP);

  /// Sets the monitor-to-sample distance and TOF
  void monitorDistanceAndTOF(const size_t monitorIndex,
                             double &monitorToSampleDistance,
                             double &monitorEPP) const;

  /// Shared pointer to the detector workspace
  Mantid::API::MatrixWorkspace_const_sptr m_detectorWs;
  /// Shared pointer to the detectors' EPP table
  Mantid::API::ITableWorkspace_const_sptr m_detectorEPPTable;
  /// Shared pointer to the monitor workspace
  Mantid::API::MatrixWorkspace_const_sptr m_monitorWs;
  /// Shared pointer to the monitor's EPP table
  Mantid::API::ITableWorkspace_const_sptr m_monitorEPPTable;
};

} // namespace Algorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_GETEIMONDET3_H_*/
