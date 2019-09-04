// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_CALCULATEPLACZEKSELFSCATTERING_H_
#define MANTID_ALGORITHMS_CALCULATEPLACZEKSELFSCATTERING_H_

#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Algorithms {

/** CalculatePlaczekSelfScattering : This algorithm calculates a correction for an 
	incident spectrum defracted by a sample.
*/
class CalculatePlaczekSelfScattering: public API::Algorithm 
{
  public:
    CalculatePlaczekSelfScattering() : API::Algorithm() {}
    virtual ~CalculatePlaczekSelfScattering() {}
    virtual const std::string name() const { return "CalculatePlaczekSelfScattering"; }
    virtual int version() const { return (1); }
    const std::vector<std::string> seeAlso() const override { return {""}; }
    const std::string category() const override { return "CorrectionFunctions"; };
    const std::string summary() const override {
      return "Calculates the Placzek self scattering correction";
    };

  private:
    void init() override;
    void exec() override;
    std::map<std::string, std::map<std::string, double>>
    get_sample_species_info(API::MatrixWorkspace_sptr ws);
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_CALCULATEPLACZEKSELFSCATTERING_H_ */