#ifndef MANTID_DATAHANDLING_SetSampleMaterial_H_
#define MANTID_DATAHANDLING_SetSampleMaterial_H_

#include "MantidAPI/DistributedAlgorithm.h"
#include "MantidKernel/NeutronAtom.h"

namespace Mantid {
namespace DataHandling {

/**
    This class allows the shape of the sample to be defined by using the allowed
   XML
    expressions

    @author Vickie Lynch, SNS
    @date 2/7/2013

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
class DLLExport SetSampleMaterial : public Mantid::API::DistributedAlgorithm {
public:
  /// Algorithm's name
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Sets the neutrons information in the sample.";
  }

  /// Algorithm's version
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"AbsorptionCorrection", "CreateSampleShape",
            "CalculateSampleTransmission"};
  }
  /// Algorithm's category for identification
  const std::string category() const override;
  std::map<std::string, std::string> validateInputs() override;

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;
  /// Print out the list of information for the material
  void fixNeutron(PhysicalConstants::NeutronAtom &neutron, double coh_xs,
                  double inc_xs, double abs_xs, double tot_xs);
};
}
}

#endif /* MANTID_DATAHANDLING_SetSampleMaterial_H_*/
