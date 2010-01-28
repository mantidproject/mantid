#ifndef MANTID_DATAHANDLING_SaveCanSAS1D_H
#define MANTID_DATAHANDLING_SaveCanSAS1D_H


//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
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
	The structure of CanSAS1d xml is
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
			</SASdetector>
		</SASinstrument>
		<SASnote></SASnote>
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

	  /// creates SASRoot element
	  Poco::XML::Element* createSASRootElement();
	 
	 /// creates SASEntry Element 
	  Poco::XML::Element* createSASEntryElement(Poco::XML::Element* parent);
	  /// creates SASDataElement and the elements it contains
	  void  createSASDataElement(Poco::XML::Element* parent);
	  ///creates SASsample element and  the elements it contains
	  void  createSASsample(Poco::XML::Element* parent);
      /// creates SASNInstrument element and the elements it contains
	  void createSASInstrument(Poco::XML::Element* parent);
	  /// createSASNote element
	  void createSASnote(Poco::XML::Element* parent);
	  /// creates Title element
	  void createTitleElement(Poco::XML::Element* parent);

	  /// creates Run element
	  void createRunElement(Poco::XML::Element* parent);

	  /// This method throws NullPointerException when element creation fails
	  void throwException(Poco::XML::Element* elem,const std::string & place,const std::string & objectName);

	   ///Overloaded method. This method throws NullPointerException when Text node creation fails
	  void throwException(Poco::XML::Text* text,const std::string & place,const std::string & objectName);
	 
	  API::MatrixWorkspace_const_sptr m_workspace;///<workspace
	  Poco::XML::Document* mDoc;         ///< The XML document    
	  Poco::XML::Element*  mRoot;	     ///< The root XML element

	   

	  };
    
  }
}
#endif