//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidQtCustomInterfaces/MuonAnalysis.h"
#include "MantidQtCustomInterfaces/IO_MuonGrouping.h"

#include "MantidQtAPI/UserSubWindow.h"

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
namespace Muon
{

  using namespace Poco::XML;
  using namespace MantidQt::API;

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

  std::vector<int> groupToRow;
  whichGroupToWhichRow(m_uiForm, groupToRow);

  int num = groupToRow.size();
  for (int i = 0; i < groupToRow.size(); i++)
  {
    Element* gElem = mDoc->createElement("group");
    gElem->setAttribute("name", m_uiForm.groupTable->item(groupToRow[i],0)->text().toStdString());
    rootElem->appendChild(gElem);
    Element* idsElem = mDoc->createElement("ids");
    idsElem->setAttribute("val", m_uiForm.groupTable->item(groupToRow[i],1)->text().toStdString());
    gElem->appendChild(idsElem);
  }

  // loop over pairs in pair table

  num = m_uiForm.pairTable->rowCount();
  for (int i = 0; i < num; i++)
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

    QComboBox* qw1 = static_cast<QComboBox*>(m_uiForm.pairTable->cellWidget(i,1));
    QComboBox* qw2 = static_cast<QComboBox*>(m_uiForm.pairTable->cellWidget(i,2));

    Element* gElem = mDoc->createElement("pair");
    gElem->setAttribute("name", itemName->text().toStdString());
    rootElem->appendChild(gElem);
    Element* fwElem = mDoc->createElement("forward-group");
    fwElem->setAttribute("val", qw1->currentText().toStdString());
    gElem->appendChild(fwElem);
    Element* bwElem = mDoc->createElement("backward-group");
    bwElem->setAttribute("val", qw2->currentText().toStdString());
    gElem->appendChild(bwElem);
    Element* alphaElem = mDoc->createElement("alpha");
    alphaElem->setAttribute("val", itemAlpha->text().toStdString());
    gElem->appendChild(alphaElem);
  } 

  writer.writeNode(outFile, mDoc);
}

