//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/SaveCanSAS1D.h"
#include "MantidKernel/FileProperty.h"
#include "MantidGeometry/IComponent.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/MantidVersion.h"
#include "MantidAPI/Sample.h"
#include <boost/shared_ptr.hpp>
#include <fstream>  

//-----------------------------------------------------------------------------
using namespace Mantid::Geometry;
using namespace Poco::XML;

namespace Mantid
{
  namespace DataHandling
  {

    // Register the algorithm into the AlgorithmFactory
    DECLARE_ALGORITHM(SaveCanSAS1D)

    /// constructor
    SaveCanSAS1D::SaveCanSAS1D()
    {}

    /// destructor
    SaveCanSAS1D::~SaveCanSAS1D()
    {}

    /// Overwrites Algorithm method.
    void SaveCanSAS1D::init()
    {
      declareProperty(new API::WorkspaceProperty<>("InputWorkspace", "", Kernel::Direction::Input,
        new API::WorkspaceUnitValidator<>("MomentumTransfer")),
        "The input workspace, which must be in units of Q");
      std::vector<std::string> exts;
      exts.push_back("xml");
      declareProperty(new Kernel::FileProperty("Filename", "", Kernel::FileProperty::Save, exts),
        "The name of the xml file to save");

      std::vector<std::string> radiation_source;
      radiation_source.push_back("Spallation Neutron Source");
      radiation_source.push_back("Pulsed Reactor Neutron Source");
      radiation_source.push_back("Reactor Neutron Source");
      radiation_source.push_back("Synchrotron X-ray Source");
      radiation_source.push_back("Pulsed Muon Source");
      radiation_source.push_back("Rotating Anode X-ray");
      radiation_source.push_back("Fixed Tube X-ray");
      radiation_source.push_back("neutron");
      radiation_source.push_back("x-ray");
      radiation_source.push_back("muon");
      radiation_source.push_back("electron");
      declareProperty("Radiation Source", "Spallation Neutron Source", new Kernel::ListValidator(
        radiation_source));
    }

    /// Overwrites Algorithm method
    void SaveCanSAS1D::exec()
    {
      m_workspace = getProperty("InputWorkspace");
      if(!m_workspace)
      {
        throw std::invalid_argument("Invalid inputworkspace ,Error in  SaveCanSAS1D");
      }
      if (m_workspace->getNumberHistograms() > 1)
      {
        throw std::invalid_argument("Error in  SaveCanSAS1D");
      }
      std::string fileName = getPropertyValue("FileName");
      std::ofstream outFile(fileName.c_str());
      if (!outFile)
      {
        throw Kernel::Exception::FileError("Unable to open file:", fileName);
      }

      // Just write out header manually, because I can't see a way to do stylesheet part in Poco
      outFile << "<?xml version=\"1.0\"?>\n"
        << "<?xml-stylesheet type=\"text/xsl\" href=\"cansasxml-html.xsl\" ?>\n";


      std::string sasroot="";
      createSASRootElement(sasroot);
      outFile<<sasroot;

      std::string sasEntry="\n\t<SASentry name=\"workspace\">";
      outFile<<sasEntry;

      std::string sasTitle;
      createSASTitleElement(sasTitle);
      outFile<<sasTitle;

      std::string sasRun;
      createSASRunElement(sasRun);
      outFile<<sasRun;

      std::string dataUnit = m_workspace->YUnitLabel();
      //look for xml special characters and replace with entity refrence
      searchandreplaceSpecialChars(dataUnit);


      std::string sasData;
      createSASDataElement(sasData);
      outFile<<sasData;

      std::string sasSample;
      createSASSampleElement(sasSample);
      outFile<<sasSample;

      std::string sasInstr="\n\t\t<SASinstrument>";
      outFile<<sasInstr;
      std::string sasInstrName="\n\t\t\t<name>";
      std::string instrname=m_workspace->getInstrument()->getName();
      //look for xml special characters and replace with entity refrence
      searchandreplaceSpecialChars(instrname);
      sasInstrName+=instrname;
      sasInstrName+="</name>";
      outFile<<sasInstrName;

      std::string sasSource;
      createSASSourceElement(sasSource);
      outFile<<sasSource;

      std::string sasCollimation="\n\t\t\t<SAScollimation/>";
      outFile<<sasCollimation;


      try
      {
        std::string sasDet;
        createSASDetectorElement(sasDet);
        outFile<<sasDet;
      }
      catch(Kernel::Exception::NotFoundError&)
      {
        outFile.close();
        throw;
      }
      catch(std::runtime_error& )
      {
        outFile.close();
        throw ;
      }

      sasInstr="\n\t\t</SASinstrument>";
      outFile<<sasInstr;

      std::string sasProcess;
      createSASProcessElement(sasProcess);
      outFile<<sasProcess;

      std::string sasNote="\n\t\t<SASnote>";
      sasNote+="\n\t\t</SASnote>";
      outFile<<sasNote;

      sasEntry="\n\t</SASentry>";
      outFile<<sasEntry;
      sasroot="\n</SASroot>";
      outFile<<sasroot;
      outFile.close();
    }


