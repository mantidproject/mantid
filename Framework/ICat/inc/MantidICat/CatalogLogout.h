// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidICat/DllConfig.h"

namespace Mantid {
namespace ICat {

/**
 CatalogLogout is responsible for logging out of a catalog based on session
 information provided by the user.
 If no session information is provided this algorithm will log out of all active
 catalogs.

  @author Sofia Antony, STFC Rutherford Appleton Laboratory
  @date 23/07/2010
  */
class MANTID_ICAT_DLL CatalogLogout : public API::Algorithm {
public:
  /// Constructor
  CatalogLogout() : API::Algorithm() {}
  /// Destructor
  ~CatalogLogout() override {}
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "CatalogLogout"; }
  /// Summary of algorithms purpose.
  const std::string summary() const override {
    return "Logs out all catalogs, or a specific catalog using the session "
           "information provided.";
  }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"CatalogLogin"}; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "DataHandling\\Catalog"; }

private:
  void init() override;
  void exec() override;
};
} // namespace ICat
} // namespace Mantid
