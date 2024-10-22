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
  This algorithm obtains the datasets for a given investigation record using the
  related ID.

  Required Properties:

  <UL>
   <LI> InvestigationId - The id of the investigation to display</LI>
   <LI> InputWorkspace -  Input workspace which saved last search</LI>
   <LI> OutputWorkspace - The putput workspace to store  </LI>
  </UL>

  @author Sofia Antony, ISIS Rutherford Appleton Laboratory
  @date 07/07/2010
*/
class MANTID_ICAT_DLL CatalogGetDataSets final : public API::Algorithm {
public:
  /// constructor for CatalogGetDataSets
  CatalogGetDataSets() : API::Algorithm() {}
  /// destructor for CatalogGetDataSets
  ~CatalogGetDataSets() override = default;
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "CatalogGetDataSets"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Obtains information of the datasets associated to a specific "
           "investigation.";
  }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"CatalogGetDataFiles", "CatalogDownloadDataFiles", "CatalogLogin"};
  }
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