    /** This method search for xml special characters in the input string
    * and  replaces this with xml entity reference
    *@param input -input string 
    */
    void SaveCanSAS1D::searchandreplaceSpecialChars(std::string &input)
    {
      std::string specialchars="&<>'\"";
      std::string::size_type searchIndex=0;
      std::string::size_type  findIndex;
      for(std::string::size_type i=0;i<specialchars.size();++i)
      {
        while(searchIndex<input.length())
        {
          findIndex=input.find(specialchars[i],searchIndex);
          if(findIndex!=std::string::npos)
          {
            //replace with xml entity refrence
            replacewithEntityReference(input,findIndex);
            searchIndex=findIndex+1;
          }
          else
          {
            searchIndex=0;
            break;
          }
        }
      }

    }

    /** This method retrieves the character at index and if it's a xml 
     *  special character replaces with XML entity reference.
     *  @param input -input string
     *  @param index  position of the special character in the input string
     */
    void SaveCanSAS1D::replacewithEntityReference(std::string& input, std::string::size_type index)
    {
      std::basic_string <char>::reference  str=input.at(index);
      switch(str)
      {
      case '&':
        input.replace(index,1,"&amp;");
        break;
      case '<':
        input.replace(index,1,"&lt;");
        break;
      case '>':
        input.replace(index,1,"&gt;");
        break;
      case '\'':
        input.replace(index,1,"&apos;");
        break;
      case '\"':
        input.replace(index,1,"&quot;");
        break;
      }
    }


    /** This method creates an XML element named "SASroot"
    *  @param rootElem  xml root element string
    */
    void SaveCanSAS1D::createSASRootElement(std::string& rootElem)
    {
      rootElem="<SASroot version=\"1.0\"";
      rootElem +="\n\t\t xmlns=\"cansas1d/1.0\"";
      rootElem+="\n\t\t xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"";
      rootElem+="\n\t\t xsi:schemaLocation=\"cansas1d/1.0 http://svn.smallangles.net/svn/canSAS/1dwg/trunk/cansas1d.xsd\">";
    }

    /** This method creates an XML element named "Title"
    *  @param sasTitle string for title element in the xml
    */
    void SaveCanSAS1D::createSASTitleElement(std::string& sasTitle)
    {
      std::string title=m_workspace->getTitle();
      //look for xml special characters and replace with entity refrence
      searchandreplaceSpecialChars(title);
      sasTitle="\n\t\t<Title>";
      sasTitle+=title;
      sasTitle+="</Title>";
    }

    /** This method creates an XML element named "Run"
    *  @param sasRun string for run element in the xml
    */
    void SaveCanSAS1D::createSASRunElement(std::string& sasRun)
    {
      sasRun="\n\t\t<Run>";
      std::string run=m_workspace->getName();
      //look for xml special characters and replace with entity refrence
      searchandreplaceSpecialChars(run);
      sasRun+=run;
      sasRun+="</Run>";
    }

    /** This method creates an XML element named "SASdata"
    *  @param sasData string for sasdata element in the xml
    */
    void SaveCanSAS1D::createSASDataElement(std::string& sasData)
    {
      std::string dataUnit = m_workspace->YUnitLabel();
      //look for xml special characters and replace with entity refrence
      searchandreplaceSpecialChars(dataUnit);

      sasData="\n\t\t<SASdata>";
      //outFile<<sasData;
      std::string sasIData;
      std::string sasIBlockData;
      std::string sasIHistData;
      for (int i = 0; i < m_workspace->getNumberHistograms(); ++i)
      {
        const MantidVec& xdata = m_workspace->readX(i);
        const MantidVec& ydata = m_workspace->readY(i);
        const MantidVec& edata = m_workspace->readE(i);
        const bool isHistogram = m_workspace->isHistogramData();
        for (int j = 0; j < m_workspace->blocksize(); ++j)
        {
          //x data is the QData in xml.If histogramdata take the mean
          double intensity = isHistogram ? (xdata[j] + xdata[j + 1]) / 2 : xdata[j];
          std::stringstream x;
          x << intensity; 
          sasIData="\n\t\t\t<Idata><Q unit=\"1/A\">";
          sasIData+=x.str();
          sasIData+="</Q>";
          sasIData+="<I unit=";
          sasIData+="\"";
          sasIData+=dataUnit;
          sasIData+="\">";
          //// workspace Y data is the I data in the xml file
          std::stringstream y;
          y << (ydata[j]);
          sasIData+=y.str();
          sasIData+="</I>";

          // workspace error data is the Idev data in the xml file
          std::stringstream e;
          e << edata[j];

          sasIData+="<Idev unit=";
          sasIData+="\"";
          sasIData+=dataUnit;
          sasIData+="\">";

          sasIData+=e.str();
          sasIData+="</Idev>";

          sasIData+="</Idata>";
          // outFile<<sasIData;
          sasIBlockData+=sasIData;
        }
        sasIHistData+=sasIBlockData;
      }
      sasData+=sasIHistData;

      sasData+="\n\t\t</SASdata>";
    }

