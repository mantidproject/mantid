// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <set>
#include <vector>
#ifndef Q_MOC_RUN
#include <unordered_map>
#endif

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidGeometry/IDTypes.h"

namespace Mantid {
namespace API {

/** A minimal class to hold the mapping between the spectrum number and its
    related detector ID numbers for a dataset.
    Normally, this mapping is contained within the collection of ISpectrum
    objects held by the workspace. This class can be useful when you want to
    pass just this information and not the entire workspace, or you want to
    store some mapping that related to spectra that are not yet, or no longer,
    contained in the workspace.
*/
class MANTID_API_DLL SpectrumDetectorMapping {
  using sdmap = std::unordered_map<specnum_t, std::set<detid_t>>;

public:
  explicit SpectrumDetectorMapping(const MatrixWorkspace_const_sptr &workspace, const bool useSpecNoIndex = true);
  SpectrumDetectorMapping(const std::vector<specnum_t> &spectrumNumbers, const std::vector<detid_t> &detectorIDs,
                          const std::vector<detid_t> &ignoreDetIDs = std::vector<detid_t>());
  SpectrumDetectorMapping(const specnum_t *const spectrumNumbers, const detid_t *const detectorIDs,
                          size_t arrayLengths);
  SpectrumDetectorMapping();
  virtual ~SpectrumDetectorMapping() = default;

  std::set<specnum_t> getSpectrumNumbers() const;
  const std::set<detid_t> &getDetectorIDsForSpectrumNo(const specnum_t spectrumNo) const;
  const std::set<detid_t> &getDetectorIDsForSpectrumIndex(const size_t spectrumIndex) const;
  const sdmap &getMapping() const;
  bool indexIsSpecNumber() const;

private:
  void fillMapFromArray(const specnum_t *const spectrumNumbers, const detid_t *const detectorIDs,
                        const size_t arrayLengths);
  void fillMapFromVector(const std::vector<specnum_t> &spectrumNumbers, const std::vector<detid_t> &detectorIDs,
                         const std::vector<detid_t> &ignoreDetIDs);

  bool m_indexIsSpecNo;
  /// The mapping of a spectrum number to zero or more detector IDs
  sdmap m_mapping;
};

} // namespace API
} // namespace Mantid
