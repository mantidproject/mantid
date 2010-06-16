//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadSpice2D.h"
#include "MantidKernel/FileProperty.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "Poco/Path.h"
#include "Poco/StringTokenizer.h"
#include "Poco/DOM/DOMParser.h"
#include "Poco/DOM/Document.h"
#include "Poco/DOM/Element.h"
#include "Poco/DOM/NodeList.h"
#include "Poco/DOM/Node.h"
#include "Poco/DOM/Text.h"
//-----------------------------------------------------------------------

using Poco::XML::DOMParser;
using Poco::XML::Document;
using Poco::XML::Element;
using Poco::XML::NodeList;
using Poco::XML::Node;
using Poco::XML::Text;


namespace Mantid
{
  namespace DataHandling
  {
    // Parse string and convert to numeric type
    template <class T>
    bool from_string(T& t, const std::string& s, std::ios_base& (*f)(std::ios_base&))
    {
      std::istringstream iss(s);
      return !(iss >> f >> t).fail();
    }

    // Register the algorithm into the AlgorithmFactory
    DECLARE_ALGORITHM(LoadSpice2D)

    /// Constructor
    LoadSpice2D::LoadSpice2D() {}

    /// Destructor
    LoadSpice2D::~LoadSpice2D() {}

    /// Overwrites Algorithm Init method.
    void LoadSpice2D::init()
    {
      std::vector<std::string> exts;
      exts.push_back("xml");
      declareProperty(new Kernel::FileProperty("Filename", "", Kernel::FileProperty::Load, exts),
          "The name of the input xml file to load");
      declareProperty(new API::WorkspaceProperty<API::Workspace>("OutputWorkspace", "",
          Kernel::Direction::Output), "The name of the Output workspace");
    }

    /// Overwrites Algorithm exec method
    void LoadSpice2D::exec()
    {
      std::string fileName = getPropertyValue("Filename");
      // Set up the DOM parser and parse xml file
      DOMParser pParser;
      Document* pDoc;
      try
      {
        pDoc = pParser.parse(fileName);
      } catch (...)
      {
        throw Kernel::Exception::FileError("Unable to parse File:", fileName);
      }
      // Get pointer to root element
      Element* pRootElem = pDoc->documentElement();
      if (!pRootElem->hasChildNodes())
      {
        throw Kernel::Exception::NotFoundError("No root element in Spice XML file", fileName);
      }
      Element* sasEntryElem = pRootElem->getChildElement("Header");
      throwException(sasEntryElem, "Header", fileName);

      // Read in scan title
      Element* element = sasEntryElem->getChildElement("Scan_Title");
      throwException(element, "Scan_Title", fileName);
      std::string wsTitle = element->innerText();

      // Read in instrument name
      element = sasEntryElem->getChildElement("Instrument");
      throwException(element, "Instrument", fileName);
      std::string instrument = element->innerText();

      // Read in the detector dimensions
      int numberXPixels;
      element = sasEntryElem->getChildElement("Number_of_X_Pixels");
      throwException(sasEntryElem, "Number_of_X_Pixels", fileName);
      std::stringstream x(element->innerText());
      x >> numberXPixels;

      int numberYPixels;
      element = sasEntryElem->getChildElement("Number_of_Y_Pixels");
      throwException(sasEntryElem, "Number_of_Y_Pixels", fileName);
      std::stringstream y(element->innerText());
      y >> numberYPixels;

      // Read in the data image
      Element* sasDataElem = pRootElem->getChildElement("Data");
      throwException(sasDataElem, "Data", fileName);

      // Read in the data buffer
      element = sasDataElem->getChildElement("Detector");
      throwException(element, "Detector", fileName);
      std::string data_str = element->innerText();

      // Create the output workspace

      // Number of bins: we use a single TOF bin
      int nBins = 1;
      // Number of detectors: should be pulled from the geometry description. Use detector pixels for now.
      int numSpectra = numberXPixels*numberYPixels;
      //const int numSpectra = 1;

      DataObjects::Workspace2D_sptr ws = boost::dynamic_pointer_cast<DataObjects::Workspace2D>(
          API::WorkspaceFactory::Instance().create("Workspace2D", numSpectra, nBins, nBins));
      ws->setTitle(wsTitle);
      ws->getAxis(0)->unit() = Kernel::UnitFactory::Instance().create("TOF");
      ws->setYUnit("Count");
      API::Workspace_sptr workspace = boost::static_pointer_cast<API::Workspace>(ws);
      setProperty("OutputWorkspace", workspace);

      // Parse out each pixel. Pixels can be separated by white space, a tab, or an end-of-line character
      Poco::StringTokenizer pixels(data_str, " \n\t", Poco::StringTokenizer::TOK_TRIM | Poco::StringTokenizer::TOK_IGNORE_EMPTY);
      Poco::StringTokenizer::Iterator pixel = pixels.begin();

      int ipixel = 0;
      // Check that we don't keep within the size of the workspace
      int pixelcount = pixels.count();
      if( pixelcount > numSpectra )
      {
        throw Kernel::Exception::FileError("Inconsitent data set: "
            "There were more data pixels found than declared in the Spice XML meta-data.", fileName);
      }

      while (pixel != pixels.end())
      {
        //int ix = ipixel%npixelsx;
        //int iy = (int)ipixel/npixelsx;

        // Get the count value and assign it to the right bin
        double count;
        from_string<double>(count, *pixel, std::dec);

        // Load workspace data
        MantidVec& X = ws->dataX(ipixel);
        MantidVec& Y = ws->dataY(ipixel);
        MantidVec& E = ws->dataE(ipixel);

        X[0] = 0.0;
        Y[0] = count;
        // Data uncertainties, computed according to the HFIR/IGOR reduction code
        E[0] = sqrt( 0.5 + fabs( count - 0.5 ));
        // The following is what I would suggest instead...
        // E[0] = count > 0 ? sqrt((double)count) : 0.0;

        ++pixel;
        ipixel++;
      }

      // run load instrument
      runLoadInstrument(instrument, ws);
    }

