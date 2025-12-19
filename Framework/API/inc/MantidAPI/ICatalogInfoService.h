// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DllConfig.h"
#include "MantidAPI/ITableWorkspace_fwd.h"

namespace Mantid {
namespace API {
/**
 This class is responsible for interfacing with the Information Data Service
 (IDS)
 to upload and download files to and from the archives.

 @author Jay Rainey, ISIS Rutherford Appleton Laboratory
 @date 24/02/2010
 */
class MANTID_API_DLL ICatalogInfoService {
public:
  // Virtual destructor
  virtual ~ICatalogInfoService() = default;
  /// Obtain the datafile location string from the archives.
  virtual const std::string getFileLocation(const long long &) = 0;
  /// Obtain url to download a file from.
  virtual const std::string getDownloadURL(const long long &) = 0;
  /// Obtain the url to upload a file to.
  virtual const std::string getUploadURL(const std::string &, const std::string &, const std::string &) = 0;
  /// Obtains the investigations that the user can publish to and saves related
  /// information to a workspace.
  virtual ITableWorkspace_sptr getPublishInvestigations() = 0;
};

using ICatalogInfoService_sptr = std::shared_ptr<ICatalogInfoService>;
using ICatalogInfoService_const_sptr = std::shared_ptr<const ICatalogInfoService>;
} // namespace API
} // namespace Mantid
