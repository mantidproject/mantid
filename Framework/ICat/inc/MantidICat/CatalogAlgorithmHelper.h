// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/CatalogFactory.h"
#include "MantidAPI/ICatalog.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"

namespace Mantid {
namespace ICat {
class CatalogAlgorithmHelper {
public:
  /// Obtain the error message returned by the IDS.
  const std::string getIDSError(Poco::Net::HTTPResponse::HTTPStatus &HTTPStatus, std::istream &responseStream);
};
} // namespace ICat
} // namespace Mantid
