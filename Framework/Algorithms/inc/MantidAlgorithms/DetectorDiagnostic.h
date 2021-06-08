// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidGeometry/IComponent.h"

#include <set>

namespace Mantid {
namespace Algorithms {
/**
  A base class for a detector diagnostic algorithm. It has not exec
  implemenation but provides functions
  that are common among these algorithms such as calculating the median and
  writing to a file.

  @author Martyn Gigg, Tessella plc
  @date 2010-12-09
*/
class MANTID_ALGORITHMS_DLL DetectorDiagnostic : public API::Algorithm {
public:
  /// Default constructor
  DetectorDiagnostic();
  /// Algorithm's category for identification
  const std::string category() const override;
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Identifies histograms and their detectors that have total numbers "
           "of counts over a user defined maximum or less than the user define "
           "minimum.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"FindDetectorsOutsideLimits", "FindDeadDetectors", "MedianDetectorTest", "DetectorEfficiencyVariation"};
  }

private:
  // Overridden Algorithm methods
  void init() override;
  void exec() override;
  /// Apply a given mask
  void applyMask(const API::MatrixWorkspace_sptr &inputWS, const API::MatrixWorkspace_sptr &maskWS);
  /// Perform checks on detector vanadium
  API::MatrixWorkspace_sptr doDetVanTest(const API::MatrixWorkspace_sptr &inputWS, int &nFails);

protected:
  /// Get the total counts for each spectra
  API::MatrixWorkspace_sptr integrateSpectra(const API::MatrixWorkspace_sptr &inputWS, const int indexMin,
                                             const int indexMax, const double lower, const double upper,
                                             const bool outputWorkspace2D = false);

  DataObjects::MaskWorkspace_sptr generateEmptyMask(const API::MatrixWorkspace_const_sptr &inputWS);

  /// Calculate the median of the given workspace. This assumes that the input
  /// workspace contains
  /// integrated counts
  std::vector<double> calculateMedian(const API::MatrixWorkspace &input, bool excludeZeroes,
                                      const std::vector<std::vector<size_t>> &indexmap);
  /// Convert to a distribution
  API::MatrixWorkspace_sptr convertToRate(API::MatrixWorkspace_sptr workspace);
  /// method to check which spectra should be grouped when calculating the
  /// median
  std::vector<std::vector<size_t>> makeMap(const API::MatrixWorkspace_sptr &countsWS);
  /// method to create the map with all spectra
  std::vector<std::vector<size_t>> makeInstrumentMap(const API::MatrixWorkspace &countsWS);

  /** @name Progress reporting */
  //@{
  /// For the progress bar, estimates of how many additions,
  /// or equivalent, member functions will do for each spectrum
  enum RunTime {
    /// An estimate of how much work SolidAngle will do for each spectrum
    RTGetSolidAngle = 15000,
    /// Estimate of the work required from Integrate for each spectrum
    RTGetTotalCounts = 5000,
    /// Work required by the ConvertToDistribution algorithm
    RTGetRate = 100,
    /// Time taken to find failing detectors
    RTMarkDetects = 200,
    /// Time taken to find failing detectors
    RTWriteFile = 200,
    /// The total of all run times
    RTTotal = RTGetSolidAngle + RTGetTotalCounts + RTGetRate + RTMarkDetects + RTWriteFile
  };

  /// Update the fraction complete estimate assuming that the algorithm has
  /// completed
  /// a task with estimated RunTime toAdd
  double advanceProgress(double toAdd);
  /// Update the fraction complete estimate assuming that the algorithm
  /// aborted a task with estimated RunTime toAdd
  void failProgress(RunTime aborted);

  /// An estimate of the percentage of the algorithm runtimes that has been
  /// completed
  double m_fracDone;
  /// An estimate total number of additions or equilivent required to compute a
  /// spectrum
  int m_TotalTime;

  /// number of parents up, 0 go to instrument
  int m_parents;
  /// The number of tests to be run
  double m_progStepWidth;
  /// Starting workspace index to run tests on
  int m_minIndex;
  /// Ending workspace index to run tests on
  int m_maxIndex;
  /// Starting x-axis value for integrations
  double m_rangeLower;
  /// Ending x-axis value for integrations
  double m_rangeUpper;
  //@}
};

} // namespace Algorithms
} // namespace Mantid
