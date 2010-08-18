//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidQtCustomInterfaces/MuonAnalysis.h"
#include "MantidQtCustomInterfaces/IO_MuonGrouping.h"

#include "Poco/DOM/Document.h"
#include "Poco/DOM/DOMWriter.h"
#include "Poco/DOM/Element.h"
#include "Poco/DOM/Text.h"
#include "Poco/XML/XMLWriter.h"

#include "Poco/DOM/DOMParser.h"
#include "Poco/DOM/Document.h"
#include "Poco/DOM/NodeList.h"
#include "Poco/DOM/NodeIterator.h"
#include "Poco/DOM/NodeFilter.h"
#include "Poco/File.h"
#include "Poco/Path.h"

#include <boost/shared_ptr.hpp>
#include <fstream>  
//-----------------------------------------------------------------------------
using namespace Poco::XML;

namespace MantidQt
{
namespace CustomInterfaces
{

  using namespace Poco::XML;

/**
 * save XML grouping file
 */
void saveGroupingTabletoXML(QTableWidget* gTable, std::string& filename)
{
  std::ofstream outFile(filename.c_str());
  if (!outFile)
  {
    throw Mantid::Kernel::Exception::FileError("Unable to open file:", filename);
  }

  DOMWriter writer;
  writer.setNewLine("\n");
  writer.setOptions(XMLWriter::PRETTY_PRINT);

  Poco::XML::Document* mDoc = new Document();
  Element* rootElem = mDoc->createElement("detector-grouping");
  mDoc->appendChild(rootElem);

  // loop over groups in table

  int numRows = gTable->rowCount();
  for (int i = 0; i < numRows; i++)
  {
    QTableWidgetItem *item = gTable->item(i,1);
    if (!item)
      break;
    if ( item->text().isEmpty() )
      break;

    Element* gElem = mDoc->createElement("group");
    gElem->setAttribute("name", gTable->item(i,0)->text().toStdString());
    rootElem->appendChild(gElem);
    Element* idsElem = mDoc->createElement("ids");
    idsElem->setAttribute("val", gTable->item(i,1)->text().toStdString());
    gElem->appendChild(idsElem);
  }

  writer.writeNode(outFile, mDoc);
}

/**
 * load XML grouping file
 */
void loadGroupingXMLtoTable(QTableWidget* gTable, std::string& filename)
{
  // Set up the DOM parser and parse xml file
  DOMParser pParser;
  Document* pDoc;
  try
  {
    pDoc = pParser.parse(filename);
  }
  catch(...)
  {
    //g_log.error("Unable to parse file " + filename);
    throw Mantid::Kernel::Exception::FileError("Unable to parse File:" , filename);
  }
  // Get pointer to root element
  Element* pRootElem = pDoc->documentElement();
  if ( !pRootElem->hasChildNodes() )
  {
    //g_log.error("XML file: " + filename + "contains no root element.");
    throw Mantid::Kernel::Exception::FileError("No root element in XML grouping file:" , filename);
  }  

  NodeList* pNL_group = pRootElem->getElementsByTagName("group");
  if ( pNL_group->length() == 0 )
  {
    //g_log.error("XML group file: " + filename + "contains no group elements.");
    throw Mantid::Kernel::Exception::FileError("XML group file contains no group elements:" , filename);
  }

  // Clear content of table
  gTable->clearContents();


  unsigned int numberGroups = pNL_group->length();
  for (unsigned int iGroup = 0; iGroup < numberGroups; iGroup++)
  {
    Element* pGroupElem = static_cast<Element*>(pNL_group->item(iGroup));

    if ( !pGroupElem->hasAttribute("name") )
      throw Mantid::Kernel::Exception::FileError("Group element with name" , filename);
    std::string gName = pGroupElem->getAttribute("name");


    Element* idlistElement = pGroupElem->getChildElement("ids");
    if (idlistElement)
    {
      std::string ids = idlistElement->getAttribute("val");

      // add info to table
      gTable->setItem(iGroup,0, new QTableWidgetItem(gName.c_str()) );
      gTable->setItem(iGroup,1, new QTableWidgetItem(ids.c_str()) );
    }
    else
    {
      //g_log.error("XML group file: " + filename + "contains no <ids> elements.");
      throw Mantid::Kernel::Exception::FileError("XML group file contains no <ids> elements:" , filename);
    }   
  }
  pNL_group->release();
  
}

}
}
