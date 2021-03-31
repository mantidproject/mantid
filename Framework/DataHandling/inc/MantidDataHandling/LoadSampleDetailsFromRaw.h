// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//-----------------------------------------------------
// Includes
//-----------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace DataHandling {
/**
   An algorithm to extract the sample details from the SPB structure within a
   RAW file

   Required properties:
   <UL>
   <LI>InputWorkspace - The workspace to add information to</LI>
   <LI>Filename - The raw file to use to gather the information</LI>
   </UL>

   @author Martyn, Tessella plc
   @date 29/07/2009
*/
class DLLExport LoadSampleDetailsFromRaw : public Mantid::API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "LoadSampleDetailsFromRaw"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Loads the simple sample geometry that is defined within an ISIS "
           "raw file.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"LoadRaw"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "DataHandling\\Raw;Sample"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
};
} // namespace DataHandling
} // namespace Mantid
