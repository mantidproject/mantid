// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_UPDATESCRIPTREPOSITORY_H_
#define MANTID_ALGORITHMS_UPDATESCRIPTREPOSITORY_H_

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Algorithms {

/** UpdateScriptRepository : Check the MantidWeb, for updates of the
    ScriptRepository. It will execute the ScriptRepository::check4update.
    Pratically, it will checkout the state of the Central Repository, and
    after, it will download all the scripts marked as AutoUpdate.
*/
class DLLExport UpdateScriptRepository : public API::Algorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Update the local instance of ScriptRepository.";
  }

  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"DownloadInstrument"};
  }
  const std::string category() const override;

private:
  void init() override;
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_UPDATESCRIPTREPOSITORY_H_ */
