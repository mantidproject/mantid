// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CRYSTAL_SAVEHKL_H_
#define MANTID_CRYSTAL_SAVEHKL_H_

#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace Crystal {

/** Save a PeaksWorkspace to a Gsas-style ASCII .hkl file.
 *
 * @author Vickie Lynch, SNS
 * @date 2011-09-28
 */

class DLLExport SaveHKL : public API::Algorithm {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "SaveHKL"; };
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Save a PeaksWorkspace to a ASCII .hkl file.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override {
    return {"LoadHKL"};
  }
  /// Algorithm's category for identification
  const std::string category() const override {
    return "Crystal\\DataHandling;DataHandling\\Text";
  }

private:
  /// Initialise the properties
  void init() override;
  /// Run the algorithm
  void exec() override;

  double absor_sphere(double &twoth, double &wl, double &tbar);
  double m_smu = 0.0;    // in 1/cm
  double m_amu = 0.0;    // in 1/cm
  double m_radius = 0.0; // in cm
  double m_power_th = 0.0;
  double spectrumCalc(double TOF, int iSpec,
                      std::vector<std::vector<double>> time,
                      std::vector<std::vector<double>> spectra, size_t id);
  DataObjects::PeaksWorkspace_sptr ws;
  void sizeBanks(std::string bankName, int &nCols, int &nRows);
};

} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_SAVEHKL_H_ */
