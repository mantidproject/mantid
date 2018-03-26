#ifndef DATAHANDING_LOADGSS_H_
#define DATAHANDING_LOADGSS_H_

//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidAPI/IFileLoader.h"

namespace Mantid {
namespace DataHandling {
/**
     Loads a file as saved by SaveGSS

     @author Michael Whitty, ISIS Facility, Rutherford Appleton Laboratory
     @date 01/09/2010

     Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
   National Laboratory & European Spallation Source

     This file is part of Mantid.

     Mantid is free software; you can redistribute it and/or modify
     it under the terms of the GNU General Public License as published by
     the Free Software Foundation; either version 3 of the License, or
     (at your option) any later version.

     Mantid is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with this program.  If not, see <http://www.gnu.org/licenses/>.

     File change history is stored at: <https://github.com/mantidproject/mantid>
     Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
class DLLExport LoadGSS : public API::IFileLoader<Kernel::FileDescriptor> {
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
  const std::vector<std::string> seeAlso() const override {
    return {"LoadAscii", "SaveGSS", "LoadMultipleGSS"};
  }

  /// Algorithm's category for identification
  const std::string category() const override {
    return "Diffraction\\DataHandling;DataHandling\\Text";
  }

  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::FileDescriptor &descriptor) const override;

private:
  /// Initialisation code
  void init() override;

  /// Execution code
  void exec() override;

  /// Main method to load GSAS
  API::MatrixWorkspace_sptr loadGSASFile(const std::string &filename,
                                         bool useBankAsSpectrum);

  /// Convert a string (value+unit) to double (value)
  double convertToDouble(std::string inputstring);

  /// Create an instrument geometry.
  void createInstrumentGeometry(API::MatrixWorkspace_sptr workspace,
                                const std::string &instrumentname,
                                const double &primaryflightpath,
                                const std::vector<int> &detectorids,
                                const std::vector<double> &totalflightpaths,
                                const std::vector<double> &twothetas);
};
}
}
#endif // DATAHANDING_LOADGSS_H_
