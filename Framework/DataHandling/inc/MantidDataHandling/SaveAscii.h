// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_SAVEASCII_H_
#define MANTID_DATAHANDLING_SAVEASCII_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace DataHandling {
/** @class SaveAscii SaveAscii.h DataHandling/SaveAscii.h

Saves a workspace or selected spectra in a coma-separated ascii file. Spectra
are saved in columns.

@author Roman Tolchenov, Tessella plc
@date 3/07/09
*/
class DLLExport SaveAscii : public API::Algorithm {
public:
  /// Default constructor
  SaveAscii();
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "SaveAscii"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Saves a 2D workspace to a ascii file.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "DataHandling\\Text"; }

private:
  /// Overwrites Algorithm method.
  void init() override;
  /// Overwrites Algorithm method
  void exec() override;

  /// Map the separator options to their string equivalents
  std::map<std::string, std::string> m_separatorIndex;
};

} // namespace DataHandling
} // namespace Mantid

#endif /*  MANTID_DATAHANDLING_SAVEASCII_H_  */
