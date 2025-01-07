// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidGeometry/Instrument_fwd.h"
#include <H5Cpp.h>

namespace Mantid {
namespace DataHandling {

/** SaveNXcanSAS : Saves a reduced workspace in the NXcanSAS format. Currently * only MatrixWorkspaces resulting from
 * 1D and 2D reductions are supported. */
class MANTID_DATAHANDLING_DLL SaveNXcanSASBase : public API::Algorithm {
protected:
  void initStandardProperties();
  void addStandardMetadata(Mantid::API::MatrixWorkspace_sptr &workspace, H5::Group &sasEntry,
                           Mantid::API::Progress *progress);
  void addData(H5::Group &data, const Mantid::API::MatrixWorkspace_sptr &workspace);
  H5::Group addSasEntry(H5::H5File &file, const Mantid::API::MatrixWorkspace_sptr &workspace,
                        const std::string &suffix);
};

std::string MANTID_DATAHANDLING_DLL makeCanSASRelaxedName(const std::string &input);
} // namespace DataHandling
} // namespace Mantid
