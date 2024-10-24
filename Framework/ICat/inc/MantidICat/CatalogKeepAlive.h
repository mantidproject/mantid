// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidICat/DllConfig.h"

namespace Mantid {
namespace ICat {
/**
 CatalogKeepAlive is responsible for keeping a catalog alive based on the
 session information.

 Required Properties:

 <UL>
  <LI> Session - Session information used to obtain the specific catalog to keep
 alive.</LI>
 </UL>

 @author Jay Rainey, ISIS Rutherford Appleton Laboratory
 @date 17/03/2014
*/
class MANTID_ICAT_DLL CatalogKeepAlive final : public API::Algorithm {
public:
  /// constructor
  CatalogKeepAlive() : API::Algorithm() {}
  /// Algorithm's name for identification.
  const std::string name() const override { return "CatalogKeepAlive"; }
  /// Summary of algorithms purpose.
  const std::string summary() const override {
    return "Refreshes the current session to the maximum amount provided by "
           "the catalog API.";
  }
  /// Algorithm's version for identification.
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"CatalogLogin"}; }
  /// Algorithm's category for identification.
  const std::string category() const override { return "DataHandling\\Catalog"; }

private:
  /// Override algorithm initialisation method.
  void init() override;
  /// Override algorithm execute method.
  void exec() override;
};
} // namespace ICat
} // namespace Mantid
