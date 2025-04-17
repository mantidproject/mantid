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
#include "MantidMuon/DllConfig.h"

namespace Mantid {

namespace Geometry {
class CompAssembly;
class Component;
class Instrument;
} // namespace Geometry

namespace Muon {
/** @class LoadInstrumentFromNexus LoadInstrumentFromNexus.h
Muon/LoadInstrumentFromNexus.h

Attempts to load information about the instrument from a ISIS NeXus file. In
particular attempt to
read L2 and 2-theta detector position values and add detectors which are
positioned relative
to the sample in spherical coordinates as (r,theta,phi)=(L2,2-theta,0.0). Also
adds dummy source
and samplepos components to instrument.

LoadInstrumentFromNexus is intended to be used as a child algorithm of
other Loadxxx algorithms, rather than being used directly.
It is used by LoadMuonNexus version 1.

LoadInstrumentFromNexus is an algorithm and as such inherits
from the Algorithm class, and overrides
the init() & exec()  methods.

Required Properties:
<UL>
<LI> Filename - The name of and path to the input NEXUS file </LI>
<LI> Workspace - The name of the workspace in which to use as a basis for any
data to be added.</LI>
</UL>

@author Anders Markvardsen, ISIS, RAL (LoadInstrumentFromRaw)
@date 2/5/2008
@author Ronald Fowler, ISIS, RAL (LoadInstrumentFromNexus)
*/
class MANTID_MUON_DLL LoadInstrumentFromNexus final : public API::Algorithm {
public:
  /// Default constructor
  LoadInstrumentFromNexus();

  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "LoadInstrumentFromNexus"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Attempts to load some information about the instrument from a "
           "Nexus file. It adds dummy source and samplepos components to "
           "instrument. If the L1 source - sample distance is not available in "
           "the file then it may be read from the mantid properties file using "
           "the key instrument.L1, as a final fallback a default distance of "
           "10m will be used.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override { return {"LoadInstrument", "Load"}; }

  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Muon\\DataHandling"; }

private:
  /// Overwrites Algorithm method. Does nothing at present
  void init() override;

  /// Overwrites Algorithm method
  void exec() override;

  /// The name and path of the input file
  std::string m_filename;
};

} // namespace Muon
} // namespace Mantid
