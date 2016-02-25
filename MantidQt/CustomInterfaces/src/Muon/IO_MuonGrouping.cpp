//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <fstream>

#include "MantidQtCustomInterfaces/Muon/MuonAnalysis.h"
#include "MantidQtCustomInterfaces/Muon/IO_MuonGrouping.h"

#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidQtAPI/UserSubWindow.h"

#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>

#include <Poco/DOM/Document.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/DOMWriter.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/Text.h>

#ifdef _MSC_VER
// Disable a flood of warnings from Poco about inheriting from std::basic_istream
  // See http://connect.microsoft.com/VisualStudio/feedback/details/733720/inheriting-from-std-fstream-produces-c4250-warning
  #pragma warning( push )
  #pragma warning( disable : 4250 )
  #include <Poco/XML/XMLWriter.h>
  #pragma warning( pop ) 
#endif

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
 * Save grouping to the XML file specified.
 *
 * @param        g :: Struct with grouping information
 * @param filename :: XML filename where information will be saved
 */
void saveGroupingToXML(const Mantid::API::Grouping &g,
                       const std::string &filename) {
  std::ofstream outFile(filename.c_str());
  if (!outFile)
    throw Mantid::Kernel::Exception::FileError("Unable to open output file", filename);

  DOMWriter writer;
  writer.setNewLine("\n");
  writer.setOptions(XMLWriter::PRETTY_PRINT);

  Poco::AutoPtr<Poco::XML::Document> mDoc = new Document();

  // Create root element with a description
  Poco::AutoPtr<Element> rootElem = mDoc->createElement("detector-grouping");
  rootElem->setAttribute("description", g.description);
  mDoc->appendChild(rootElem);

  // Create group elements
  for (size_t gi = 0; gi < g.groups.size(); gi++)
  {
    Poco::AutoPtr<Element> gElem = mDoc->createElement("group");
    gElem->setAttribute("name", g.groupNames[gi]);
    rootElem->appendChild(gElem);

    Poco::AutoPtr<Element> idsElem = mDoc->createElement("ids");
    idsElem->setAttribute("val", g.groups[gi]);
    gElem->appendChild(idsElem);
  }

  // Create pair elements
  for (size_t pi = 0; pi < g.pairs.size(); pi++)
  {
    Poco::AutoPtr<Element> gElem = mDoc->createElement("pair");
    gElem->setAttribute("name", g.pairNames[pi]);
    rootElem->appendChild(gElem);

    Poco::AutoPtr<Element> fwElem = mDoc->createElement("forward-group");
    fwElem->setAttribute("val", g.groupNames[g.pairs[pi].first]);
    gElem->appendChild(fwElem);

    Poco::AutoPtr<Element> bwElem = mDoc->createElement("backward-group");
    bwElem->setAttribute("val", g.groupNames[g.pairs[pi].second]);
    gElem->appendChild(bwElem);

    Poco::AutoPtr<Element> alphaElem = mDoc->createElement("alpha");
    alphaElem->setAttribute("val", boost::lexical_cast<std::string>(g.pairAlphas[pi]));
    gElem->appendChild(alphaElem);
  } 

  // Create default group/pair name element
  Poco::AutoPtr<Element> gElem = mDoc->createElement("default");
  gElem->setAttribute("name", g.defaultName);
  rootElem->appendChild(gElem);

  writer.writeNode(outFile, mDoc);
}

/**
 * Parses information from the grouping table and saves to Grouping struct.
 *
 * @param form :: Muon Analysis UI containing table widgets
 * @param    g :: Grouping struct to store parsed info to
 */
void parseGroupingTable(const Ui::MuonAnalysis &form,
                        Mantid::API::Grouping &g) {
  // Parse description
  g.description = form.groupDescription->text().toStdString();

  // Parse grouping info
  std::vector<int> groupToRow = whichGroupToWhichRow(form);

  // Resize group arrays
  g.groupNames.resize(groupToRow.size());
  g.groups.resize(groupToRow.size());

  // Fill group arrays
  for (size_t gi = 0; gi < groupToRow.size(); gi++)
  {
    g.groupNames[gi] = form.groupTable->item(groupToRow[gi],0)->text().toStdString();
    g.groups[gi] = form.groupTable->item(groupToRow[gi],1)->text().toStdString();
  }

  // Parse pair info
  std::vector<int> pairToRow = whichPairToWhichRow(form);

  // Resize pair arrays
  g.pairNames.resize(pairToRow.size());
  g.pairs.resize(pairToRow.size());
  g.pairAlphas.resize(pairToRow.size());

  // Fill pair arrays
  for (size_t pi = 0; pi < pairToRow.size(); pi++)
  {
    g.pairNames[pi] = form.pairTable->item(pairToRow[pi],0)->text().toStdString();

    QComboBox* fwd = static_cast<QComboBox*>(form.pairTable->cellWidget(pairToRow[pi],1));
    QComboBox* bwd = static_cast<QComboBox*>(form.pairTable->cellWidget(pairToRow[pi],2));

    g.pairs[pi] = std::make_pair(fwd->currentIndex(), bwd->currentIndex());

    g.pairAlphas[pi] = form.pairTable->item(pairToRow[pi],3)->text().toDouble();
  } 

  // Use currently selected group/pair as default value
  g.defaultName = form.frontGroupGroupPairComboBox->currentText().toStdString();
}

