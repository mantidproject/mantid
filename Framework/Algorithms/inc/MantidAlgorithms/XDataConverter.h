// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DistributedAlgorithm.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidKernel/cow_ptr.h"

namespace Mantid {
namespace HistogramData {
class HistogramDx;
class HistogramX;
} // namespace HistogramData
namespace Algorithms {
/**
  This is an abstract base class for sharing methods between algorithms that
  operate only
  on X data. Inheriting classes should overide the isRequired,
  checkInputWorkspace, getNewXSize and
  setXData methods to return the appropriate values.

  @author Martyn Gigg, Tessella plc
  @date 2010-12-14
*/
class MANTID_ALGORITHMS_DLL XDataConverter : public API::DistributedAlgorithm {
public:
  /// Default constructor
  XDataConverter();
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "General"; }

protected:
  /// Returns true if the algorithm needs to be run.
  virtual bool isProcessingRequired(const API::MatrixWorkspace_sptr inputWS) const = 0;
  /// Returns the size of the new X vector
  virtual std::size_t getNewXSize(const std::size_t ySize) const = 0;
  /// Calculate the X point values. Implement in an inheriting class.
  virtual Kernel::cow_ptr<HistogramData::HistogramX>
  calculateXPoints(const Kernel::cow_ptr<HistogramData::HistogramX> inputX) const = 0;

private:
  /// Override init
  void init() override;
  /// Override exec
  void exec() override;

  std::size_t getNewYSize(const API::MatrixWorkspace_sptr &inputWS);

  /// Set the X data on given spectra
  void setXData(const API::MatrixWorkspace_sptr &outputWS, const API::MatrixWorkspace_sptr &inputWS, const int index);

  /// Flag if the X data is shared
  bool m_sharedX;
  /// Cached data for shared X values
  Kernel::cow_ptr<HistogramData::HistogramX> m_cachedX{nullptr};
};

} // namespace Algorithms
} // namespace Mantid
