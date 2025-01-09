// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidDataHandling/DllConfig.h"
#include "MantidDataObjects/TableWorkspace.h"

namespace Mantid {
namespace DataHandling {

class ChopperConfiguration;

/** SaveGSASInstrumentFile :  Convert Fullprof"s instrument resolution file
   (.irf) to  GSAS"s instrument
    file (.iparm/.prm).
  */
class MANTID_DATAHANDLING_DLL SaveGSASInstrumentFile final : public API::Algorithm {
public:
  SaveGSASInstrumentFile();
  /// Algorithm's name
  const std::string name() const override { return "SaveGSASInstrumentFile"; }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Generate a GSAS instrument file from either a table workspace "
           "containing profile parameters or a Fullprof's instrument "
           "resolution file (.irf file). ";
  }

  /// Algorithm's version
  int version() const override { return (1); }
  const std::vector<std::string> seeAlso() const override { return {"LoadGSASInstrumentFile", "SaveGSS"}; }
  /// Algorithm's category for identification
  const std::string category() const override { return "Diffraction\\DataHandling"; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;

  /// Process properties
  void processProperties();

  /// Set up some constant by default
  void initConstants(const std::map<unsigned int, std::map<std::string, double>> &profmap);

  /// Set up chopper/instrument constant parameters from profile map
  std::shared_ptr<ChopperConfiguration>
  setupInstrumentConstants(const std::map<unsigned int, std::map<std::string, double>> &profmap);

  /// Set up for PG3 chopper constants
  std::shared_ptr<ChopperConfiguration> setupPG3Constants(int intfrequency);
  /// Set up for NOM chopper constants
  std::shared_ptr<ChopperConfiguration> setupNOMConstants(int intfrequency);

  /// Parse profile table workspace to a map
  void parseProfileTableWorkspace(const API::ITableWorkspace_sptr &ws,
                                  std::map<unsigned int, std::map<std::string, double>> &profilemap);

  /// Convert to GSAS instrument file
  void convertToGSAS(const std::vector<unsigned int> &outputbankids, const std::string &gsasinstrfilename,
                     const std::map<unsigned int, std::map<std::string, double>> &bankprofilemap);

  /// Build a data structure for GSAS's tabulated peak profile
  void buildGSASTabulatedProfile(const std::map<unsigned int, std::map<std::string, double>> &bankprofilemap,
                                 unsigned int bankid);

  /// Write the header of the file
  void writePRMHeader(const std::vector<unsigned int> &banks, const std::string &prmfilename);

  /// Write out .prm/.iparm file
  void writePRMSingleBank(const std::map<unsigned int, std::map<std::string, double>> &bankprofilemap,
                          unsigned int bankid, const std::string &prmfilename);

  /// Caclualte L2 from DIFFC and L1
  double calL2FromDtt1(double difc, double L1, double twotheta);

  /// Calculate TOF difference
  double calTOF(double n, double ep, double eq, double er, double tp, double tq, double tr, double dsp);

  /// Calculate a value related to (alph0, alph1, alph0t, alph1t) or (beta0,
  /// beta1, beta0t, beta1t)
  double aaba(double n, double ea1, double ea2, double ta1, double ta2, double dsp);

  /// Get parameter value from a map
  double getValueFromMap(const std::map<std::string, double> &profilemap, const std::string &parname);

  /// Get parameter value from class storage
  double getProfileParameterValue(const std::map<std::string, double> &profilemap, const std::string &paramname);

  /// Load fullprof resolution file.
  void loadFullprofResolutionFile(const std::string &irffilename);

  /// Calcualte d-space value.
  double calDspRange(double dtt1, double zero, double tof);

  // TODO Replace it with gsl's erfc()
  double erfc(double xx);

  /// Input workspace
  API::ITableWorkspace_sptr m_inpWS;
  // DataObjects::TableWorkspace_sptr m_inpWS;

  /// Instrument
  std::string m_instrument;
  /// L1
  double m_L1;
  /// L2
  double m_L2;
  /// 2Theta
  double m_2theta;
  /// Frequency
  int m_frequency;
  /// User input ID line
  std::string m_id_line;
  /// Sample
  std::string m_sample;

  /// Banks IDs to process
  std::vector<unsigned int> m_vecBankID2File;

  /// Output file name
  std::string m_gsasFileName;

  /// Chopper configuration
  std::shared_ptr<ChopperConfiguration> m_configuration;

  /// Profile parameter map
  std::map<unsigned int, std::map<std::string, double>> m_profileMap;

  //
  std::vector<double> m_gdsp;
  std::vector<double> m_gdt;
  std::vector<double> m_galpha;
  std::vector<double> m_gbeta;

  std::map<unsigned int, double> m_bank_mndsp;
  std::map<unsigned int, double> m_bank_mxtof;
};

} // namespace DataHandling
} // namespace Mantid
