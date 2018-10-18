// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_SNSDATAARCHIVE_H_
#define MANTID_DATAHANDLING_SNSDATAARCHIVE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IArchiveSearch.h"
#include "MantidKernel/System.h"
#include "MantidCatalog/ONCat.h"

#include <string>

using Mantid::Catalog::ONCat::ONCat_uptr;

namespace Mantid {
namespace DataHandling {
/**
 * Please see the .cpp file for more information.
 */

class DLLExport SNSDataArchive : public API::IArchiveSearch {
public:
  std::string
  getArchivePath(const std::set<std::string> &basenames,
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

#endif /* MANTID_DATAHANDLING_SNSDATAARCHIVE_H_ */
