// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_CREATEMODERATORMODEL_H_
#define MANTID_DATAHANDLING_CREATEMODERATORMODEL_H_

#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace DataHandling {
/**
  Defines an algorithm to create a moderator model object an attach it to the
  given workspace
 */
class DLLExport CreateModeratorModel : public API::Algorithm {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Creates the given moderator model and attaches it to the input "
           "workspace.";
  }

  int version() const override;
  const std::string category() const override;

private:
  void init() override;
  void exec() override;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_CREATEMODERATORMODEL_H_ */
