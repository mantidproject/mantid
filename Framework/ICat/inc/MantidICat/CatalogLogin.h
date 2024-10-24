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
 This class is responsible for authentication of credentials against the
 catalog.

       Required Properties:
 <UL>
  <LI> Username - The logged in user name </LI>
  <LI> Password - The password of the logged in user </LI>
  <LI> Facility name - The name of the facility to log in to </LI>
 </UL>

 @author Sofia Antony, ISIS Rutherford Appleton Laboratory
 @date 07/07/2010
 */
class MANTID_ICAT_DLL CatalogLogin final : public API::Algorithm {
public:
  /// constructor
  CatalogLogin() : API::Algorithm() {}
  /// Destructor
  ~CatalogLogin() override = default;
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "CatalogLogin"; }
  /// Summary of algorithms purpose.
  const std::string summary() const override { return "Authenticates credentials against a given catalog."; }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"CatalogLogout", "CatalogSearch", "CatalogPublish"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "DataHandling\\Catalog"; }

private:
  /// Overwrites Algorithm method.
  void init() override;
  /// Overwrites Algorithm method
  void exec() override;
};
} // namespace ICat
} // namespace Mantid
