// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidDataHandling/SaveNexusProcessed.h"

namespace Mantid {
namespace DataHandling {

/** SaveNexusESS : Save algorithm to save a NeXus organised hdf5 file containing
 * data and geometry from reduced experiment for use at European Spallation
 * Source.
 *
 * Uses Template Method pattern to reuse as much as possible from base
 * SaveNexusProcessed.
 */
class MANTID_DATAHANDLING_DLL SaveNexusESS : public Mantid::DataHandling::SaveNexusProcessed {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;

protected:
  bool processGroups() override;

private:
  void saveNexusGeometry(const Mantid::API::MatrixWorkspace &ws, const std::string &filename,
                         std::optional<size_t> entryNumber = std::optional<size_t>());

  virtual bool saveLegacyInstrument() override;

  void init() override;

  void exec() override;
};

} // namespace DataHandling
} // namespace Mantid
