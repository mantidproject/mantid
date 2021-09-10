// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidKernel/cow_ptr.h"

#include <map>

namespace Mantid {

namespace Algorithms {

/** ConvertAxesToRealSpace : Converts the spectrum and TOF axes to real space
  values, integrating the data in the process
*/
class MANTID_ALGORITHMS_DLL ConvertAxesToRealSpace : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"ConvertSpectrumAxis", "ConvertUnits"}; }
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;

  /// Local cache data about a spectra
  struct SpectraData {
    double verticalValue;
    double horizontalValue;
    double intensity;
    double error;
    int verticalIndex;
    int horizontalIndex;
  };

  /// summary data about an Axis
  struct AxisData {
    std::string label;
    double min;
    double max;
    int bins;
  };

  // map to store unit captions and measures
  std::map<std::string, std::string> m_unitMap;

  void fillAxisValues(MantidVec &vector, const AxisData &axisData, bool isHistogram);
  void fillUnitMap(std::vector<std::string> &orderedVector, std::map<std::string, std::string> &unitMap,
                   const std::string &caption, const std::string &unit);
};

} // namespace Algorithms
} // namespace Mantid
