//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "IO_MuonGrouping.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidQtWidgets/Common/UserSubWindow.h"

#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/DOMWriter.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/Text.h>
#include <Poco/XML/XMLWriter.h>

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <fstream>

//-----------------------------------------------------------------------------
using namespace Mantid;
using namespace Mantid::API;
using namespace Poco::XML;

namespace MantidQt {
namespace CustomInterfaces {
namespace Muon {

using namespace Poco::XML;
using namespace MantidQt::API;

/**
 * Save grouping to the XML file specified.
 *
 * @param grouping :: Struct with grouping information
 * @param filename :: XML filename where information will be saved
 */
void MuonGroupingHelper::saveGroupingToXML(
    const Mantid::API::Grouping &grouping, const std::string &filename) {
  std::ofstream outFile(filename.c_str());
  if (!outFile)
    throw Mantid::Kernel::Exception::FileError("Unable to open output file",
                                               filename);

  DOMWriter writer;
  writer.setNewLine("\n");
  writer.setOptions(XMLWriter::PRETTY_PRINT);

  Poco::AutoPtr<Poco::XML::Document> mDoc = new Document();

  // Create root element with a description
  Poco::AutoPtr<Element> rootElem = mDoc->createElement("detector-grouping");
  rootElem->setAttribute("description", grouping.description);
  mDoc->appendChild(rootElem);

  // Create group elements
  for (size_t gi = 0; gi < grouping.groups.size(); gi++) {
    Poco::AutoPtr<Element> gElem = mDoc->createElement("group");
    gElem->setAttribute("name", grouping.groupNames[gi]);
    rootElem->appendChild(gElem);

    Poco::AutoPtr<Element> idsElem = mDoc->createElement("ids");
    idsElem->setAttribute("val", grouping.groups[gi]);
    gElem->appendChild(idsElem);
  }

  // Create pair elements
  for (size_t pi = 0; pi < grouping.pairs.size(); pi++) {
    Poco::AutoPtr<Element> gElem = mDoc->createElement("pair");
    gElem->setAttribute("name", grouping.pairNames[pi]);
    rootElem->appendChild(gElem);

    Poco::AutoPtr<Element> fwElem = mDoc->createElement("forward-group");
    fwElem->setAttribute("val", grouping.groupNames[grouping.pairs[pi].first]);
    gElem->appendChild(fwElem);

    Poco::AutoPtr<Element> bwElem = mDoc->createElement("backward-group");
    bwElem->setAttribute("val", grouping.groupNames[grouping.pairs[pi].second]);
    gElem->appendChild(bwElem);

    Poco::AutoPtr<Element> alphaElem = mDoc->createElement("alpha");
    alphaElem->setAttribute(
        "val", boost::lexical_cast<std::string>(grouping.pairAlphas[pi]));
    gElem->appendChild(alphaElem);
  }

  // Create default group/pair name element
  Poco::AutoPtr<Element> gElem = mDoc->createElement("default");
  gElem->setAttribute("name", grouping.defaultName);
  rootElem->appendChild(gElem);

  writer.writeNode(outFile, mDoc);
}

/**
 * Parses information from the grouping table and saves to Grouping struct.
 *
 * @returns :: Grouping struct storing parsed info
 */
Mantid::API::Grouping MuonGroupingHelper::parseGroupingTable() const {
  Grouping grouping;
  // Parse description
  grouping.description = m_uiForm.groupDescription->text().toStdString();

  // Parse grouping info
  std::vector<int> groupToRow = whichGroupToWhichRow();

  // Resize group arrays
  grouping.groupNames.resize(groupToRow.size());
  grouping.groups.resize(groupToRow.size());

  // Fill group arrays
  for (size_t gi = 0; gi < groupToRow.size(); gi++) {
    grouping.groupNames[gi] =
        m_uiForm.groupTable->item(groupToRow[gi], 0)->text().toStdString();
    grouping.groups[gi] =
        m_uiForm.groupTable->item(groupToRow[gi], 1)->text().toStdString();
  }

  // Parse pair info
  std::vector<int> pairToRow = whichPairToWhichRow();

  // Resize pair arrays
  grouping.pairNames.resize(pairToRow.size());
  grouping.pairs.resize(pairToRow.size());
  grouping.pairAlphas.resize(pairToRow.size());

  // Fill pair arrays
  for (size_t pi = 0; pi < pairToRow.size(); pi++) {
    grouping.pairNames[pi] =
        m_uiForm.pairTable->item(pairToRow[pi], 0)->text().toStdString();

    QComboBox *fwd = static_cast<QComboBox *>(
        m_uiForm.pairTable->cellWidget(pairToRow[pi], 1));
    QComboBox *bwd = static_cast<QComboBox *>(
        m_uiForm.pairTable->cellWidget(pairToRow[pi], 2));

    grouping.pairs[pi] =
        std::make_pair(fwd->currentIndex(), bwd->currentIndex());

    grouping.pairAlphas[pi] =
        m_uiForm.pairTable->item(pairToRow[pi], 3)->text().toDouble();
  }

  // Use currently selected group/pair as default value
  grouping.defaultName =
      m_uiForm.frontGroupGroupPairComboBox->currentText().toStdString();

  return grouping;
}

/**
 * Fills in the grouping table using information from provided Grouping struct.
 *
 * @param grouping :: [input] Grouping struct to use for filling the table
 * @returns Index of default group/group pair
 */
int MuonGroupingHelper::fillGroupingTable(
    const Mantid::API::Grouping &grouping) {
  // Add groups to a table
  for (int gi = 0; gi < static_cast<int>(grouping.groups.size()); gi++) {
    m_uiForm.groupTable->setItem(
        gi, 0, new QTableWidgetItem(grouping.groupNames[gi].c_str()));
    m_uiForm.groupTable->setItem(
        gi, 1, new QTableWidgetItem(grouping.groups[gi].c_str()));
  }

  // Add pairs to the table
  for (int pi = 0; pi < static_cast<int>(grouping.pairs.size()); pi++) {
    // Set the name
    m_uiForm.pairTable->setItem(
        pi, 0, new QTableWidgetItem(grouping.pairNames[pi].c_str()));

    // Set selected forward/backward groups
    QComboBox *fwd =
        static_cast<QComboBox *>(m_uiForm.pairTable->cellWidget(pi, 1));
    fwd->setCurrentIndex(static_cast<int>(grouping.pairs[pi].first));
    QComboBox *bwd =
        static_cast<QComboBox *>(m_uiForm.pairTable->cellWidget(pi, 2));
    bwd->setCurrentIndex(static_cast<int>(grouping.pairs[pi].second));

    // Set alpha
    m_uiForm.pairTable->setItem(
        pi, 3,
        new QTableWidgetItem(
            boost::lexical_cast<std::string>(grouping.pairAlphas[pi]).c_str()));
  }

  // Set description
  m_uiForm.groupDescription->setText(grouping.description.c_str());

  // Select default element
  try {
    return getGroupGroupPairIndex(grouping.defaultName);
  } catch (std::invalid_argument & /*err*/) {
    // Not a big error. Just select the first group in the list
    return 0;
  }
}

/**
 * Get the index of the named Group / Group Pair
 * @param name :: Name of the Group / Group Pair
 * @returns Index of the group/pair of that name
 * @throws std::invalid_argument if there is no group/pair of that name
 */
int MuonGroupingHelper::getGroupGroupPairIndex(const std::string &name) {
  QString qsname(name.c_str());
  for (int i = 0; i < m_uiForm.frontGroupGroupPairComboBox->count(); i++) {
    if (m_uiForm.frontGroupGroupPairComboBox->itemText(i) == qsname) {
      return i;
    }
  }
  // If we reach here, we didn't find any such group
  std::ostringstream message;
  message << "No group/pair with name " << name << " found in list";
  throw std::invalid_argument(message.str());
}

/**
 * create 'map' relating group number to row number in group table
 *
 * @returns :: The 'map' of group number to table row number
 */
std::vector<int> MuonGroupingHelper::whichGroupToWhichRow() const {
  std::vector<int> groupToRow;

  int numRows = m_uiForm.groupTable->rowCount();
  for (int i = 0; i < numRows; i++) {
    // test if group name valid
    QTableWidgetItem *item0 = m_uiForm.groupTable->item(i, 0);
    if (!item0)
      continue;
    if (item0->text().isEmpty())
      continue;

    // test if group IDs valid
    QTableWidgetItem *item1 = m_uiForm.groupTable->item(i, 1);
    QTableWidgetItem *item2 = m_uiForm.groupTable->item(i, 2);
    if (!item1 || !item2)
      continue;
    if (item1->text().isEmpty() || item2->text().isEmpty())
      continue;
    try {
      boost::lexical_cast<int>(item2->text().toStdString().c_str());
    } catch (boost::bad_lexical_cast &) {
      continue;
    }

    groupToRow.push_back(i);
  }
  return groupToRow;
}

/**
 * create 'map' relating pair number to row number in pair table
 *
 * @returns :: The 'map' of pair number to table row number
 */
std::vector<int> MuonGroupingHelper::whichPairToWhichRow() const {
  std::vector<int> pairToRow;

  int numRows = m_uiForm.pairTable->rowCount();
  for (int i = 0; i < numRows; i++) {
    // test if pair name valid
    QTableWidgetItem *item0 = m_uiForm.pairTable->item(i, 0);
    if (!item0)
      continue;
    if (item0->text().isEmpty())
      continue;

    // test if alpha is specified
    QTableWidgetItem *item3 = m_uiForm.pairTable->item(i, 3);
    if (!item3)
      continue;
    if (item3->text().isEmpty())
      continue;

    // test if content in combo boxes
    QComboBox *qwF =
        static_cast<QComboBox *>(m_uiForm.pairTable->cellWidget(i, 1));
    QComboBox *qwB =
        static_cast<QComboBox *>(m_uiForm.pairTable->cellWidget(i, 2));
    if (!qwF || !qwB)
      continue;
    if (qwF->count() < 2 || qwB->count() < 2)
      continue;

    pairToRow.push_back(i);
  }
  return pairToRow;
}
} // namespace Muon
} // namespace CustomInterfaces
} // namespace MantidQt
