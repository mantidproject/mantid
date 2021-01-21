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
This algorithm obtains all of the information for the investigations the logged
in user is an investigator of.

Required Properties:
<UL>
 <LI>  OutputWorkspace - name of the OutputWorkspace which contains my
investigations search
</UL>

@author Sofia Antony, ISIS Rutherford Appleton Laboratory
@date 04/08/2010

 */
class MANTID_ICAT_DLL CatalogMyDataSearch : public API::Algorithm {
public:
  /// constructor
  CatalogMyDataSearch() : API::Algorithm() {}
  /// destructor
  ~CatalogMyDataSearch() override {}
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "CatalogMyDataSearch"; }
  /// Summary of algorithms purpose.
  const std::string summary() const override {
    return "Obtains the user's investigations for all active catalogs and "
           "stores them into a workspace.";
  }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"CatalogSearch"}; }
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
