// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid {
namespace Algorithms {

/** UpdateScriptRepository : Check the MantidWeb, for updates of the
    ScriptRepository. It will execute the ScriptRepository::check4update.
    Pratically, it will checkout the state of the Central Repository, and
    after, it will download all the scripts marked as AutoUpdate.
*/
class MANTID_ALGORITHMS_DLL UpdateScriptRepository : public API::Algorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Update the local instance of ScriptRepository."; }

  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"DownloadInstrument"}; }
  const std::string category() const override;

private:
  void init() override;
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid
