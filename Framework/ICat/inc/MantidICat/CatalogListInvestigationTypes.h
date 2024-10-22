// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidICat/DllConfig.h"

namespace Mantid {
namespace ICat {

/**
 This algorithm obtains a list of investigation types from the catalog.

 @author Sofia Antony, STFC Rutherford Appleton Laboratory
 @date 12/08/2010
 */
class MANTID_ICAT_DLL CatalogListInvestigationTypes final : public API::Algorithm {
public:
  /// constructor
  CatalogListInvestigationTypes() : API::Algorithm() {}
  /// destructor
  ~CatalogListInvestigationTypes() override = default;
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "CatalogListInvestigationTypes"; }
  /// Summary of algorithms purpose.
  const std::string summary() const override { return "Obtains a list of investigation types for active catalogs."; }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"CatalogListInstruments"}; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "DataHandling\\Catalog"; }

private:
  /// Overwrites Algorithm init method.
  void init() override;
  /// Overwrites Algorithm exec method
  void exec() override;
};
} // namespace ICat
} // namespace Mantid
