// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_IARCHIVESEARCH_H_
#define MANTID_API_IARCHIVESEARCH_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"

#ifndef Q_MOC_RUN
#include <boost/shared_ptr.hpp>
#endif
#include <set>
#include <string>
#include <vector>

#define DECLARE_ARCHIVESEARCH(classname, facility)                             \
  namespace {                                                                  \
  Mantid::Kernel::RegistrationHelper register_archive_##facility(              \
      ((Mantid::API::ArchiveSearchFactory::Instance().subscribe<classname>(    \
           #facility)),                                                        \
       0));                                                                    \
  }

namespace Mantid {
namespace API {

/**
This class is an archive searching interface.

@author Roman Tolchenov, Tessella plc
@date 27/07/2010
*/
class MANTID_API_DLL IArchiveSearch {
public:
  /// Virtual destructor
  virtual ~IArchiveSearch() = default;
  /**
   * Return the full path to a data file in an archive. The first match is
   * returned
   * @param filenames :: A list of filenames (without extensions) to pass to
   * the archive
   * @param exts :: A list of extensions to check for in turn against each file
   */
  virtual std::string
  getArchivePath(const std::set<std::string> &filenames,
                 const std::vector<std::string> &exts) const = 0;
};

/// Typedef for a shared pointer to an IArchiveSearch
using IArchiveSearch_sptr = boost::shared_ptr<IArchiveSearch>;
} // namespace API
} // namespace Mantid

#endif // MANTID_API_IARCHIVESEARCH_H_