    /** Run the sub-algorithm LoadInstrument (as for LoadRaw)
     * @param inst_name The name written in the Nexus file
     * @param localWorkspace The workspace to insert the instrument into
     */
    void LoadSpice2D::runLoadInstrument(const std::string & inst_name,
        DataObjects::Workspace2D_sptr localWorkspace)
    {
      // Determine the search directory for XML instrument definition files (IDFs)
      std::string directoryName = Kernel::ConfigService::Instance().getString(
          "instrumentDefinition.directory");
      if (directoryName.empty())
      {
        // This is the assumed deployment directory for IDFs, where we need to be relative to the
        // directory of the executable, not the current working directory.
        directoryName = Poco::Path(Mantid::Kernel::ConfigService::Instance().getBaseDir()).resolve(
            "../Instrument").toString();
      }

      // For Nexus Mantid processed, Instrument XML file name is read from nexus
      std::string instrumentID = inst_name;
      // force ID to upper case
      std::transform(instrumentID.begin(), instrumentID.end(), instrumentID.begin(), toupper);
      std::string fullPathIDF = directoryName + "/" + instrumentID + "_Definition.xml";

      API::IAlgorithm_sptr loadInst = createSubAlgorithm("LoadInstrument");

      // Now execute the sub-algorithm. Catch and log any error, but don't stop.
      try
      {
        loadInst->setPropertyValue("Filename", fullPathIDF);
        loadInst->setProperty<API::MatrixWorkspace_sptr> ("Workspace", localWorkspace);
        loadInst->execute();
      } catch (std::invalid_argument&)
      {
        g_log.information("Invalid argument to LoadInstrument sub-algorithm");
      } catch (std::runtime_error&)
      {
        g_log.information("Unable to successfully run LoadInstrument sub-algorithm");
      }

    }

    /* This method throws not found error if a element is not found in the xml file
     * @param elem pointer to  element
     * @param name  element name
     * @param fileName xml file name
     */
    void LoadSpice2D::throwException(Poco::XML::Element* elem, const std::string & name,
        const std::string& fileName)
    {
      if (!elem)
      {
        throw Kernel::Exception::NotFoundError(name + " element not found in Spice XML file", fileName);
      }
    }

}
}