/**
 * load XML grouping file. It is assumed that tables and combo box cleared before this method is called
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


  // add content to group table

  QStringList allGroupNames;  // used to populate combo boxes 
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
      allGroupNames.push_back( m_uiForm.groupTable->item(iGroup,0)->text() );
    }
    else
    {
      throw Mantid::Kernel::Exception::FileError("XML group file contains no <ids> elements:" , filename);
    }   
  }
  pNL_group->release();
  

  // populate pair table combo boxes

  int rowNum = m_uiForm.pairTable->rowCount();
  for (int i = 0; i < rowNum; i++)
  {
    QComboBox* qw1 = static_cast<QComboBox*>(m_uiForm.pairTable->cellWidget(i,1));
    QComboBox* qw2 = static_cast<QComboBox*>(m_uiForm.pairTable->cellWidget(i,2));

    for (int ii = 0; ii < allGroupNames.size(); ii++)
    {

      qw1->addItem( allGroupNames[ii] );
      qw2->addItem( allGroupNames[ii] );
    }
    
    if ( qw2->count() > 1 )
      qw2->setCurrentIndex(1);
  }




  // add content to pair table

  QStringList allPairNames;  
  NodeList* pNL_pair = pRootElem->getElementsByTagName("pair");
  int nPairs = pNL_pair->length();
  if ( pNL_pair->length() > 0 )
  {
    for (int iPair = 0; iPair < nPairs; iPair++)
    {
      Element* pGroupElem = static_cast<Element*>(pNL_pair->item(iPair));

      if ( !pGroupElem->hasAttribute("name") )
        throw Mantid::Kernel::Exception::FileError("pair element without name" , filename);
      std::string gName = pGroupElem->getAttribute("name");
      m_uiForm.pairTable->setItem(iPair,0, new QTableWidgetItem(gName.c_str()) );
      allPairNames.push_back(gName.c_str());

      Element* fwElement = pGroupElem->getChildElement("forward-group");
      if (fwElement)
      {
        std::string ids = fwElement->getAttribute("val");
        QComboBox* qw1 = static_cast<QComboBox*>(m_uiForm.pairTable->cellWidget(iPair,1));
        int comboIndex = qw1->findText(ids.c_str());
        if ( comboIndex < 0 )
          throw Mantid::Kernel::Exception::FileError("XML pair group contains forward-group with unrecognised group name" , filename);
        qw1->setCurrentIndex(comboIndex);
      }
      else
      {
        throw Mantid::Kernel::Exception::FileError("XML pair group contains no <forward-group> elements:" , filename);
      }   

      Element* bwElement = pGroupElem->getChildElement("backward-group");
      if (bwElement)
      {
        std::string ids = bwElement->getAttribute("val");
        QComboBox* qw2 = static_cast<QComboBox*>(m_uiForm.pairTable->cellWidget(iPair,2));
        int comboIndex = qw2->findText(ids.c_str());
        if ( comboIndex < 0 )
          throw Mantid::Kernel::Exception::FileError("XML pair group contains backward-group with unrecognised group name" , filename);
        qw2->setCurrentIndex(comboIndex);
      }
      else
      {
        throw Mantid::Kernel::Exception::FileError("XML pair group contains no <backward-group> elements:" , filename);
      }

      Element* element = pGroupElem->getChildElement("alpha");
      if (element)
      {
        if ( element->hasAttribute("val") )
        {
          m_uiForm.pairTable->setItem(iPair,3, new QTableWidgetItem(element->getAttribute("val").c_str()));
        }
        else
          throw Mantid::Kernel::Exception::FileError("XML pair group contains an <alpha> element with no 'val' attribute:" , filename);
      }
      // if alpha element not there for now just default it to 1.0
      else 
      {
        m_uiForm.pairTable->setItem(iPair,3, new QTableWidgetItem(1.0));
      }

    }
  }
  pNL_pair->release(); 


  // populate front combobox

  m_uiForm.frontGroupGroupPairComboBox->addItems(allGroupNames);
  m_uiForm.frontGroupGroupPairComboBox->addItems(allPairNames);


  // at some point put in some code which reads default choice 

  /*
  Element* element = pGroupElem->getChildElement("default");
  if (element)
  {
    if ( element->hasAttribute("name") )
    {
          m_uiForm.pairTable->setItem(iPair,3, new QTableWidgetItem(element->getAttribute("val").c_str()));
        }
      }
*/

}

/**
 * create 'map' relating group number to row number in group table
 *
 * @param groupToRow The 'map' returned
 */
void whichGroupToWhichRow(Ui::MuonAnalysis& m_uiForm, std::vector<int>& groupToRow)
{
  groupToRow.clear();

  int numRows = m_uiForm.groupTable->rowCount();
  for (int i = 0; i < numRows; i++)
  {
    // test if group name valid
    QTableWidgetItem *item0 = m_uiForm.groupTable->item(i,0);
    if (!item0)
      continue;
    if ( item0->text().isEmpty() )
      continue;

    // test if group IDs valid
    QTableWidgetItem *item1 = m_uiForm.groupTable->item(i,1);
    QTableWidgetItem *item2 = m_uiForm.groupTable->item(i,2);
    if (!item1 || !item2)
      continue;
    if ( item1->text().isEmpty() || item2->text().isEmpty() )
      continue;
    if ( item2->text() == "Invalid IDs string" )
      continue;

    groupToRow.push_back(i);
  }
}


/**
 * create 'map' relating pair number to row number in pair table
 *
 * @param pairToRow The 'map' returned
 */
void whichPairToWhichRow(Ui::MuonAnalysis& m_uiForm, std::vector<int>& pairToRow)
{
  pairToRow.clear();

  int numRows = m_uiForm.pairTable->rowCount();
  for (int i = 0; i < numRows; i++)
  {
    // test if pair name valid
    QTableWidgetItem *item0 = m_uiForm.pairTable->item(i,0);
    if (!item0)
      continue;
    if ( item0->text().isEmpty() )
      continue;

    // test if alpha is specified
    QTableWidgetItem *item3 = m_uiForm.pairTable->item(i,3);
    if (!item3)
      continue;
    if ( item3->text().isEmpty() )
      continue;

    pairToRow.push_back(i);
  }
}


}
}
}
