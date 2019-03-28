// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_GETEIMONDET2_H_
#define MANTID_ALGORITHMS_GETEIMONDET2_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/DeprecatedAlgorithm.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

namespace Mantid {
namespace Algorithms {

/** Estimates the incident neutron energy from the time of flight
    between a monitor and a set of detectors.
*/
class DLLExport GetEiMonDet2 : public API::Algorithm,
                               public API::DeprecatedAlgorithm {
public:
  /// Constructs a GetEiMonDet2 object
  GetEiMonDet2();

  /// Initializes the algorithm
  void init() override;

  /// Executes the algorithm
  void exec() override;

  /// Returns algorithm's name for identification

  const std::string name() const override { return "GetEiMonDet"; }

  /// Returns a summary of algorithm's purpose
  const std::string summary() const override {
    return "Calculates the kinetic energy of neutrons leaving the source based "
           "on the time it takes for them to travel between a monitor and a "
           "set of detectors.";
  }

  /// Returns algorithm's version for identification
  int version() const override { return 2; }
  const std::vector<std::string> seeAlso() const override { return {"GetEi"}; }

  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Inelastic\\Ei"; }

private:
  /// Calculates the average sample-to-detector distance and TOF
  void averageDetectorDistanceAndTOF(const std::vector<size_t> &detectorIndices,
                                     double &sampleToDetectorDistance,
                                     double &detectorEPP);

  /// Calculates the total TOF from monitor to detectors
  double computeTOF(const double distance, const double detectorEPP,
                    const double monitorEPP);

  /// Sets the monitor-to-sample distance and TOF
  void monitorDistanceAndTOF(const size_t monitorIndex,
                             double &monitorToSampleDistance,
                             double &monitorEPP) const;

  /// Reads detector and monitor indices from properties
  void parseIndices(std::vector<size_t> &detectorIndices,
                    size_t &monitorIndex) const;

  /// Removes duplicates and checks consistency
  void sanitizeIndices(std::vector<size_t> &detectorIndices,
                       size_t monitorIndex) const;

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

#endif /*MANTID_ALGORITHMS_GETEIMONDET2_H_*/