    /** This method creates an XML element named "SASsample"
    *  @param sasSample string for sassample element in the xml
    */
    void SaveCanSAS1D::createSASSampleElement(std::string &sasSample)
    {
      sasSample="\n\t\t<SASsample>";
      //outFile<<sasSample;
      std::string  sasSampleId="\n\t\t\t<ID>";
      std::string sampleid=m_workspace->getTitle();
      //look for xml special characters and replace with entity refrence
      searchandreplaceSpecialChars(sampleid);
      sasSampleId+=sampleid;
      sasSampleId+="</ID>";
      sasSample+=sasSampleId;
      //outFile<<sasSampleId;
      sasSample+="\n\t\t</SASsample>";
    }

    /** This method creates an XML element named "SASsource"
    *  @param sasSource string for sassource element in the xml
    */
    void SaveCanSAS1D::createSASSourceElement(std::string& sasSource )
    {
      sasSource="\n\t\t\t<SASsource>";
      //outFile<<sasSource;

      std::string radiation_source = getPropertyValue("Radiation Source");
      std::string sasrad="\n\t\t\t\t<radiation>";
      sasrad+=radiation_source;
      sasrad+="</radiation>";
      sasSource+=sasrad;
      //outFile<<sasrad;
      sasSource+="\n\t\t\t</SASsource>";

    }
    /** This method creates an XML element named "SASdetector"
    *  @param sasDet string for sasdetector element in the xml
    */
    void SaveCanSAS1D::createSASDetectorElement(std::string& sasDet)
    {
      sasDet="\n\t\t\t<SASdetector>";
      //outFile<<sasDet;

      std::string detectorName ;
      Geometry::IDetector_const_sptr detgroup;
      try
      {
        // Get the detector object (probably a group) that goes with the result spectrum
        detgroup = m_workspace->getDetector(0);
        const int id = detgroup->getID(); //id of the first detector in the group
        // Now make sure we've got an individual detector object
        Geometry::IDetector_const_sptr det = m_workspace->getInstrument()->getDetector(id);
        // Get all its ancestors
        const std::vector<boost::shared_ptr<const IComponent> > ancs = det->getAncestors();
        // The one we want is the penultimate one
        // Shouldn't ever happen, but protect against detector having no ancestors
        detectorName = (ancs.size() > 1) ? ancs[ancs.size()-2]->getName() : det->getName();
        //look for xml special characters and replace with entity refrence
        searchandreplaceSpecialChars(detectorName);
      }
      catch(Kernel::Exception::NotFoundError&)
      {
        throw;
      }
      catch(std::runtime_error& )
      {
        throw ;
      }

      std::string sasDetname="\n\t\t\t\t<name>";
      sasDetname+=detectorName;
      sasDetname+="</name>";
      sasDet+=sasDetname;

      //outFile<<sasDetname;
      std::string sasDetUnit="\n\t\t\t\t<SDD unit=\"m\">";

      std::stringstream sdd;
      double distance = detgroup->getDistance(*m_workspace->getInstrument()->getSample());
      sdd << distance;

      sasDetUnit+=sdd.str();
      sasDetUnit+="</SDD>";
      //outFile<<sasDetUnit;
      sasDet+=sasDetUnit;
      sasDet+="\n\t\t\t</SASdetector>";
      //outFile<<sasDet;
    }

    /** This method creates an XML element named "SASprocess"
    *  @param sasProcess string for sasprocess element in the xml
    */
    void SaveCanSAS1D::createSASProcessElement(std::string& sasProcess)
    {
      sasProcess="\n\t\t<SASprocess>";
      //outFile<<sasProcess;

      std::string sasProcname="\n\t\t\t<name>";
      sasProcname+="Mantid generated CanSAS1D XML";
      sasProcname+="</name>";
      sasProcess+=sasProcname;
      //outFile<<sasProcname;

      time_t date;
      time(&date);
      std::tm*  t;
      t=localtime(&date);

      char temp [25];
      strftime (temp,25,"%d-%b-%Y %H:%M:%S",localtime(&date));
      std::string sasDate(temp);

      std::string sasProcdate="\n\t\t\t<date>";
      sasProcdate+=sasDate;
      sasProcdate+="</date>";
      sasProcess+=sasProcdate;

      std::string version(MANTID_VERSION);

      std::string sasProcsvn="\n\t\t\t<term name=\"svn\">";
      sasProcsvn+=version;
      sasProcsvn+="</term>";
      //outFile<<sasProcsvn;
      sasProcess+=sasProcsvn;

      const API::Sample& sample=  m_workspace->sample();
      std::string user_file="";
      try
      {
        user_file=sample.getLogData("UserFile")->value();
      }
      catch(std::runtime_error&ex)
      {
        g_log.debug()<<ex.what()<<std::endl;
      }

      std::string sasProcuserfile="\n\t\t\t<term name=\"user_file\">";
      sasProcuserfile+=user_file;
      sasProcuserfile+="</term>";
      //outFile<<sasProcuserfile;
      sasProcess+=sasProcuserfile;
      sasProcess+="\n\t\t</SASprocess>";
    }


  }

}
