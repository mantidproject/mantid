// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataHandling/DllConfig.h"
#include <fstream>

namespace Poco {
namespace XML {
class Document;
class Element;
class Text;
} // namespace XML
} // namespace Poco

namespace Mantid {
namespace DataHandling {
/** @class SaveCanSAS1D  DataHandling/SaveCanSAS1D.h

@verbatim

This algorithm saves  workspace into CanSAS1d format. This is an xml format
except
the <Idata>, </Idata> tags and all data in between must be one line, which
necesitates
the files be written iostream functions outside xml libraries.

The structure of CanSAS1d xml is:

<SASroot version="1.0" xmlns="" xmlns:xsi="" xsi:schemaLocation="">
<SASentry>
  <Title></Title>
  <Run></Run>
  <SASdata>
    <Idata><Q unit="1/A"></Q><I unit="a.u."></I><Idev unit="a.u."></Idev><Qdev
unit="1/A"></Qdev></Idata>
  </SASdata>
  <SASsample>
    <ID></ID>
  </SASsample>
  <SASinstrument>
    <name></name>
    <SASsource>
      <radiation></radiation>
      <wavelength unit="A"></wavelength>
    </SASsource>
    <SAScollimation/>
    <SASdetector>
      <name></name>
      <SDD></SDD>
    </SASdetector>
  </SASinstrument>
  <SASprocess>
    <name></name>
    <date></date>
    <term name="svn"></term>
    <term name="user_file"></term>
  </SASprocess>
  <SASnote>
  </SASnote>
</SASentry>
</SASroot>
@endverbatim

Required properties:
<UL>
<LI> InputWorkspace - The name workspace to save.</LI>
<LI> Filename - The path save the file</LI>
</UL>

@author Sofia Antony, Rutherford Appleton Laboratory
@date 19/01/2010
*/
class MANTID_DATAHANDLING_DLL SaveCanSAS1D : public API::Algorithm {
public:
  virtual ~SaveCanSAS1D() = default;
  const std::string name() const override { return "SaveCanSAS1D"; }
  const std::string summary() const override {
    return "Save a MatrixWorkspace to a file in the CanSAS1D XML format (for 1D data).";
  }
  int version() const override { return 1; }
  const std::string category() const override { return "DataHandling\\XML;SANS\\DataHandling"; }

protected:
  /// Overwrites Algorithm method.
  void init() override;
  /// overriden method sets appending for workspace groups
  void setOtherProperties(API::IAlgorithm *alg, const std::string &propertyName, const std::string &propertyValue,
                          int perioidNum) override;
  /// Overwrites Algorithm method
  void exec() override;

  /// Opens the output file and, as necessary blanks it, writes the file header
  /// and moves the file pointer
  void prepareFileToWriteEntry(const std::string &fileName);
  /// opens the named file if possible or returns false
  bool openForAppending(const std::string &filename);
  /// Moves to the end of the last entry in the file
  void findEndofLastEntry();
  /// Write xml header tags
  virtual void writeHeader(const std::string &fileName);
  /// this method searches for xml special characters and replace with entity
  /// references
  void searchandreplaceSpecialChars(std::string &input);
  /// replaces the charcter at index in the input string with xml entity
  /// reference(eg.replace '&' with "&amp;")
  void replacewithEntityReference(std::string &input, const std::string::size_type &index);
  /// sasroot element
  virtual void createSASRootElement(std::string &rootElem);

  /// this method creates a sasTitle element
  void createSASTitleElement(std::string &sasTitle);

  /// this method creates a sasSample element
  void createSASSampleElement(std::string &sasSample);

  /// this method creates a sasRun Element
  void createSASRunElement(std::string &sasRun);

  /// this method creates a sasData element
  void createSASDataElement(std::string &sasData, size_t workspaceIndex);

  /// this method creates a sasSource element
  void createSASSourceElement(std::string &sasSource);

  /// this method creates a sasDetector element
  void createSASDetectorElement(std::string &sasDet);

  /// this method creates a sasProcess element
  void createSASProcessElement(std::string &sasProcess);

  /// this method creates a sasInstrument element
  void createSASInstrument(std::string &sasInstrument);

  /// points to the workspace that will be written to file
  API::MatrixWorkspace_const_sptr m_workspace;
  /// an fstream object is used to write the xml manually as the user requires a
  /// specific format with new line characters and this can't be done in using
  /// the stylesheet part in Poco or libXML
  std::fstream m_outFile;
};
} // namespace DataHandling
} // namespace Mantid
