#ifndef MANTID_DATAHANDLING_CREATESAMPLESHAPE_H_
#define MANTID_DATAHANDLING_CREATESAMPLESHAPE_H_

//--------------------------------
// Includes
//--------------------------------
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace DataHandling {

/**
    This class allows the shape of the sample to be defined by using the allowed
   XML
    expressions

    @author Martyn Gigg, Tessella Support Services plc
    @date 13/03/2009

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport CreateSampleShape : public Mantid::API::Algorithm {
public:
  /// Algorithm's name
  const std::string name() const override { return "CreateSampleShape"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Create a shape object to model the sample.";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override {
    return {"AbsorptionCorrection", "SetSampleMaterial", "CopySample"};
  }
  /// Algorithm's category for identification
  const std::string category() const override { return "Sample;"; }
  /// Algorithm's aliases
  const std::string alias() const override { return "SetSampleShape"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
};
} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_CREATESAMPLESHAPE_H_*/
