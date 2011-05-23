#ifdef _WIN32
#pragma warning (disable: 4996)
#endif
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/CreateDummyCalFile.h"
#include "MantidAPI/FileProperty.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/ConfigService.h"
#include "MantidAPI/InstrumentDataService.h"
#include "MantidGeometry/IInstrument.h"

#include <queue>
#include <fstream>
#include <iomanip>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/Element.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/DOM/NodeIterator.h>
#include <Poco/DOM/NodeFilter.h>
#include <Poco/File.h>
#include <Poco/Path.h>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/detail/classification.hpp>

using Poco::XML::DOMParser;
using Poco::XML::Document;
using Poco::XML::Element;
using Poco::XML::Node;
using Poco::XML::NodeList;
using Poco::XML::NodeIterator;
using Poco::XML::NodeFilter;


namespace Mantid
{
  namespace Algorithms
  {

    // Register the class into the algorithm factory
    DECLARE_ALGORITHM(CreateDummyCalFile)
    
    /// Sets documentation strings for this algorithm
    void CreateDummyCalFile::initDocs()
    {
      this->setWikiSummary("Create a [[CalFile|calibration file]] (extension *.cal) from a workspace by harvesting the detector ids from the instrument. All of the offsets will be zero, and the pixels will be all grouped into group one and the final column should be one. This will allow generating powder patterns from instruments that have not done a proper calibration. Deprecated: use [[CreateGroupingWorkspace]] instead.");
      this->setOptionalMessage("Create a calibration file (extension *.cal) from a workspace by harvesting the detector ids from the instrument. All of the offsets will be zero, and the pixels will be all grouped into group one and the final column should be one. This will allow generating powder patterns from instruments that have not done a proper calibration. Deprecated: use CreateGroupingWorkspace instead.");
    }
    

    using namespace Kernel;
    using namespace API;
    using namespace Geometry;
    using namespace DataObjects;

    CreateDummyCalFile::CreateDummyCalFile():API::Algorithm(),group_no(0)
    {
      this->useAlgorithm("CreateGroupingWorkspace");
      this->deprecatedDate("2011-05-10");
    }

    /** Initialisation method. Declares properties to be used in algorithm.
     *
     */
    void CreateDummyCalFile::init()
    {
      declareProperty(
        new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input),
                            "The workspace containing the geometry to be calibrated." );
      declareProperty(new FileProperty("CalFilename","",FileProperty::Save, ".cal"),
        "The name of the output CalFile");
    }

    /** Executes the algorithm
     *
     *  @throw Exception::FileError If the grouping file cannot be opened or read successfully
     *  @throw runtime_error If unable to run one of the sub-algorithms successfully
     */
    void CreateDummyCalFile::exec()
    {
      std::ostringstream mess;
      // Get the input workspace
      MatrixWorkspace_const_sptr inputW = getProperty("InputWorkspace");
      if (!inputW)
        throw std::invalid_argument("No InputWorkspace");

      //Get some stuff from the input workspace
      IInstrument_sptr inst = inputW->getInstrument();
      std::string instname = inst->getName();

      // Check that the instrument is in store
      // Get only the first 3 letters
      std::string instshort=instname;
      std::transform(instshort.begin(),instshort.end(),instshort.begin(),toupper);
      instshort=instshort+"_Definition.xml";
      // Determine the search directory for XML instrument definition files (IDFs)
      std::string directoryName = Kernel::ConfigService::Instance().getInstrumentDirectory();

      // Set up the DOM parser and parse xml file
      DOMParser pParser;
      Document* pDoc;
      try
      {
        pDoc = pParser.parse(directoryName+instshort);
      }
      catch(...)
      {
        g_log.error("Unable to parse file " + m_filename);
        throw Kernel::Exception::FileError("Unable to parse File:" , m_filename);
      }
      // Get pointer to root element
      Element* pRootElem = pDoc->documentElement();
      if ( !pRootElem->hasChildNodes() )
      {
        g_log.error("XML file: " + m_filename + "contains no root element.");
        throw Kernel::Exception::InstrumentDefinitionError("No root element in XML instrument file", m_filename);
      }

      // Handle used in the singleton constructor for instrument file should append the value
      // of the last-modified tag inside the file to determine if it is already in memory so that
      // changes to the instrument file will cause file to be reloaded.
      instshort = instshort + pRootElem->getAttribute("last-modified");

      // If instrument not in store, insult the user
      if (!API::InstrumentDataService::Instance().doesExist(instshort))
      {
        g_log.error("Instrument "+instshort+" is not present in data store.");
        throw std::runtime_error("Instrument "+instshort+" is not present in data store.");
      }

      // Get the names of groups
      groups=instname;

      // Split the names of the group and insert in a vector, throw if group empty
      std::vector<std::string> vgroups;
      boost::split( vgroups, instname, boost::algorithm::detail::is_any_ofF<char>(",/*"));
      if (vgroups.empty())
      {
        g_log.error("Could not determine group names. Group names should be separated by / or ,");
        throw std::runtime_error("Could not determine group names. Group names should be separated by / or ,");
      }

      // Assign incremental number to each group
      std::map<std::string,int> group_map;
      int index=0;
      for (std::vector<std::string>::const_iterator it=vgroups.begin();it!=vgroups.end();it++)
        group_map[(*it)]=++index;

      // Not needed anymore
      vgroups.clear();

      // Find Detectors that belong to groups
      typedef boost::shared_ptr<Geometry::ICompAssembly> sptr_ICompAss;
      typedef boost::shared_ptr<Geometry::IComponent> sptr_IComp;
      typedef boost::shared_ptr<Geometry::IDetector> sptr_IDet;
      std::queue< std::pair<sptr_ICompAss,int> > assemblies;
      sptr_ICompAss current=boost::dynamic_pointer_cast<Geometry::ICompAssembly>(inst);
      sptr_IDet currentDet;
      sptr_IComp currentIComp;
      sptr_ICompAss currentchild;

      int top_group, child_group;

      if (current.get())
      {
        top_group=group_map[current->getName()]; // Return 0 if not in map
        assemblies.push(std::make_pair<sptr_ICompAss,int>(current,top_group));
      }

      std::string filename=getProperty("CalFilename");

      // Plan to overwrite file, so do not check if it exists
      bool overwrite=false; 

      int number=0;
      Progress prog(this,0.0,0.8,assemblies.size());
      while(!assemblies.empty()) //Travel the tree from the instrument point
      {
        current=assemblies.front().first;
        top_group=assemblies.front().second;
        assemblies.pop();
        int nchilds=current->nelements();
        if (nchilds!=0)
        {
          for (int i=0;i<nchilds;++i)
          {
            currentIComp=(*(current.get()))[i]; // Get child
            currentDet=boost::dynamic_pointer_cast<Geometry::IDetector>(currentIComp);
            if (currentDet.get())// Is detector
            {
              if (overwrite) // Map will contains udet as the key
                instrcalib[currentDet->getID()]=std::make_pair<int,int>(number++,top_group);
              else          // Map will contains the entry number as the key
                instrcalib[number++]=std::make_pair<int,int>(currentDet->getID(),top_group);
            }
            else // Is an assembly, push in the queue
            {
              currentchild=boost::dynamic_pointer_cast<Geometry::ICompAssembly>(currentIComp);
              if (currentchild.get())
              {
                child_group=group_map[currentchild->getName()];
                if (child_group==0)
                  child_group=top_group;
                assemblies.push(std::make_pair<sptr_ICompAss,int>(currentchild,child_group));
              }
            }
          }
        }
        prog.report();
      }
      // Write the results in a file
      saveGroupingFile(filename,overwrite);
      progress(0.2);
      return;
    }

