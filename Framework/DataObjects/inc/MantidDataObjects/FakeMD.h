// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAOBJECTS_FAKEMD_H_
#define MANTID_DATAOBJECTS_FAKEMD_H_

#include <vector>

#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidDataObjects/DllConfig.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidKernel/uniform_int_distribution.h"

namespace Mantid {
namespace DataObjects {

/**
    Provides a helper class to add fake data to an MD workspace
  */
class MANTID_DATAOBJECTS_DLL FakeMD {
public:
  FakeMD(const std::vector<double> &uniformParams,
         const std::vector<double> &peakParams, const int randomSeed,
         const bool randomizeSignal);

  void fill(API::IMDEventWorkspace_sptr workspace);

private:
  void setupDetectorCache(const API::IMDEventWorkspace &workspace);

  template <typename MDE, size_t nd>
  void addFakePeak(typename MDEventWorkspace<MDE, nd>::sptr ws);
  template <typename MDE, size_t nd>
  void addFakeUniformData(typename MDEventWorkspace<MDE, nd>::sptr ws);

  template <typename MDE, size_t nd>
  void addFakeRandomData(const std::vector<double> &params,
                         typename MDEventWorkspace<MDE, nd>::sptr ws);
  template <typename MDE, size_t nd>
  void addFakeRegularData(const std::vector<double> &params,
                          typename MDEventWorkspace<MDE, nd>::sptr ws);

  detid_t pickDetectorID();

  //------------------ Member variables ------------------------------------
  std::vector<double> m_uniformParams;
  std::vector<double> m_peakParams;
  const int m_randomSeed;
  const bool m_randomizeSignal;
  mutable std::vector<detid_t> m_detIDs;
  std::mt19937 m_randGen;
  Kernel::uniform_int_distribution<size_t> m_uniformDist;
};

} // namespace DataObjects
} // namespace Mantid

#endif /* MANTID_DATAOBJECTS_FAKEMD_H_ */
