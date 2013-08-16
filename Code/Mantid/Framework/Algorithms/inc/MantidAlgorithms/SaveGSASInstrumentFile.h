#ifndef MANTID_ALGORITHMS_SAVEGSASINSTRUMENTFILE_H_
#define MANTID_ALGORITHMS_SAVEGSASINSTRUMENTFILE_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/TableWorkspace.h"

namespace Mantid
{
namespace Algorithms
{

class ChopperConfiguration
{
public:
  ChopperConfiguration(double freq, std::string bankidstr, std::string cwlstr, std::string mndspstr,
                       std::string mxdspstr, std::string maxtofstr);

  /// Get bank IDs in configuration
  std::vector<unsigned int> getBankIDs();
  /// Check wehther a bank is defined
  bool hasBank(unsigned int bankid);
  /// Get a parameter from a bank
  double getParameter(unsigned int bankid, std::string paramname);
  /// Set a parameter to a bank
  void setParameter(unsigned int bankid, std::string paramname, double value);

private:
  std::string parseString();
  /// Parse string to a double vector
  std::vector<double> parseStringDbl(std::string instring);
  /// Parse string to an integer vector
  std::vector<unsigned int> parseStringUnsignedInt(std::string instring);

  double m_frequency;
  std::vector<unsigned int> m_bankIDs;
  std::map<unsigned int, size_t> m_bankIDIndexMap;

  std::vector<double> m_vec2Theta;
  std::vector<double> m_vecL1;
  std::vector<double> m_vecL2;

  std::vector<double> m_vecCWL;
  std::vector<double> m_mindsps;
  std::vector<double> m_maxdsps;
  std::vector<double> m_maxtofs;

  std::vector<double> m_splitds;
  std::vector<int> m_vruns;

};

typedef boost::shared_ptr<ChopperConfiguration> ChopperConfiguration_sptr;

/** SaveGSASInstrumentFile : TODO: DESCRIPTION
    
    Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
  /// Algorithm's version
  virtual int version() const { return (1); }
  /// Algorithm's category for identification
  virtual const std::string category() const { return "Diffraction;Algorithm\\Text"; }

private:
  /// Sets documentation strings for this algorithm
  virtual void initDocs();
  /// Initialisation code
  void init();
  /// Execution code
  void exec();

  /// Process properties
  void processProperties();

  /// Set up some constant by default
  void initConstants(double chopperfrequency);

  /// Set up for PG3 chopper constants
  ChopperConfiguration_sptr setupPG3Constants(int intfrequency);
  /// Set up for NOM chopper constants
  ChopperConfiguration_sptr setupNOMConstants(int intfrequency);

  /// Parse profile table workspace to a map
  void parseProfileTableWorkspace(DataObjects::TableWorkspace_sptr ws,
                                  std::map<unsigned int, std::map<std::string, double> >& profilemap);

  /// Convert to GSAS instrument file
  void convertToGSAS(std::vector<unsigned int> banks, std::string gsasinstrfilename,
                     std::map<unsigned int, std::map<std::string, double> > bankprofilemap);

  /// Build a data structure for GSAS's tabulated peak profile
  void buildGSASTabulatedProfile(std::map<unsigned int, std::map<std::string, double> > bankprofilemap, unsigned int bankid);

  /// Write the header of the file
  void writePRMHeader(std::vector<unsigned int> banks, std::string prmfilename);

  /// Write out .prm/.iparm file
  void writePRMSingleBank(std::map<unsigned int, std::map<std::string, double> > bankprofilemap,
                          unsigned int bankid, std::string prmfilename);

  ///
  void makeParameterConsistent();

  /// Caclualte L2 from DIFFC and L1
  double calL2FromDtt1(double difc, double L1, double twotheta);

  /// Calculate TOF difference
  double calTOF(double n, double ep, double eq, double er, double tp, double tq, double tr, double dsp);

  /// Calculate a value related to (alph0, alph1, alph0t, alph1t) or (beta0, beta1, beta0t, beta1t)
  double aaba(double n, double ea1, double ea2, double ta1, double ta2, double dsp);

  /// Get parameter value from a map
  double getValueFromMap(std::map<std::string, double> profilemap, std::string parname);

  /// Get parameter value from class storage
  // double getProfileParameterValue(unsigned int bankid, std::string paramname);
  double getProfileParameterValue(std::map<std::string, double> profilemap , std::string paramname);

  /// Load fullprof resolution file.
  void loadFullprofResolutionFile(std::string irffilename);

  /// Input workspace
  DataObjects::TableWorkspace_sptr m_inpWS;

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
  ChopperConfiguration_sptr m_configuration;

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

/// Examine whether str1's last few characters are same as str2
bool endswith(std::string str1, std::string str2) { throw std::runtime_error("Implement soon!");}


} // namespace Algorithms
} // namespace Mantid

#endif  /* MANTID_ALGORITHMS_SAVEGSASINSTRUMENTFILE_H_ */
