#ifndef MANTID_DATAOBJECTS_FAKEMD_H_
#define MANTID_DATAOBJECTS_FAKEMD_H_
#include <vector>

#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidDataObjects/DllConfig.h"
#include "MantidDataObjects/MDEventWorkspace.h"

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/variate_generator.hpp>

namespace Mantid {
namespace DataObjects {

/**
    Provides a helper class to add fake data to an MD workspace


    Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
  boost::mt19937 m_randGen;
  boost::uniform_int<size_t> m_uniformDist;
};

} // namespace DataObjects
} // namespace Mantid

#endif /* MANTID_DATAOBJECTS_FAKEMD_H_ */
