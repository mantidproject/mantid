// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IArchiveSearch.h"
#include "MantidCatalog/ONCat.h"
#include "MantidDataHandling/DllConfig.h"

#include <string>

using Mantid::Catalog::ONCat::ONCat_uptr;

namespace Mantid {
namespace DataHandling {
/**
 * Please see the .cpp file for more information.
 */

class MANTID_DATAHANDLING_DLL ORNLDataArchive : public API::IArchiveSearch {
public:
  const API::Result<std::string> getArchivePath(const std::set<std::string> &basenames,
                                                const std::vector<std::string> &suffixes) const override;

  //////////////////////////////////////////////////////////////////////
  // Exposed publicly for testing purposes only.
  //////////////////////////////////////////////////////////////////////
  void setONCat(ONCat_uptr oncat);
  //////////////////////////////////////////////////////////////////////

private:
  ONCat_uptr m_oncat = nullptr;
};
} // namespace DataHandling
} // namespace Mantid