    bool CreateDummyCalFile::groupingFileDoesExist(const std::string& filename) const
    {
      std::ifstream file(filename.c_str());
      // Check if the file already exists
      if (!file)
        return false;
      file.close();
      std::ostringstream mess;
      mess << "Calibration file "<< filename << " already exist. Only grouping will be modified";
      g_log.information(mess.str());
      return true;
    }

    /// Creates and saves the output file
    void CreateDummyCalFile::saveGroupingFile(const std::string& filename,bool overwrite) const
    {
      std::ostringstream message;
      std::fstream outfile;
      std::fstream infile;
      if (!overwrite) // Open the file directly
      {
        outfile.open(filename.c_str(), std::ios::out);
        if (!outfile.is_open())
        {
          message << "Can't open Calibration File: " << filename;
          g_log.error(message.str());
          throw std::runtime_error(message.str());
          message.str("");
        }
      }
      else
      {
        infile.open(filename.c_str(),std::ios::in);
        std::string newfilename=filename+"2";
        outfile.open(newfilename.c_str(), std::ios::out);
        if (!infile.is_open())
        {
          message << "Can't open input Calibration File: " << filename;
          g_log.error(message.str());
          throw std::runtime_error(message.str());
        }
        if (!outfile.is_open())
        {
          message << "Can't open new Calibration File: " << newfilename;
          g_log.error(message.str());
          throw std::runtime_error(message.str());
        }
      }

      // Write the headers
      writeHeaders(outfile,filename,overwrite);

      if (overwrite)
      {
        int number, udet, select, group;
        double offset;

        instrcalmap::const_iterator it;
        std::string str;
        while(getline(infile,str))
        {
          if (str.empty() || str[0]=='#') // Skip the headers
            continue;
          std::istringstream istr(str);
          istr >> number >> udet >> offset >> select >> group;
          it=instrcalib.find(udet);
          if (it==instrcalib.end()) // Not found, don't assign a group
            group=0;
          group=((*it).second).second; // If found then assign new group
          writeCalEntry(outfile,number,udet,offset,select,group);
        }
      }
      else //
      {
        instrcalmap::const_iterator it=instrcalib.begin();
        for (;it!=instrcalib.end();it++)
          writeCalEntry(outfile,(*it).first,((*it).second).first,0.0,1,((*it).second).second);
      }

      // Closing
      outfile.close();
      if (overwrite)
        infile.close();
      return;
    }

    /// Writes a single calibration line to the output file
    void CreateDummyCalFile::writeCalEntry(std::ostream& os, int number, int udet, double offset, int select, int group)
    {
      os << std::fixed << std::setw(9) << number <<
        std::fixed << std::setw(15) << udet <<
        std::fixed << std::setprecision(7) << std::setw(15)<< offset <<
        std::fixed << std::setw(8) << select <<
        std::fixed << std::setw(8) << group  << "\n";
      return;
    }

    /// Writes out the header to the output file
    void CreateDummyCalFile::writeHeaders(std::ostream& os,const std::string& filename,bool overwrite) const
    {
      os << "# Diffraction focusing calibration file created by Mantid" <<  "\n";
      os << "# Detectors have been grouped using assembly names:" << groups <<"\n";
      if (overwrite)
      {
        os << "# Template file " << filename << " has been used" << "\n";
        os << "# Only grouping has been changed, offset from template file have been copied" << "\n";
      }
      else
        os << "# No template file, all offsets set to 0.0 and select to 1" << "\n";

      os << "#  Number           UDET         offset      select  group" << "\n";
      return;
    }

  } // namespace Algorithm
} // namespace Mantid
