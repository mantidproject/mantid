// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidICat/CatalogSearchParam.h"
#include "MantidICat/DllConfig.h"

namespace Mantid {
namespace ICat {

/**
  This class is responsible for searching the catalog using the properties
  specified.

  Required Properties:
  <UL>
    <LI> Investigation name - The name of the investigation to search </LI>
    <LI> Instrument - The instrument to use in the search </LI>
    <LI> Run range - The range of run numbers to search between </LI>
    <LI> Start date - The start date used for search </LI>
    <LI> End date - The end date used for search </LI>
    <LI> Keywords - The keywords used for search </LI>
    <LI> Investigation id - The id of an investigation to search for </LI>
    <LI> Investigators name - Search for all investigations this investigator is
  in </LI>
    <LI> Sample - The name of the sample used in an investigation <LI>
    <LI> Investigation abstract - The abstract of the investigation to be
  searched <LI>
    <LI> Investigation type - The type of the investigation to search for <LI>
    <LI> My data - Search through the investigations you are part of <LI>
  </UL>

  @author Sofia Antony, ISIS Rutherford Appleton Laboratory
  @date 04/11/2013
 */
class MANTID_ICAT_DLL CatalogSearch : public API::Algorithm {
public:
  /// constructor
  CatalogSearch() : API::Algorithm() {}
  /// destructor
  ~CatalogSearch() override {}
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "CatalogSearch"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Searches all active catalogs using the provided input parameters.";
  }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"CatalogMyDataSearch", "CatalogGetDataFiles", "CatalogLogin", "CatalogPublish"};
  }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "DataHandling\\Catalog"; }

private:
  /// Overwrites Algorithm init method.
  void init() override;
  /// Overwrites Algorithm exec method
  void exec() override;
  /// Get all inputs for the algorithm
  void getInputProperties(CatalogSearchParam &params);
  /// Parse the run-range input field, split it into start and end run, and set
  /// related parameters.
  void setRunRanges(std::string &runRange, CatalogSearchParam &params);
};
} // namespace ICat
} // namespace Mantid
