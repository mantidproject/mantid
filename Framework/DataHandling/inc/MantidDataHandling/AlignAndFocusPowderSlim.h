// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidDataHandling/DllConfig.h"

namespace NeXus {
class File;
}

namespace Mantid {
namespace DataHandling {

/** AlignAndFocusPowderSlim : TODO: DESCRIPTION
 */
class MANTID_DATAHANDLING_DLL AlignAndFocusPowderSlim : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;
  const std::vector<std::string> seeAlso() const override;

private:
  void init() override;
  void exec() override;

  void loadTOF(std::unique_ptr<std::vector<float>> &data, ::NeXus::File &h5file);
  void loadDetid(std::unique_ptr<std::vector<uint32_t>> &data, ::NeXus::File &h5file);
};

} // namespace DataHandling
} // namespace Mantid
