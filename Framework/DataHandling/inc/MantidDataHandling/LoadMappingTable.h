// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace DataHandling {
/** @class LoadMappingTable LoadMappingTable.h DataHandling/LoadMappingTable.h

Loads the mapping table between spectra and IDetector
from a raw file. It returns a SpectraDetectorTable which maintain a multimap.
The key of the multimap is the spectra number and the value
is the pointer to IDetector. The association is one to many, i.e. a spectrum can
have one or many
detectors contributing to it. Alternatively the same spectrum can contribute to
different spectra
(for example in DAE2 (Data Aquisition Electronic) when a spectra containing
electronically focussed data is created simultaneously
with individual spectra). LoadMappingTable is an algorithm and as such inherits
from the Algorithm class and overrides the init() & exec() methods.

Required Properties:
<UL>
<LI> Filename - The name of and path to the input RAW file </LI>
<LI> Workspace - The name of the workspace in which to store the imported data
</LI>
</UL>

@author Laurent Chapon, ISIS Rutherford Appleton Laboratory
@date 25/04/2008
*/
class DLLExport LoadMappingTable : public API::Algorithm {
public:
  /// Default constructor
  LoadMappingTable();
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "LoadMappingTable"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Builds up the mapping between spectrum number and the detector "
           "objects in the instrument Geometry.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; };
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "DataHandling\\Instrument;DataHandling\\Raw"; }

private:
  /// The name and path of the input file
  std::string m_filename;

  /// Overwrites Algorithm method.
  void init() override;

  /// Overwrites Algorithm method
  void exec() override;
};

} // namespace DataHandling
} // namespace Mantid
