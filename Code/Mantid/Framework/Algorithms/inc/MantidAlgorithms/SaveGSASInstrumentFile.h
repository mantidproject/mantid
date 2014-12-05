#ifndef MANTID_ALGORITHMS_SAVEGSASINSTRUMENTFILE_H_
#define MANTID_ALGORITHMS_SAVEGSASINSTRUMENTFILE_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"

namespace Mantid
{
namespace Algorithms
{

class ChopperConfiguration;

/** SaveGSASInstrumentFile :  Convert Fullprof"s instrument resolution file (.irf) to  GSAS"s instrument 
    file (.iparm/.prm).
    
    Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
class DLLExport SaveGSASInstrumentFile : public API::Algorithm
{
public:
  SaveGSASInstrumentFile();
  virtual ~SaveGSASInstrumentFile();
  /// Algorithm's name
  virtual const std::string name() const { return "SaveGSASInstrumentFile"; }
    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Generate a GSAS instrument file from either a table workspace containing profile parameters or a Fullprof's instrument resolution file (.irf file). ";}

  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "Diffraction"; }

private:
  
  /// Initialisation code
  void init();
  /// Execution code
  void exec();

  /// Process properties
  void processProperties();

  /// Set up some constant by default
  void initConstants(const std::map<unsigned int, std::map<std::string, double> >& profmap);

  /// Set up chopper/instrument constant parameters from profile map
  boost::shared_ptr<ChopperConfiguration> setupInstrumentConstants(
      const std::map<unsigned int, std::map<std::string, double> >& profmap);

  /// Set up for PG3 chopper constants
  boost::shared_ptr<ChopperConfiguration> setupPG3Constants(int intfrequency);
  /// Set up for NOM chopper constants
  boost::shared_ptr<ChopperConfiguration> setupNOMConstants(int intfrequency);

  /// Parse profile table workspace to a map
  void parseProfileTableWorkspace(API::ITableWorkspace_sptr ws,
                                  std::map<unsigned int, std::map<std::string, double> >& profilemap);

  /// Convert to GSAS instrument file
  void convertToGSAS(const std::vector<unsigned int>& outputbankids, const std::string& gsasinstrfilename,
                     const std::map<unsigned int, std::map<std::string, double> >& bankprofilemap);

  /// Build a data structure for GSAS's tabulated peak profile
  void buildGSASTabulatedProfile(const std::map<unsigned int, std::map<std::string, double> >& bankprofilemap, unsigned int bankid);

  /// Write the header of the file
  void writePRMHeader(const std::vector<unsigned int>& banks, const std::string& prmfilename);

  /// Write out .prm/.iparm file
  void writePRMSingleBank(const std::map<unsigned int, std::map<std::string, double> >& bankprofilemap,
                          unsigned int bankid, const std::string& prmfilename);


  /// Caclualte L2 from DIFFC and L1
  double calL2FromDtt1(double difc, double L1, double twotheta);

  /// Calculate TOF difference
  double calTOF(double n, double ep, double eq, double er, double tp, double tq, double tr, double dsp);

  /// Calculate a value related to (alph0, alph1, alph0t, alph1t) or (beta0, beta1, beta0t, beta1t)
  double aaba(double n, double ea1, double ea2, double ta1, double ta2, double dsp);

  /// Get parameter value from a map
  double getValueFromMap(const std::map<std::string, double>& profilemap, const std::string& parname);

  /// Get parameter value from class storage
  double getProfileParameterValue(const std::map<std::string, double>& profilemap , const std::string& paramname);

  /// Load fullprof resolution file.
  void loadFullprofResolutionFile(std::string irffilename);

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
  boost::shared_ptr<ChopperConfiguration> m_configuration;

  /// Profile parameter map
  std::map<unsigned int, std::map<std::string, double> > m_profileMap;

  //
  std::vector<double> m_gdsp;
  std::vector<double> m_gdt;
  std::vector<double> m_galpha;
  std::vector<double> m_gbeta;

  std::map<unsigned int, double> m_bank_mndsp;
  std::map<unsigned int, double> m_bank_mxtof;

};

} // namespace Algorithms
} // namespace Mantid

#endif  /* MANTID_ALGORITHMS_SAVEGSASINSTRUMENTFILE_H_ */
