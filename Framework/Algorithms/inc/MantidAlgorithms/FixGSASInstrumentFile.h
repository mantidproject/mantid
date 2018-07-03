#ifndef MANTID_ALGORITHMS_FIXGSASINSTRUMENTFILE_H_
#define MANTID_ALGORITHMS_FIXGSASINSTRUMENTFILE_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {

/** FixGSASInstrumentFile : TODO: DESCRIPTION

  Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport FixGSASInstrumentFile : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "FixGSASInstrumentFile"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Fix format error in an GSAS instrument file.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"LoadGSASInstrumentFile"};
  }

  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override {
    return "Diffraction\\DataHandling";
  }

private:
  /// Implement abstract Algorithm methods
  void init() override;
  /// Implement abstract Algorithm methods
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_FIXGSASINSTRUMENTFILE_H_ */
