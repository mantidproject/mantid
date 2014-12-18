#include "MantidAlgorithms/MaskBinsFromTable.h"
#include "MantidKernel/System.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidGeometry/Instrument.h"

#include <boost/algorithm/string/predicate.hpp>


using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

using namespace std;

namespace Mantid
{
namespace Algorithms
{

  DECLARE_ALGORITHM(MaskBinsFromTable)

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  MaskBinsFromTable::MaskBinsFromTable()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  MaskBinsFromTable::~MaskBinsFromTable()
  {
  }
  
  //----------------------------------------------------------------------------------------------

  //----------------------------------------------------------------------------------------------
  void MaskBinsFromTable::init()
  {
    this->declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input, boost::make_shared<HistogramValidator>()),
        "Input Workspace to mask bins. ");
    this->declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output),
        "Output Workspace with bins masked.");
    this->declareProperty(new WorkspaceProperty<DataObjects::TableWorkspace>("MaskingInformation", "", Direction::Input),
        "Input TableWorkspace containing parameters, XMin and XMax and either SprectaList or DetectorIDsList");

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Main execution body
    */
  void MaskBinsFromTable::exec()
  {
    // Process input properties
    MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
    TableWorkspace_sptr paramWS = getProperty("MaskingInformation");

    processMaskBinWorkspace(paramWS, inputWS);

    // Mask bins for all
    maskBins(inputWS);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Call MaskBins
    * @param dataws :: MatrixWorkspace to mask bins for
    */
  void MaskBinsFromTable::maskBins(API::MatrixWorkspace_sptr dataws)
  {
    bool firstloop = true;
    API::MatrixWorkspace_sptr outputws;

    size_t numcalls = m_xminVec.size();

    g_log.debug() << "There will be " << numcalls << " calls to MaskBins" << "\n";
    for (size_t ib = 0; ib < numcalls; ++ib)
    {
      // Construct algorithm
      IAlgorithm_sptr maskbins = this->createChildAlgorithm("MaskBins", 0, 0.3, true);
      maskbins->initialize();

      // Set properties
      g_log.debug() << "Input to MaskBins: SpetraList = '" << m_spectraVec[ib] << "'; Xmin = "
                    << m_xminVec[ib] << ", Xmax = " << m_xmaxVec[ib] << ".\n";

      if (firstloop)
      {
        maskbins->setProperty("InputWorkspace", dataws);
        firstloop = false;
      }
      else
      {
        if (!outputws)
          throw runtime_error("Programming logic error.");
        maskbins->setProperty("InputWorkspace", outputws);
      }
      maskbins->setProperty("OutputWorkspace", this->getPropertyValue("OutputWorkspace"));
      maskbins->setPropertyValue("SpectraList", m_spectraVec[ib]);
      maskbins->setProperty("XMin", m_xminVec[ib]);
      maskbins->setProperty("XMax", m_xmaxVec[ib]);

      bool isexec = maskbins->execute();
      if (!isexec)
      {
        stringstream errmsg;
        errmsg << "MaskBins() is not executed for row " << ib << "\n";
        g_log.error(errmsg.str());
        throw std::runtime_error(errmsg.str());
      }
      else
      {
        g_log.debug("MaskBins() is executed successfully.");
      }

      outputws = maskbins->getProperty("OutputWorkspace");
      if (!outputws)
      {
        stringstream errmsg;
        errmsg << "OutputWorkspace cannot be obtained from algorithm row " << ib << ". ";
        g_log.error(errmsg.str());
        throw std::runtime_error(errmsg.str());
      }
    }

    //
    g_log.debug() << "About to set to output." << "\n";
    setProperty("OutputWorkspace", outputws);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Process input Mask bin TableWorkspace.
    * It will convert detector IDs list to spectra list
    * @param masktblws :: TableWorkspace for mask bins
    * @param dataws :: MatrixWorkspace to mask
    */
  void MaskBinsFromTable::processMaskBinWorkspace(TableWorkspace_sptr masktblws, API::MatrixWorkspace_sptr dataws)
  {
    // Check input
    if (!masktblws)
      throw std::invalid_argument("Input workspace is not a table workspace.");
    g_log.debug() << "Lines of parameters workspace = " << masktblws->rowCount() << std::endl;

    // Check column names type and sequence
    vector<std::string> colnames = masktblws->getColumnNames();

    // check colum name order
    id_xmin = -1;
    id_xmax = -1;
    id_spec = -1;
    id_dets = -1;
    m_useDetectorID = false;
    m_useSpectrumID = false;

    for (int i = 0; i < static_cast<int>(colnames.size()); ++i)
    {
      string colname = colnames[i];
      transform(colname.begin(), colname.end(), colname.begin(), ::tolower);
      if (colname.compare("xmin") == 0)
        id_xmin = i;
      else if (colname.compare("xmax") == 0)
        id_xmax = i;
      else if (boost::algorithm::starts_with(colname, "spec"))
      {
        id_spec = i;
      }
      else if (boost::algorithm::starts_with(colname, "detectorid"))
      {
        id_dets = i;
      }
      else
      {
        g_log.warning() << "In TableWorkspace " << masktblws->name() << ", column " << i << " with name "
                        << colname << " is not used by MaskBinsFromTable.";
      }
    }

    if (id_xmin < 0 || id_xmax < 0 || id_xmin == id_xmax)
      throw runtime_error("Either Xmin nor Xmax is not given. ");
    if (id_spec == id_dets)
      throw runtime_error("Neither SpectraList nor DetectorIDList is given.");
    else if (id_dets >= 0)
      m_useDetectorID = true;
    else
      m_useSpectrumID = true;

    // Construct vectors for xmin, xmax and spectra-list
    size_t numrows = masktblws->rowCount();
    for (size_t i = 0; i < numrows; ++i)
    {
      double xmin = masktblws->cell<double>(i, static_cast<size_t>(id_xmin));
      double xmax = masktblws->cell<double>(i, static_cast<size_t>(id_xmax));

      string spectralist;
      if (m_useSpectrumID)
      {
        spectralist = masktblws->cell<string>(i, static_cast<size_t>(id_spec));
      }
      else
      {
        // Convert detectors list to spectra list
        string detidslist = masktblws->cell<string>(i, static_cast<size_t>(id_dets));
        spectralist = convertToSpectraList(dataws, detidslist);
      }

      g_log.debug() << "Row " << i << " XMin = " << xmin << "  XMax = " << xmax << " SpectraList = " << spectralist
                    << ".\n";

      // Store to class variables
      m_xminVec.push_back(xmin);
      m_xmaxVec.push_back(xmax);
      m_spectraVec.push_back(spectralist);
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Convert a list of detector IDs list (string) to a list of spectra/workspace indexes list
    * @param dataws :: MatrixWorkspace to mask
    * @param detidliststr :: list of detector IDs in string format
    * @return :: list of spectra/workspace index IDs in string format
    */
  std::string MaskBinsFromTable::convertToSpectraList(API::MatrixWorkspace_sptr dataws, std::string detidliststr)
  {
    // Use array property to get a list of detectors
    vector<int> detidvec;
    ArrayProperty<int> parser("detids", detidliststr);

    size_t numitems = parser.size();
    for (size_t i = 0;i < numitems; ++i)
    {
      detidvec.push_back(parser.operator ()()[i]);
      g_log.debug() << "[DB] DetetorID = " << detidvec.back() << " to mask.";
    }

    // Get workspace index from
    vector<size_t> wsindexvec;

    detid2index_map refermap = dataws->getDetectorIDToWorkspaceIndexMap(false);
    for (size_t i = 0; i < numitems; ++i)
    {
      detid_t detid = detidvec[i];
      detid2index_map::const_iterator fiter = refermap.find(detid);
      if (fiter != refermap.end())
      {
        size_t wsindex = fiter->second;
        wsindexvec.push_back(wsindex);
      }
      else
      {
        g_log.warning() << "Detector ID " << detid << " cannot be mapped to any workspace index/spectrum." << ".\n";
      }
    }

    // Sort the vector
    if (wsindexvec.size() == 0)
      throw runtime_error("There is no spectrum found for input detectors list.");

    sort(wsindexvec.begin(), wsindexvec.end());

    // Convert the spectra to a string
    stringstream rss;
    size_t headid = wsindexvec[0];
    size_t previd = wsindexvec[0];

    for (size_t i = 1; i < wsindexvec.size(); ++i)
    {
      size_t currid = wsindexvec[i];
      if (currid > previd+1)
      {
        // skipped.  previous region should be written, and a new region start
        if (headid == previd)
          rss << headid << ", ";
        else
          rss << headid << "-" << previd << ", ";
        headid = currid;
      }
      else
      {
        // Equal to continuous
        ;
      }

      // Update
      previd = currid;
    }

    // Finalize
    if (headid == previd)
      rss << headid;
    else
      rss << headid << "-" << previd;

    string spectraliststr(rss.str());

    return spectraliststr;
  }

} // namespace Mantid
} // namespace Algorithms
