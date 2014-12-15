#ifndef MANTID_DATAHANDLING_LOADFULLPROFRESOLUTION_H_
#define MANTID_DATAHANDLING_LOADFULLPROFRESOLUTION_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/TableWorkspace.h"

namespace Poco { namespace XML {
  class Document;
  class Element;
}}

namespace Mantid
{
  namespace DataHandling
  {

    /** LoadFullprofResolution : Load Fullprof resolution (.irf) file to TableWorkspace(s)

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
    class DLLExport LoadFullprofResolution : public API::Algorithm
    {
    public:
      LoadFullprofResolution();
      virtual ~LoadFullprofResolution();

      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "LoadFullprofResolution";}

      /// Algorithm's version for identification overriding a virtual method
      virtual int version() const { return 1;}

      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "Diffraction";}
      ///Summary of algorithms purpose
      virtual const std::string summary() const {return "Load Fullprof's resolution (.irf) file to one or multiple TableWorkspace(s) and/or where this is supported."
        " See description section, translate fullprof resolution fitting parameter into Mantid equivalent fitting parameters.";}

      /// Get row numbers of the parameters in the table workspace
      static void getTableRowNumbers(const API::ITableWorkspace_sptr & tablews, std::map<std::string, size_t>& parammap);

      /// Put parameters into a matrix workspace
      static void putParametersIntoWorkspace( const API::Column_const_sptr, API::MatrixWorkspace_sptr ws, int profNumber, std::string & parameterXMLString);

      /// Add an Ikeda-Carpenter PV ALFBE parameter 
      static void addALFBEParameter(const API::Column_const_sptr, Poco::XML::Document* mDoc, Poco::XML::Element* parent, const std::string& paramName);

      /// Add set of Ikeda-Carpenter PV Sigma parameters 
      static void addSigmaParameters(const API::Column_const_sptr, Poco::XML::Document* mDoc, Poco::XML::Element* parent );

      /// Add set of Ikeda-Carpenter PV Gamma parameters 
      static void addGammaParameters(const API::Column_const_sptr, Poco::XML::Document* mDoc, Poco::XML::Element* parent );

      /// Add set of BackToBackExponential S parameters 
      static void addBBX_S_Parameters(const API::Column_const_sptr, Poco::XML::Document* mDoc, Poco::XML::Element* parent );

      /// Add set of BackToBackExponential A parameters 
      static void addBBX_A_Parameters(const API::Column_const_sptr, Poco::XML::Document* mDoc, Poco::XML::Element* parent );

      /// Add set of BackToBackExponential B parameters 
      static void addBBX_B_Parameters(const API::Column_const_sptr, Poco::XML::Document* mDoc, Poco::XML::Element* parent );

      /// Get value for XML eq attribute for parameter
      static std::string getXMLEqValue( const API::Column_const_sptr, const std::string& name );

      /// Get value for XML eq attribute for squared parameter
      static std::string getXMLSquaredEqValue( const API::Column_const_sptr column, const std::string& name );

      // Translate a parameter name from as it appears in the table workspace to its name in the XML file
      static std::string getXMLParameterName( const std::string& name );

    private:
      /// Implement abstract Algorithm methods
      void init();
      /// Implement abstract Algorithm methods
      void exec();

      /// Load file to a vector of strings
      void loadFile(std::string filename, std::vector<std::string>& lines);

      /// Get the NPROF number
      int getProfNumber( const std::vector<std::string>& lines );

      /// Scan imported file for bank information
      void scanBanks(const std::vector<std::string>& lines, const bool useBankIDsInFile, std::vector<int>& banks,
        std::map<int, int> &bankstartindexmap, std::map<int, int> &bankendindexmap );

      /// Parse .irf file to a map
      void parseResolutionStrings(std::map<std::string, double>& parammap, const std::vector<std::string>& lines, const bool useBankIDsInFile, int bankid, int startlineindex, int endlineindex, int nProf);

      void parseBankLine(std::string line, double& cwl, int& bankid);

      /// Search token for profile number
      int searchProfile();

      /// Parse 1 bank of lines of profile 9
      void parseProfile9();

      /// Parse 1 bank of lines of profile 10
      void parseProfile10();

      /// Generate output workspace
      DataObjects::TableWorkspace_sptr genTableWorkspace(std::map<int, std::map<std::string, double> > bankparammap);

      /// Generate bank information workspace
      DataObjects::TableWorkspace_sptr genInfoTableWorkspace(std::vector<int> banks);

      /// Create Bank to Workspace Correspondence
      void createBankToWorkspaceMap ( const std::vector<int>& banks, const std::vector<int>& workspaces, std::map< int, size_t>& WorkpsaceOfBank );

      /// Place to store the row numbers
      static std::map<std::string, size_t> m_rowNumbers;

    };


  } // namespace DataHandling
} // namespace Mantid

#endif  /* MANTID_DATAHANDLING_LOADFULLPROFRESOLUTION_H_ */
