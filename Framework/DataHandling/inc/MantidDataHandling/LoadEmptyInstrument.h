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
#include "MantidAPI/IFileLoader.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/FileDescriptor.h"

namespace Mantid {
namespace DataHandling {
/**
Loads an instrument definition file into a workspace, with the purpose of being
able to visualise an instrument without requiring to read in a ISIS raw datafile
first.
The name of the algorithm refers to the fact that an instrument
is loaded into a workspace but without any real data - hence the reason for
referring to
it as an 'empty' instrument.

Required Properties:
<UL>
<LI> Filename - The name of an instrument definition file </LI>
<LI> OutputWorkspace - The name of the workspace in which to store the imported
instrument</LI>
</UL>

Optional Properties: (note that these options are not available if reading a
multiperiod file)
<UL>
<LI> detector_value  - This value affect the colour of the detectorss in the
instrument display window</LI>
<LI> monitor_value  - This value affect the colour of the monitors in the
instrument display window</LI>
</UL>

@author Anders Markvardsen, ISIS, RAL
@date 31/10/2008
*/
class MANTID_DATAHANDLING_DLL LoadEmptyInstrument : public API::IFileLoader<Kernel::FileDescriptor> {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "LoadEmptyInstrument"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Loads an Instrument Definition File (IDF) into a workspace rather "
           "than a data file.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override { return {"LoadInstrument"}; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "DataHandling\\Instrument"; }
  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::FileDescriptor &descriptor) const override;
  static std::string retrieveValidInstrumentFilenameExtension(const std::string &filename);
  static std::vector<std::string> getValidInstrumentFilenameExtensions();

private:
  /// Overwrites Algorithm method.
  void init() override;
  /// Overwrites Algorithm method
  void exec() override;

  API::MatrixWorkspace_sptr runLoadInstrument(const std::string &filename, const std::string &instrumentname);
  API::MatrixWorkspace_sptr runLoadIDFFromNexus(const std::string &filename);
};

} // namespace DataHandling
} // namespace Mantid
