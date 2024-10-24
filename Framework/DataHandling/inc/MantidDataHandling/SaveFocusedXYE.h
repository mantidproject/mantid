// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidDataHandling/DllConfig.h"

namespace Mantid {
namespace DataHandling {
/**
     Saves a focused data set (usually output of a diffraction focusing routine
   but not exclusively)
     into a three column format containing X_i, Y_i, and E_i.
     For data where the focusing routine has generated several spectra (for
   example, multi-bank instruments),
     the option is provided for saving all spectra into a single file, separated
   by headers, or into
     several files that will be named "workspaceName_"+spectra_number.

     Required properties:
     <UL>
     <LI> InputWorkspace - The workspace name to save. </LI>
     <LI> Filename - The filename for output. </LI>
     </UL>

     Optional properties:
     <UL>
     <LI> SplitFiles    - Split into N files for workspace with N-spectra
   (default: true).</LI>
     <LI> Append        - Append to Filename, if it already exists (default:
   false).</LI>
     <LI> IncludeHeader - Include header (comment) lines in output (default:
   true).</LI>
     </UL>

     @author Laurent Chapon, ISIS Facility, Rutherford Appleton Laboratory
     @date 04/03/2009
  */
class MANTID_DATAHANDLING_DLL SaveFocusedXYE : public API::Algorithm {
public:
  enum HeaderType { XYE, MAUD, TOPAS };
  /// Algorithm's name
  const std::string name() const override { return "SaveFocusedXYE"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Saves a focused data set (usually the output of a diffraction "
           "focusing routine but not exclusively) into a three column format "
           "containing X_i, Y_i, and E_i.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"SaveFullprofResolution", "SaveAscii"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "Diffraction\\DataHandling;DataHandling\\Text"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
  /// Write the header information
  void writeHeaders(std::ostream &os, API::MatrixWorkspace_const_sptr &workspace) const;
  /// Write the header information in default "XYE" format
  void writeXYEHeaders(std::ostream &os, API::MatrixWorkspace_const_sptr &workspace) const;
  /// Write the header information in MAUD format
  void writeMAUDHeaders(std::ostream &os, API::MatrixWorkspace_const_sptr &workspace) const;
  /// Write spectra header
  void writeSpectraHeader(std::ostream &os, size_t index1, size_t index2, double flightPath, double tth,
                          const std::string &caption, const std::string &spectraAxisCaption,
                          const std::string &spectraAxisLabel, double observable);
  /// Write spectra XYE header
  void writeXYESpectraHeader(std::ostream &os, size_t index1, const std::string &caption,
                             const std::string &spectrumAxisCaption, const std::string &spectraAxisLabel,
                             double observable);
  /// Write spectra MAUD header
  void writeMAUDSpectraHeader(std::ostream &os, size_t index1, size_t index2, double flightPath, double tth,
                              const std::string &caption);
  /// sets non workspace properties for the algorithm
  void setOtherProperties(IAlgorithm *alg, const std::string &propertyName, const std::string &propertyValue,
                          int perioidNum) override;

  /// Header type
  HeaderType m_headerType{XYE};
  /// Comment character
  std::string m_comment;
};
} // namespace DataHandling
} // namespace Mantid
