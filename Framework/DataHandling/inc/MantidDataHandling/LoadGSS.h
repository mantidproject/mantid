// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidAPI/IFileLoader.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidKernel/FileDescriptor.h"

namespace Mantid {
namespace DataHandling {
/**
     Loads a file as saved by SaveGSS

     @author Michael Whitty, ISIS Facility, Rutherford Appleton Laboratory
     @date 01/09/2010
  */
class MANTID_DATAHANDLING_DLL LoadGSS : public API::IFileLoader<Kernel::FileDescriptor> {
public:
  /// Algorithm's name
  const std::string name() const override { return "LoadGSS"; }

  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Loads a GSS file such as that saved by SaveGSS. This is not a "
           "lossless process, as SaveGSS truncates some data. There is no "
           "instrument assosciated with the resulting workspace.  'Please "
           "Note': Due to limitations of the GSS file format, the process of "
           "going from Mantid to a GSS file and back is not perfect.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"LoadAscii", "SaveGSS", "LoadMultipleGSS"}; }

  /// Algorithm's category for identification
  const std::string category() const override { return "Diffraction\\DataHandling;DataHandling\\Text"; }

  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::FileDescriptor &descriptor) const override;

private:
  /// Initialisation code
  void init() override;

  /// Execution code
  void exec() override;

  /// Main method to load GSAS
  API::MatrixWorkspace_sptr loadGSASFile(const std::string &filename, bool useBankAsSpectrum);

  /// Convert a string (value+unit) to double (value)
  double convertToDouble(const std::string &inputstring);

  /// Create an instrument geometry.
  void createInstrumentGeometry(const API::MatrixWorkspace_sptr &workspace, const std::string &instrumentname,
                                const double &primaryflightpath, const std::vector<int> &detectorids,
                                const std::vector<double> &totalflightpaths, const std::vector<double> &twothetas,
                                const std::vector<double> &difcs);
};
} // namespace DataHandling
} // namespace Mantid
