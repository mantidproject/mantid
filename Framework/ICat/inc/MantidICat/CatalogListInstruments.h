// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidICat/DllConfig.h"

namespace Mantid {
namespace ICat {
/**
  This algorithm obtains a list of instruments types from the catalog.

  @author Sofia Antony, STFC Rutherford Appleton Laboratory
  @date 09/07/2010
*/
class MANTID_ICAT_DLL CatalogListInstruments final : public API::Algorithm {
public:
  /// constructor
  CatalogListInstruments() : API::Algorithm() {}
  /// destructor
  ~CatalogListInstruments() override = default;
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "CatalogListInstruments"; }
  /// Summary of algorithms purpose.
  const std::string summary() const override {
    return "Lists the name of instruments from all catalogs or a specific "
           "catalog based on session information.";
  }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"CatalogListInvestigationTypes"}; }
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
