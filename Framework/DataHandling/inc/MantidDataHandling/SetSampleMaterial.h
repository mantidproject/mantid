// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_SetSampleMaterial_H_
#define MANTID_DATAHANDLING_SetSampleMaterial_H_

#include "MantidAPI/DistributedAlgorithm.h"
#include "MantidDataHandling/ReadMaterial.h"
#include "MantidKernel/NeutronAtom.h"
namespace Mantid {
namespace DataHandling {

/**
    This class allows the shape of the sample to be defined by using the allowed
   XML
    expressions

    @author Vickie Lynch, SNS
    @date 2/7/2013
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
    return {"SetSample", "AbsorptionCorrection", "CreateSampleShape",
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

  ReadMaterial::MaterialParameters params;
};
} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_SetSampleMaterial_H_*/
