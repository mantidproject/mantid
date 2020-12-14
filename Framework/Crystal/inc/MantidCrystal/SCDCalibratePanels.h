// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/Quat.h"

namespace Mantid {
namespace Crystal {
/** SCDCalibratePanels calibrates instrument parameters for Rectangular
 Detectors

 *  @author Ruth Mikkelson(adapted from Isaw's Calibration code)
 *  @date   Mar 12, 2012
 *
 *
 */

class SCDCalibratePanels : public Mantid::API::Algorithm {
public:

  /// Algorithm's name for identification
  const std::string name() const override {
    return "SCDCalibratePanels";
  };

  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Panel parameters and L0 are optimized to "
           "minimize errors between theoretical and actual q values for the "
           "peaks";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; };

  /// Algorithm's category, overriding a virtual method
  const std::string category() const override {
    return "Crystal\\Corrections";
  };

  /// Extra help info
  const std::vector<std::string> seeAlso() const override {
    return {"CalculateUMatrix"};
  };

private:
  
  /// save to DetCal format, can be loaded with func LoadIsawDetCal()
  void saveIsawDetCal(std::shared_ptr<Geometry::Instrument> &instrument,
                      boost::container::flat_set<std::string> &AllBankName,
                      double T0, const std::string &filename);

  /// Function to calculate U
  void findU(const DataObjects::PeaksWorkspace_sptr &peaksWs);

  /// save workspaces
  void saveNexus(const std::string &outputFile,
                 const API::MatrixWorkspace_sptr &outputWS);

  /// Function to optimize L1
  void findL1(int nPeaks, const DataObjects::PeaksWorkspace_sptr &peaksWs);

  /// Function to optimize L2
  void findL2(boost::container::flat_set<std::string> MyBankNames,
              const DataObjects::PeaksWorkspace_sptr &peaksWs);

  /// Function to optimize T0
  void findT0(int nPeaks, const DataObjects::PeaksWorkspace_sptr &peaksWs);

  void exec() override;

  void init() override;

  API::ITableWorkspace_sptr Result;

  double mT0 = 0.0;

  /**
   * Saves the new instrument to an xml file that can be used with the
   * LoadParameterFile Algorithm. If the filename is empty, nothing gets done.
   *
   * @param FileName     The filename to save this information to
   *
   * @param AllBankNames The names of the banks in each group whose values are
   *                         to be saved to the file
   *
   * @param instrument   The instrument with the new values for the banks in
   *Groups
   */
  void saveXmlFile(const std::string &FileName,
                   const boost::container::flat_set<std::string> &AllBankNames,
                   const Geometry::Instrument &instrument) const;
};

} // namespace Crystal
} // namespace Mantid