/**
 * Fills in the grouping table using information from provided Grouping struct.
 *
 * @param    g :: Grouping struct to use for filling the table
 * @param form :: Muon Analysis UI containing table widgets
 */
void fillGroupingTable(const Mantid::API::Grouping& g, Ui::MuonAnalysis& form)
{
  // Add groups to a table
  for(int gi = 0; gi < static_cast<int>(g.groups.size()); gi++)
  {
    form.groupTable->setItem(gi, 0, new QTableWidgetItem(g.groupNames[gi].c_str()));
    form.groupTable->setItem(gi, 1, new QTableWidgetItem(g.groups[gi].c_str()));
  }

  // Add pairs to the table
  for(int pi = 0; pi < static_cast<int>(g.pairs.size()); pi++)
  {
    // Set the name
    form.pairTable->setItem(pi, 0, new QTableWidgetItem(g.pairNames[pi].c_str()));

    // Set selected forward/backward groups
    QComboBox* fwd = static_cast<QComboBox*>(form.pairTable->cellWidget(pi,1));
    fwd->setCurrentIndex(static_cast<int>(g.pairs[pi].first));
    QComboBox* bwd = static_cast<QComboBox*>(form.pairTable->cellWidget(pi,2));
    bwd->setCurrentIndex(static_cast<int>(g.pairs[pi].second));

    // Set alpha
    form.pairTable->setItem(pi, 3, 
      new QTableWidgetItem(boost::lexical_cast<std::string>(g.pairAlphas[pi]).c_str()));
  }

  // Set description
  form.groupDescription->setText(g.description.c_str());

  // Select default element
  setGroupGroupPair(form, g.defaultName);
}

/**
 * Set Group / Group Pair name
 *
 * @param m_uiForm :: The UI form
 * @param name :: Name you want to set the front Group / Group Pair name to
 */
void setGroupGroupPair(Ui::MuonAnalysis& m_uiForm, const std::string& name)
{
  QString qsname(name.c_str());
  for (int i = 0; i < m_uiForm.frontGroupGroupPairComboBox->count(); i++)
  {
    if ( m_uiForm.frontGroupGroupPairComboBox->itemText(i) == qsname )
    {
      m_uiForm.frontGroupGroupPairComboBox->setCurrentIndex(i);
      return;
    }
  }
}

/**
 * Groups the workspace according to grouping provided.
 *
 * @param ws :: Workspace to group
 * @param  g :: The grouping information
 * @return Sptr to created grouped workspace
 */
MatrixWorkspace_sptr groupWorkspace(MatrixWorkspace_const_sptr ws,
                                    const Mantid::API::Grouping &g) {
  // As I couldn't specify multiple groups for GroupDetectors, I am going down quite a complicated
  // route - for every group distinct grouped workspace is created using GroupDetectors. These
  // workspaces are then merged into the output workspace.

  // Create output workspace
  MatrixWorkspace_sptr outWs =
    WorkspaceFactory::Instance().create(ws, g.groups.size(), ws->readX(0).size(), ws->blocksize());

  for(size_t gi = 0; gi < g.groups.size(); gi++)
  {
    Mantid::API::IAlgorithm_sptr alg = AlgorithmManager::Instance().create("GroupDetectors");
    alg->setChild(true); // So Output workspace is not added to the ADS
    alg->initialize();
    alg->setProperty("InputWorkspace", boost::const_pointer_cast<MatrixWorkspace>(ws));
    alg->setPropertyValue("SpectraList", g.groups[gi]);
    alg->setPropertyValue("OutputWorkspace", "grouped"); // Is not actually used, just to make validators happy
    alg->execute();

    MatrixWorkspace_sptr grouped = alg->getProperty("OutputWorkspace");

    // Copy the spectrum
    *(outWs->getSpectrum(gi)) = *(grouped->getSpectrum(0));

    // Update spectrum number
    outWs->getSpectrum(gi)->setSpectrumNo(static_cast<specid_t>(gi));

    // Copy to the output workspace
    outWs->dataY(gi) = grouped->readY(0);
    outWs->dataX(gi) = grouped->readX(0);
    outWs->dataE(gi) = grouped->readE(0);
  }

  return outWs;
}

