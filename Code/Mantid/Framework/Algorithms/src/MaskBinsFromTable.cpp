#include "MantidAlgorithms/MaskBinsFromTable.h"
#include "MantidKernel/System.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TableRow.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

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
  
  void MaskBinsFromTable::initDocs()
  {
    this->setWikiSummary("Mask bins from a table workspace. ");
    this->setOptionalMessage("Mask bins from a table workspace. ");
  }

  void MaskBinsFromTable::init()
  {
    this->declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::Input, boost::make_shared<HistogramValidator>()),
        "Input Workspace to mask bins. ");
    this->declareProperty(new WorkspaceProperty<>("OutputWorkspace","",Direction::Output),
        "Output Workspace with bins masked.");
    this->declareProperty(new WorkspaceProperty<DataObjects::TableWorkspace>("MaskingInformation", "", Direction::Input),
        "Input TableWorkspace containing parameters, SpectraList, XMin and XMax.");

    return;
  }

  void MaskBinsFromTable::exec()
  {
    MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
    DataObjects::TableWorkspace_sptr paramWS = getProperty("MaskingInformation");

    // 1. Check input table workspace and column order
    g_log.debug() << "Lines of parameters workspace = " << paramWS->rowCount() << std::endl;

    if (!paramWS)
    {
    	throw std::invalid_argument("Input table workspace is not accepted.");
    }
    else
    {

    }

    // 2. Loop over all rows
    bool firstloop = true;
    API::MatrixWorkspace_sptr outputws;

    for (size_t ib = 0; ib < paramWS->rowCount(); ++ib)
    {
      API::TableRow therow = paramWS->getRow(ib);
      double xmin, xmax;
      std::string speclist;
      therow >> xmin >> xmax >> speclist;

      g_log.debug() << "Row " << ib << " XMin = " << xmin << "  XMax = " << xmax << " SpectraList = " << speclist << std::endl;

      API::IAlgorithm_sptr maskbins = this->createSubAlgorithm("MaskBins", 0, 0.3, true);
      maskbins->initialize();
      if (firstloop)
      {
        maskbins->setPropertyValue("InputWorkspace", this->getPropertyValue("InputWorkspace"));
        firstloop = false;
      }
      else
      {
        maskbins->setProperty("InputWorkspace", outputws);
      }
      maskbins->setProperty("OutputWorkspace", this->getPropertyValue("OutputWorkspace"));
      maskbins->setPropertyValue("SpectraList", speclist);
      maskbins->setProperty("XMin", xmin);
      maskbins->setProperty("XMax", xmax);

      bool isexec = maskbins->execute();
      if (!isexec)
      {
        g_log.error() << "MaskBins() is not executed for row " << ib << std::endl;
        throw std::runtime_error("MaskBins() is not executed");
      }

      outputws = maskbins->getProperty("OutputWorkspace");
      if (!outputws)
      {
        g_log.error() << "OutputWorkspace is not retrieved for row " << ib << ". " << std::endl;
        throw std::runtime_error("OutputWorkspace is not got from MaskBins");
      }
    }

    setProperty("OutputWorkspace", outputws);

    return;
  }



} // namespace Mantid
} // namespace Algorithms
