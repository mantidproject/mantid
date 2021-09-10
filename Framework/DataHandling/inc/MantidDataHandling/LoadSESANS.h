// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IFileLoader.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidKernel/FileDescriptor.h"

#include <unordered_map>

using Column = std::vector<double>;
using ColumnMap = std::unordered_map<std::string, Column>;
using AttributeMap = std::unordered_map<std::string, std::string>;

namespace Mantid {
namespace DataHandling {

/** LoadSESANS : Load a workspace in the SESANS file format

        Required properties:
        <UL>
        <LI> Filename - The path to the file</LI>
        <LI> OutputWorkspace - The name of the output workspace</LI>
        </UL>

        @author Joseph Ramsay, ISIS
        @date 20/07/2017
*/
class MANTID_DATAHANDLING_DLL LoadSESANS : public API::IFileLoader<Kernel::FileDescriptor> {
public:
  const std::string name() const override;
  const std::string summary() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"SaveSESANS"}; }
  const std::string category() const override;
  int confidence(Kernel::FileDescriptor &descriptor) const override;

private:
  // Private constants
  const std::string m_spinEchoLength = "SpinEchoLength";
  const std::string m_wavelength = "Wavelength";
  const std::string m_depolarisation = "Depolarisation";
  const std::string m_depolarisationError = "Depolarisation_error";
  const std::string m_beginData = "BEGIN_DATA";

  const std::vector<std::string> m_mandatoryAttributes{
      "FileFormatVersion",   "DataFileTitle",       "Sample",         "Thickness",       "Thickness_unit",
      "Theta_zmax",          "Theta_zmax_unit",     "Theta_ymax",     "Theta_ymax_unit", "Orientation",
      "SpinEchoLength_unit", "Depolarisation_unit", "Wavelength_unit"};
  const std::vector<std::string> m_mandatoryColumnHeaders{m_spinEchoLength, m_wavelength, m_depolarisation,
                                                          m_depolarisationError};
  const std::vector<std::string> m_fileExtensions{".ses", ".SES", ".sesans", ".SESANS"};

  // Private functions
  void init() override;
  void exec() override;

  AttributeMap consumeHeaders(std::ifstream &infile, std::string &line, int &lineNum);
  ColumnMap consumeData(std::ifstream &infile, std::string &line, int &lineNum);
  std::pair<std::string, std::string> splitHeader(const std::string &line, const int &lineNum);

  void checkMandatoryHeaders(const AttributeMap &attributes);

  void throwFormatError(const std::string &line, const std::string &message, const int &lineNum);

  API::MatrixWorkspace_sptr makeWorkspace(ColumnMap columns);
};

} // namespace DataHandling
} // namespace Mantid
