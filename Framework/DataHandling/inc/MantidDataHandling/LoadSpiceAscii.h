// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_LOADSPICEASCII_H_
#define MANTID_DATAHANDLING_LOADSPICEASCII_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidKernel/System.h"

#include "MantidAPI/IFileLoader.h"

namespace Mantid {
namespace DataHandling {

/** LoadSpiceAscii : TODO: DESCRIPTION
 */
class DLLExport LoadSpiceAscii : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"LoadSpice2D", "LoadSpiceXML2DDet"};
  }
  const std::string category() const override;
  const std::string summary() const override;

private:
  void init() override;
  void exec() override;

  bool validateLogNamesType(const std::vector<std::string> &floatlognames,
                            const std::vector<std::string> &intlognames,
                            const std::vector<std::string> &strlognames);

  /// Parse SPICE Ascii file to dictionary
  void parseSPICEAscii(const std::string &filename,
                       std::vector<std::vector<std::string>> &datalist,
                       std::vector<std::string> &titles,
                       std::map<std::string, std::string> &runinfodict);

  /// Create data workspace
  API::ITableWorkspace_sptr
  createDataWS(const std::vector<std::vector<std::string>> &datalist,
               const std::vector<std::string> &titles);

  /// Create run information workspace
  API::MatrixWorkspace_sptr
  createRunInfoWS(std::map<std::string, std::string> runinfodict,
                  std::vector<std::string> &floatlognamelist,
                  std::vector<std::string> &intlognamelist,
                  std::vector<std::string> &strlognamelist,
                  bool ignoreunlisted);

  /// Convert input date string to mantid date string
  std::string processDateString(const std::string &rawdate,
                                const std::string &dateformat);

  /// Convert input time string to mantid time string
  std::string processTimeString(const std::string &rawtime,
                                const std::string &timeformat);

  /// Set up run start time
  void setupRunStartTime(API::MatrixWorkspace_sptr runinfows,
                         const std::vector<std::string> &datetimeprop);

  /// Add property to workspace
  template <typename T>
  void addProperty(API::MatrixWorkspace_sptr ws, const std::string &pname,
                   T pvalue);
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_LOADSPICEASCII_H_ */
