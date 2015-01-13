#ifndef MANTID_DATAHANDLING_SaveCanSAS1D_H
#define MANTID_DATAHANDLING_SaveCanSAS1D_H

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include <fstream>

namespace Poco {
namespace XML {
class Document;
class Element;
class Text;
}
}

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

Copyright &copy; 2007-10 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

File change history is stored at: <https://github.com/mantidproject/mantid>.
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport SaveCanSAS1D : public API::Algorithm {
public:
  /// default constructor
  SaveCanSAS1D();
  virtual ~SaveCanSAS1D();

  virtual const std::string name() const { return "SaveCanSAS1D"; }
  virtual const std::string summary() const {
    return "Save a MatrixWorkspace to a file in the canSAS 1-D format";
  }
  virtual int version() const { return 1; }
  virtual const std::string category() const {
    return "DataHandling\\XML;SANS";
  }

protected:
  /// Overwrites Algorithm method.
  virtual void init();
  /// overriden method sets appending for workspace groups
  void setOtherProperties(API::IAlgorithm *alg, const std::string &propertyName,
                          const std::string &propertyValue, int perioidNum);
  /// Overwrites Algorithm method
  virtual void exec();

  /// Opens the output file and, as necessary blanks it, writes the file header
  /// and moves the file pointer
  void prepareFileToWriteEntry();
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
  void replacewithEntityReference(std::string &input,
                                  const std::string::size_type &index);
  /// sasroot element
  virtual void createSASRootElement(std::string &rootElem);

  /// this method creates sastitle element
  void createSASTitleElement(std::string &sasTitle);

  /// this method creates sassample element
  void createSASSampleElement(std::string &sasSample);

  /// this method creates sasRun Element
  void createSASRunElement(std::string &sasRun);

  /// this method creates SASData element
  void createSASDataElement(std::string &sasData);

  /// this method creates SASSourcelement
  void createSASSourceElement(std::string &sasSource);

  /// this method creates sasDetector element
  void createSASDetectorElement(std::string &sasDet);

  /// this method creates sasProcess element
  void createSASProcessElement(std::string &sasProcess);

  /// points to the workspace that will be written to file
  API::MatrixWorkspace_const_sptr m_workspace;
  /// an fstream object is used to write the xml manually as the user requires a
  /// specific format with new line characters and this can't be done in using
  /// the stylesheet part in Poco or libXML
  std::fstream m_outFile;
};
}
}

#endif
