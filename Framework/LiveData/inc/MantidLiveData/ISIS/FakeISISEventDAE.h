#ifndef MANTID_LIVEDATA_FAKEISISEVENTDAE_H_
#define MANTID_LIVEDATA_FAKEISISEVENTDAE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace LiveData {
/**
    Simulates ISIS histogram DAE. It runs continuously until canceled and
    listens to port 6789 for ISIS DAE commands.

    Copyright &copy; 2008-9 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class FakeISISEventDAE : public API::Algorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "FakeISISEventDAE"; }
  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override {
    return "DataHandling\\DataAcquisition";
  }
  const std::vector<std::string> seeAlso() const override {
    return {"FakeISISHistoDAE"};
  }

  /// Algorithm's summary
  const std::string summary() const override {
    return "Simulates ISIS event DAE.";
  }

private:
  void init() override;
  void exec() override;
};

} // namespace LiveData
} // namespace Mantid

#endif /*MANTID_LIVEDATA_FAKEEventSHISTODAE_H_*/
