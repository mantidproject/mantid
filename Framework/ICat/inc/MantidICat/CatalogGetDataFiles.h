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
 CatalogGetDataFiles obtains a list of datafiles and related information for an
investigation.

Required Properties:

<UL>
 <LI> InvestigationId - The id of the investigation to use for searching.</LI>
 <LI> OutputWorkspace - The workspace to store the datafile information.</LI>
</UL>

@author Sofia Antony, ISIS Rutherford Appleton Laboratory
@date 07/07/2010

 */
class MANTID_ICAT_DLL CatalogGetDataFiles : public API::Algorithm {
public:
  /// Constructor
  CatalogGetDataFiles() : API::Algorithm() {}
  /// Destructor
  ~CatalogGetDataFiles() override {}
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "CatalogGetDataFiles"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Obtains information of the datafiles associated to a specific "
           "investigation.";
  }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"CatalogDownloadDataFiles", "CatalogGetDataSets", "CatalogLogin", "ISISJournalGetExperimentRuns"};
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
