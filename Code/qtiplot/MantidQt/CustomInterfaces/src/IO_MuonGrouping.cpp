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
void saveGroupingTabletoXML(Ui::MuonAnalysis& m_uiForm, const std::string& filename)
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

  int numRows = m_uiForm.groupTable->rowCount();
  for (int i = 0; i < numRows; i++)
  {
    QTableWidgetItem *item = m_uiForm.groupTable->item(i,1);
    if (!item)
      break;
    if ( item->text().isEmpty() )
      break;

    Element* gElem = mDoc->createElement("group");
    gElem->setAttribute("name", m_uiForm.groupTable->item(i,0)->text().toStdString());
    rootElem->appendChild(gElem);
    Element* idsElem = mDoc->createElement("ids");
    idsElem->setAttribute("val", m_uiForm.groupTable->item(i,1)->text().toStdString());
    gElem->appendChild(idsElem);
  }

  // loop over pairs in pair table

  numRows = m_uiForm.pairTable->rowCount();
  for (int i = 0; i < numRows; i++)
  {
    QTableWidgetItem *itemName = m_uiForm.pairTable->item(i,0);
    if (!itemName)
      break;
    if ( itemName->text().isEmpty() )
      break;
    QTableWidgetItem *itemAlpha = m_uiForm.pairTable->item(i,3);
    if (!itemAlpha)
      break;
    if ( itemAlpha->text().isEmpty() )
      break;

    Element* gElem = mDoc->createElement("pair");
    gElem->setAttribute("name", itemName->text().toStdString());
    rootElem->appendChild(gElem);
    Element* fwElem = mDoc->createElement("forward-group");
    fwElem->setAttribute("name", m_uiForm.pairTable->item(i,1)->text().toStdString());
    gElem->appendChild(fwElem);
    Element* bwElem = mDoc->createElement("backward-group");
    bwElem->setAttribute("name", m_uiForm.pairTable->item(i,2)->text().toStdString());
    gElem->appendChild(bwElem);
    Element* alphaElem = mDoc->createElement("alpha");
    alphaElem->setAttribute("val", itemAlpha->text().toStdString());
    gElem->appendChild(alphaElem);
  } 

  writer.writeNode(outFile, mDoc);
}

/**
 * load XML grouping file
 */
void loadGroupingXMLtoTable(Ui::MuonAnalysis& m_uiForm, const std::string& filename)
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
    throw Mantid::Kernel::Exception::FileError("Unable to parse File:" , filename);
  }
  // Get pointer to root element
  Element* pRootElem = pDoc->documentElement();
  if ( !pRootElem->hasChildNodes() )
  {
    throw Mantid::Kernel::Exception::FileError("No root element in XML grouping file:" , filename);
  }  

  NodeList* pNL_group = pRootElem->getElementsByTagName("group");
  if ( pNL_group->length() == 0 )
  {
    throw Mantid::Kernel::Exception::FileError("XML group file contains no group elements:" , filename);
  }

  // Clear content of tables table
  m_uiForm.groupTable->clearContents();
  m_uiForm.pairTable->clearContents();
  m_uiForm.frontGroupGroupPairComboBox->clear();

  for (int i = 0; i < m_uiForm.pairTable->rowCount(); i++)
  {
    m_uiForm.pairTable->setCellWidget(i,1, new QComboBox);
    m_uiForm.pairTable->setCellWidget(i,2, new QComboBox);
  }


  // add content to group table
  unsigned int numberGroups = pNL_group->length();
  for (unsigned int iGroup = 0; iGroup < numberGroups; iGroup++)
  {
    Element* pGroupElem = static_cast<Element*>(pNL_group->item(iGroup));

    if ( !pGroupElem->hasAttribute("name") )
      throw Mantid::Kernel::Exception::FileError("Group element without name" , filename);
    std::string gName = pGroupElem->getAttribute("name");


    Element* idlistElement = pGroupElem->getChildElement("ids");
    if (idlistElement)
    {
      std::string ids = idlistElement->getAttribute("val");

      // add info to table
      m_uiForm.groupTable->setItem(iGroup,0, new QTableWidgetItem(gName.c_str()) );
      m_uiForm.groupTable->setItem(iGroup,1, new QTableWidgetItem(ids.c_str()) );
    }
    else
    {
      //g_log.error("XML group file: " + filename + "contains no <ids> elements.");
      throw Mantid::Kernel::Exception::FileError("XML group file contains no <ids> elements:" , filename);
    }   
  }
  pNL_group->release();
  


  // add content to pair table

/*  NodeList* pNL_pair = pRootElem->getElementsByTagName("pair");
  if ( pNL_pair->length() == 0 )

  numberGroups = pNL_pair->length();
  for (unsigned int iGroup = 0; iGroup < numberGroups; iGroup++)
  {
    Element* pGroupElem = static_cast<Element*>(pNL_pair->item(iGroup));

    if ( !pGroupElem->hasAttribute("name") )
      throw Mantid::Kernel::Exception::FileError("pair element without name" , filename);
    std::string gName = pGroupElem->getAttribute("name");


    Element* fwElement = pGroupElem->getChildElement("forward-group");
    if (fwElement)
    {
      std::string ids = fwElement->getAttribute("val");

      // add info to table
      m_uiForm.groupTable->setItem(iGroup,0, new QTableWidgetItem(gName.c_str()) );
      m_uiForm.groupTable->setItem(iGroup,1, new QTableWidgetItem(ids.c_str()) );
    }
    else
    {
      throw Mantid::Kernel::Exception::FileError("XML pair group contains no <forward-group> elements:" , filename);
    }   
  }
  pNL_group->release(); */


}

}
}
