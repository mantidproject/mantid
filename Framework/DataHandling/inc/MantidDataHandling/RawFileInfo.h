// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//------------------------------------
// Includes
//------------------------------------
#include "MantidAPI/Algorithm.h"

//------------------------------------
// Forward declarations
//------------------------------------
class ISISRAW;

namespace Mantid {
namespace DataHandling {
/**
   An algorithm to extract pertinent information about a RAW file without
   loading the data.

   Required input properties:
   <UL>
   <LI> Filename - The raw file to use to gather the information </LI>
   <LI> GetRunParameters - Flag indicating whether to output run parameters
   (RPB_STRUCT) in a table (default false)</LI>
   </UL>

   Output properties:
   <UL>
   <LI> RunTitle         - The title of the run (r_title) </LI>
   <LI> RunHeader        - The run header (HDR_STRUCT) </LI>
   <LI> SpectraCount     - The number of spectra (t_nsp1) </LI>
   <LI> TimeChannelCount - The number of time channels (t_ntc1) </LI>
   <LI> PeriodCount      - The number of periods (t_nper) </LI>
   <LI> RunParameterTable (If requested by GetRunParameters flag above) <LI>
   </UL>

   @author Martyn, Tessella plc
   @date 29/07/2009
*/
class DLLExport RawFileInfo : public API::Algorithm {
public:
  static const std::string runTitle(const ISISRAW &isisRaw);
  static const std::string runHeader(const ISISRAW &isisRaw);
  /// Algorithm's name
  const std::string name() const override { return "RawFileInfo"; }
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Extract run parameters from a  RAW file as output properties."; }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"LoadRaw"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "DataHandling\\Raw"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
};
} // namespace DataHandling
} // namespace Mantid