/**
 * create 'map' relating group number to row number in group table
 *
 * @param m_uiForm :: The UI form
 * @returns :: The 'map' of group number to table row number
 */
std::vector<int> whichGroupToWhichRow(const Ui::MuonAnalysis &m_uiForm) {
  std::vector<int> groupToRow;

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
    try
    {
       boost::lexical_cast<int>(item2->text().toStdString().c_str());
    }  catch (boost::bad_lexical_cast&)
    {
      continue;
    }

    groupToRow.push_back(i);
  }
  return groupToRow;
}


/**
 * create 'map' relating pair number to row number in pair table
 *
 * @param m_uiForm :: The UI form
 * @returns :: The 'map' of pair number to table row number
 */
std::vector<int> whichPairToWhichRow(const Ui::MuonAnalysis &m_uiForm) {
  std::vector<int> pairToRow;

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

    // test if content in combo boxes
    QComboBox* qwF = static_cast<QComboBox*>(m_uiForm.pairTable->cellWidget(i,1));
    QComboBox* qwB = static_cast<QComboBox*>(m_uiForm.pairTable->cellWidget(i,2));
    if (!qwF || !qwB)
      continue;
    if ( qwF->count() < 2 || qwB->count() < 2 )
      continue;

    pairToRow.push_back(i);
  }
  return pairToRow;
}

/**
 * Convert a grouping table to a grouping struct.
 * @param table :: A table to convert
 * @return Grouping info
 */
boost::shared_ptr<Mantid::API::Grouping>
tableToGrouping(ITableWorkspace_sptr table) {
  auto grouping = boost::make_shared<Mantid::API::Grouping>();

  for ( size_t row = 0; row < table->rowCount(); ++row )
  {
    std::vector<int> detectors = table->cell< std::vector<int> >(row,0);

    // toString() expects the sequence to be sorted
    std::sort( detectors.begin(), detectors.end() );

    // Convert to a range string, i.e. 1-5,6-8,9
    std::string detectorRange = Strings::toString(detectors);

    grouping->groupNames.push_back(boost::lexical_cast<std::string>(row + 1));
    grouping->groups.push_back(detectorRange);
  }

  // If we have 2 groups only - create a longitudinal pair
  if ( grouping->groups.size() == 2 )
  {
    grouping->pairNames.emplace_back("long");
    grouping->pairAlphas.push_back(1.0);
    grouping->pairs.emplace_back(0, 1);
  }

  return grouping;
}

/**
 * Converts a grouping information to a grouping table. Discard all the information not stored in a
 * table - group names, pairing, description.
 * @param grouping :: Grouping information to convert
 * @return A grouping table as accepted by MuonGroupDetectors
 */
ITableWorkspace_sptr
groupingToTable(boost::shared_ptr<Mantid::API::Grouping> grouping) {
  auto newTable = boost::dynamic_pointer_cast<ITableWorkspace>(
      WorkspaceFactory::Instance().createTable("TableWorkspace") );

  newTable->addColumn("vector_int", "Detectors");

  for ( auto it = grouping->groups.begin(); it != grouping->groups.end(); ++it )
  {
    TableRow newRow = newTable->appendRow();
    newRow << Strings::parseRange(*it);
  }

  return newTable;
}

/**
 * Returns a "dummy" grouping which a single group with all the detectors in it.
 * @param instrument :: Instrument we want a dummy grouping for
 * @return Grouping information
 */
boost::shared_ptr<Mantid::API::Grouping>
getDummyGrouping(Instrument_const_sptr instrument) {
  // Group with all the detectors
  std::ostringstream all;
  all << "1-" << instrument->getNumberDetectors();

  auto dummyGrouping = boost::make_shared<Mantid::API::Grouping>();
  dummyGrouping->description = "Dummy grouping";
  dummyGrouping->groupNames.emplace_back("all");
  dummyGrouping->groups.push_back(all.str());
  return dummyGrouping;
}

}
}
}
