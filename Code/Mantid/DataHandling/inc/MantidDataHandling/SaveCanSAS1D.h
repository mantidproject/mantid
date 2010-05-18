#ifndef MANTID_DATAHANDLING_SaveCanSAS1D_H
#define MANTID_DATAHANDLING_SaveCanSAS1D_H

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"
//----------------------------------------------------------------------

namespace Poco{
  namespace XML{
    class Document;
    class Element;
    class Text;
  }
}

namespace Mantid
{
  namespace DataHandling
  {
    /** @class SaveCanSAS1D  DataHandling/SaveCanSAS1D.h

    This algorithm saves  workspace into CanSAS1d format.
    The structure of CanSAS1d xml is:
    
    @verbatim
    <SASroot version="1.0" xmlns="" xmlns:xsi="" xsi:schemaLocation="">
    <SASentry>
      <Title></Title>
      <Run></Run>
      <SASdata>
        <Idata>
          <Q unit="1/A"></Q>
          <I unit="a.u."></I>
          <Idev unit="a.u."></Idev>
          <Qdev unit="1/A"></Qdev>
        </Idata>
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

    Copyright &copy; 2007-10 STFC Rutherford Appleton Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
    class DLLExport SaveCanSAS1D : public API::Algorithm
    {
    public:
      /// default constructor
      SaveCanSAS1D();
      ~SaveCanSAS1D();

      /// Algorithm's name for identification overriding a virtual method
      virtual const std::string name() const { return "SaveCanSAS1D"; }
      /// Algorithm's version for identification overriding a virtual method
      virtual const int version() const { return 1; }
      /// Algorithm's category for identification overriding a virtual method
      virtual const std::string category() const { return "DataHandling"; }

    private:
      /// Overwrites Algorithm method.
      void init();
      /// Overwrites Algorithm method
      void exec();

      /// this method searches for xml special characters and replace with entity references
      void searchandreplaceSpecialChars(std::string &input);

      /// replaces the charcter at index in the input string with xml entity reference(eg.replace '&' with "&amp;")
      void replacewithEntityReference(std::string& input, std::string::size_type index);

      /// sasroot element
      void createSASRootElement(std::string& rootElem);

      /// this method creates sastitle element
      void createSASTitleElement(std::string& sasTitle);

      /// this method creates sassample element
      void createSASSampleElement(std::string &sasSample);

      /// this method creates sasRun Element
      void createSASRunElement(std::string& sasRun);

      /// this method creates SASData element
      void createSASDataElement(std::string& sasData);

      /// this method creates SASSourcelement
      void createSASSourceElement(std::string& sasSource );

      ///this method creates sasDetector element
      void createSASDetectorElement(std::string& sasDet);

      ///this method creates sasProcess element
      void createSASProcessElement(std::string& sasProcess);

      API::MatrixWorkspace_sptr m_workspace; ///<workspace
    };
    
  }
}

#endif
