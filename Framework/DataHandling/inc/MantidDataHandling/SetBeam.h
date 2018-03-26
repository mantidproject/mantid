#ifndef MANTID_DATAHANDLING_SETBEAM_H_
#define MANTID_DATAHANDLING_SETBEAM_H_

#include "MantidDataHandling/DllConfig.h"
#include "MantidAPI/Algorithm.h"
namespace Mantid {
namespace DataHandling {

/**
  Set properties of the beam.

  Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_DATAHANDLING_DLL SetBeam final : public API::Algorithm {
public:
  const std::string name() const override final;
  int version() const override final;
  const std::vector<std::string> seeAlso() const override {
    return {"SetSample"};
  }
  const std::string category() const override final;
  const std::string summary() const override final;

private:
  std::map<std::string, std::string> validateInputs() override final;
  void init() override final;
  void exec() override final;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_SETBEAM_H_ */
