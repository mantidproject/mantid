// NexusFileWriter
// @author Ronald Fowler
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <vector>
#include <sstream>
#include <napi.h>
#include <stdlib.h>
#ifdef _WIN32
#include <io.h>
#endif /* _WIN32 */
#include "MantidNexus/NexusFileWriter.h"
#include <boost/shared_ptr.hpp>

namespace Mantid
{
namespace NeXus
{
  using namespace Kernel;
  using namespace API;

  Logger& NexusFileWriter::g_log = Logger::get("NexusFileWriter");

  /// Empty default constructor
  NexusFileWriter::NexusFileWriter() :
                   m_nexusformat(NXACC_CREATE5), m_nexuscompression(NX_COMP_LZW)
  {
  }

  int NexusFileWriter::writeNexusTextField( const NXhandle& h, const std::string& name, const std::string& value)
  {
    // write a NXdata section "name" with string value "value" (not used?)
    int status;
    int dims_array[1] = { value.size() };
    status=NXmakedata(h, name.c_str(), NX_CHAR, 1, dims_array);
    status=NXopendata(h, name.c_str());
    status=NXputdata(h, (void*)&(value[0]));
    status=NXclosedata(h);
    return(0);
  }

//
// Write out the data in a workspace in Nexus "Processed" format.
// This *Proposed* standard comprises the fields:
// <NXentry name="{Name of entry}">
//   <title>
//     {Extended title for entry}
//   </title>
//   <definition URL="http://www.nexusformat.org/instruments/xml/NXprocessed.xml"
//       version="1.0">
//     NXprocessed
//   </definition>
//   <NXsample name="{Name of sample}">?
//     {Any relevant sample information necessary to define the data.}
//   </NXsample>
//   <NXdata name="{Name of processed data}">
//     <values signal="1" type="NX_FLOAT[:,:]" axes="axis1:axis2">{Processed values}</values>
//     <axis1 type="NX_FLOAT[:]">{Values of the first dimension's axis}</axis1>
//     <axis2 type="NX_FLOAT[:]">{Values of the second dimension's axis}</axis2>
//   </NXdata>
//   <NXprocess name="{Name of process}">?
//     {Any relevant information about the steps used to process the data.}
//   </NXprocess>
// </NXentry>

  int NexusFileWriter::openNexusWrite(const std::string& fileName, const std::string& entryName)
  {
   // open named file and entry - file may exist
   // @throw Exception::FileError if cannot open Nexus file for writing
   //
   NXaccess mode;
   NXstatus status;
   std::string className="NXentry";
   std::string mantidEntryName=entryName; // temp allow this for existing files
   m_filename=fileName;
   if(Poco::File(m_filename).exists())
       mode = NXACC_RDWR;
   else
   {
       if( fileName.find(".xml") < fileName.size() || fileName.find(".XML") < fileName.size() )
       {
           mode = NXACC_CREATEXML;
           m_nexuscompression = NX_COMP_NONE;
       }
       else
           mode = m_nexusformat;
       mantidEntryName="mantid_workspace_1";
   }
   status=NXopen(fileName.c_str(), mode, &fileID);
   if(status==NX_ERROR)
   {
       g_log.error("Unable to open file " + fileName);
       throw Exception::FileError("Unable to open File:" , fileName);	  
   }
   if(mode==NXACC_RDWR)
   {
       int count=findMantidWSEntries();
       std::stringstream suffix;
       suffix << (count+1);
       mantidEntryName="mantid_workspace_"+suffix.str();
   }
   status=NXmakegroup(fileID,mantidEntryName.c_str(),className.c_str());
   if(status==NX_ERROR)
       return(2);
   status=NXopengroup(fileID,mantidEntryName.c_str(),className.c_str());
   return(0);
  }

  int NexusFileWriter::closeNexusFile()
  {
   NXstatus status;
   status=NXclosegroup(fileID);
   status=NXclose(&fileID);
   return(0);
  }
  int NexusFileWriter::writeNexusProcessedHeader( const std::string& entryName,
	                                         const std::string& title)
  {
   // Write Nexus mantid workspace header fields for the NXentry/IXmantid/NXprocessed field.
   // The URLs are not correct
   std::string className="NXentry";
   std::vector<std::string> attributes,avalues;
   if( ! writeNxText(fileID, "title", title, attributes, avalues) )
       return(3);
   //
   attributes.push_back("URL");
   avalues.push_back("http://www.nexusformat.org/instruments/xml/NXprocessed.xml");
   attributes.push_back("Version");
   avalues.push_back("1.0");
   // this may not be the "correct" long term path, but it is valid at present 
   if( ! writeNxText(fileID, "definition", className, attributes, avalues) )
       return(3);
   avalues.clear();
   avalues.push_back("http://www.isis.rl.ac.uk/xml/IXmantid.xml");
   avalues.push_back("1.0");
   if( ! writeNxText(fileID, "definition_local", className, attributes, avalues) )
       return(3);
   return(0);
  }
  bool NexusFileWriter::writeNexusInstrumentXmlName(const std::string& instrumentXml,const std::string& date,
      const std::string& version)
  {
      std::vector<std::string> attributes,avalues;
      if(date != "")
      {
          attributes.push_back("date");
          avalues.push_back(date);
      }
      if(version != "")
      {
         attributes.push_back("Version");
         avalues.push_back(version);
      }
      if( ! writeNxText(fileID, "instrument_source", instrumentXml, attributes, avalues) )
          return(false);
      return(true);
  }
  bool NexusFileWriter::writeNexusInstrument(const boost::shared_ptr<API::Instrument>& instrument)
  {
   NXstatus status;

   //write instrument entry
   status=NXmakegroup(fileID,"instrument","NXinstrument");
   if(status==NX_ERROR)
       return(false);
   status=NXopengroup(fileID,"instrument","NXinstrument");
   //
   std::string name=instrument->getName();
   std::vector<std::string> attributes,avalues;
   if( ! writeNxText(fileID, "name", name, attributes, avalues) )
       return(false);

   status=NXclosegroup(fileID);

   return((status==NX_ERROR)?3:0);
      return(true);
  }
//
// write an NXdata entry with char values
//
  bool NexusFileWriter::writeNxText(NXhandle fileID, std::string name, std::string value, std::vector<std::string> attributes,
	                           std::vector<std::string> avalues)
  {
   NXstatus status;
   int dimensions[1];
   dimensions[0]=value.size()+1;
   status=NXmakedata(fileID, name.c_str(), NX_CHAR, 1, dimensions);
   if(status==NX_ERROR) return(false);
   status=NXopendata(fileID, name.c_str());
   for(size_t it=0; it<attributes.size(); ++it)
       status=NXputattr(fileID, attributes[it].c_str(), (void*)avalues[it].c_str(), avalues[it].size(), NX_CHAR);
   status=NXputdata(fileID, (void*)value.c_str());
   status=NXclosedata(fileID);
   return(true);
  }
//
// write an NXdata entry with Float value
//
  bool NexusFileWriter::writeNxFloat(const std::string name, const double& value, const std::vector<std::string> attributes,
	                           const std::vector<std::string> avalues)
  {
   NXstatus status;
   int dimensions[1];
   dimensions[0]=1;
   status=NXmakedata(fileID, name.c_str(), NX_FLOAT64, 1, dimensions);
   if(status==NX_ERROR) return(false);
   status=NXopendata(fileID, name.c_str());
   for(size_t it=0; it<attributes.size(); ++it)
       status=NXputattr(fileID, attributes[it].c_str(), (void*)avalues[it].c_str(), avalues[it].size(), NX_CHAR);
   status=NXputdata(fileID, (void*)&value);
   status=NXclosedata(fileID);
   return(true);
  }
//
// Write an NXnote entry with data giving parameter pair values for algorithm history and environment
// Use NX_CHAR instead of NX_BINARY for the parameter values to make more simple.
//
  bool NexusFileWriter::writeNxNote(const std::string& noteName, const std::string& author, const std::string& date,
                         const std::string& description, const std::string& pairValues)
  {
   NXstatus status;
   status=NXmakegroup(fileID,noteName.c_str(),"NXnote");
   if(status==NX_ERROR)
       return(false);
   status=NXopengroup(fileID,noteName.c_str(),"NXnote");
   //
   std::vector<std::string> attributes,avalues;
   if(date!="")
   {
       attributes.push_back("date");
       avalues.push_back(date);
   }
   if( ! writeNxText(fileID, "author", author, attributes, avalues) )
       return(false);
   attributes.clear();
   avalues.clear();
   if( ! writeNxText(fileID, "description", description, attributes, avalues) )
       return(false);
   if( ! writeNxText(fileID, "data", pairValues, attributes, avalues) )
       return(false);

   status=NXclosegroup(fileID);
   return(true);
  }
//
// Write sample related information to the nexus file
//}

  int NexusFileWriter::writeNexusProcessedSample( const std::string& entryName, const std::string& name,
							  const boost::shared_ptr<Mantid::API::Sample> sample)
  {
   NXstatus status;

   //write sample entry
   status=NXmakegroup(fileID,"sample","NXsample");
   if(status==NX_ERROR)
       return(2);
   status=NXopengroup(fileID,"sample","NXsample");
   //
   std::vector<std::string> attributes,avalues;
   if( ! writeNxText(fileID, "name", name, attributes, avalues) )
       return(3);
   // Write proton_charge here, if available. Note that TOFRaw has this at the NXentry level, though there is
   // some debate if this is appropriate. Hence for Mantid write it to the NXsample section as it is stored in Sample.
   const double totalProtonCharge=sample->getProtonCharge();
   if( totalProtonCharge>0 )
   {
       attributes.push_back("units");
       avalues.push_back("microAmps*hour");
       if( ! writeNxFloat( "proton_charge", totalProtonCharge, attributes, avalues) )
           return(4);
   }

   status=NXclosegroup(fileID);

   return((status==NX_ERROR)?3:0);
  }

  int NexusFileWriter::writeNexusProcessedData( const std::string& entryName,
							const boost::shared_ptr<Mantid::DataObjects::Workspace2D> localworkspace,
							const bool uniformSpectra, const int m_spec_min, const int m_spec_max)
  {
   NXstatus status;

   //write data entry
   status=NXmakegroup(fileID,"workspace","NXdata");
   if(status==NX_ERROR)
       return(2);
   status=NXopengroup(fileID,"workspace","NXdata");
   // write workspace data
   const int nHist=localworkspace->getNumberHistograms();
   if(nHist<1)
	   return(2);
   const int nSpectBins=localworkspace->dataY(0).size();
   const int nSpect=m_spec_max-m_spec_min+1;
   int dims_array[2] = { nSpect,nSpectBins };
   std::string name=localworkspace->getTitle();
   if(name.size()==0)
	   name="values";
   int start[2]={0,0},asize[2]={1,dims_array[1]};
   status=NXcompmakedata(fileID, name.c_str(), NX_FLOAT64, 2, dims_array,m_nexuscompression,asize);
   status=NXopendata(fileID, name.c_str());

   for(int i=m_spec_min;i<=m_spec_max;i++)
   {
      status=NXputslab(fileID, (void*)&(localworkspace->dataY(i)[0]),start,asize);
	  start[0]++;
   }
   int signal=1;
   status=NXputattr (fileID, "signal", &signal, 1, NX_INT32);
   Mantid::API::Axis *xAxis=localworkspace->getAxis(0);
   Mantid::API::Axis *yAxis=localworkspace->getAxis(1);
   std::string xLabel,yLabel;
   if(xAxis->isSpectra())
	   xLabel="spectraNumber";
   else
	   xLabel=xAxis->unit()->unitID();
   std::vector<double> histNumber;
   if(yAxis->isSpectra())
   {
	   yLabel="spectraNumber";
	   for(int i=m_spec_min;i<=m_spec_max;i++)
	       histNumber.push_back((double)yAxis->spectraNo(i));
   }
   else
	   yLabel=yAxis->unit()->unitID();
   const std::string axesNames=xLabel+":"+yLabel;
   status=NXputattr (fileID, "axes", (void*)axesNames.c_str(), axesNames.size(), NX_CHAR);
   status=NXclosedata(fileID);
   // error
   name="errors";
   //status=NXmakedata(fileID, name.c_str(), NX_FLOAT64, 2, dims_array);
   status=NXcompmakedata(fileID, name.c_str(), NX_FLOAT64, 2, dims_array,m_nexuscompression,asize);
   status=NXopendata(fileID, name.c_str());
   start[0]=0;
   for(int i=m_spec_min;i<=m_spec_max;i++)
   {
      status=NXputslab(fileID, (void*)&(localworkspace->dataE(i)[0]),start,asize);
	  start[0]++;
   }
   status=NXclosedata(fileID);
   // write X data, as single array or all values if "ragged"
   if(uniformSpectra)
   {
	   dims_array[0]=localworkspace->dataX(1).size();
	   status=NXmakedata(fileID, "axis1", NX_FLOAT64, 1, dims_array);
       status=NXopendata(fileID, "axis1");
	   status=NXputdata(fileID, (void*)&(localworkspace->dataX(1)[0]));
   }
   else
   {
	   dims_array[0]=nSpect;
	   dims_array[1]=localworkspace->dataX(1).size();
	   status=NXmakedata(fileID, "axis1", NX_FLOAT64, 2, dims_array);
       status=NXopendata(fileID, "axis1");
	   start[0]=0; asize[1]=dims_array[1];
       for(int i=m_spec_min;i<=m_spec_max;i++)
       {
           status=NXputslab(fileID, (void*)&(localworkspace->dataX(i)[0]),start,asize);
	       start[0]++;
       }
   }
   std::string dist=(localworkspace->isDistribution()) ? "1" : "0";
   status=NXputattr(fileID, "distribution", (void*)dist.c_str(), 2, NX_CHAR);
   status=NXclosedata(fileID);
   // write axis2, maybe just spectra number
   dims_array[0]=m_spec_max-m_spec_min+1;
   status=NXmakedata(fileID, "axis2", NX_FLOAT64, 1, dims_array);
   status=NXopendata(fileID, "axis2");
   status=NXputdata(fileID, (void*)&(histNumber[m_spec_min]));
   status=NXclosedata(fileID);

   status=NXclosegroup(fileID);
   return((status==NX_ERROR)?3:0);
  }

  int NexusFileWriter::writeNexusProcessedProcess(const boost::shared_ptr<Mantid::DataObjects::Workspace2D> localworkspace)
  {
   // Write Process section
   NXstatus status;
   status=NXmakegroup(fileID,"process","NXprocess");
   if(status==NX_ERROR)
       return(2);
   status=NXopengroup(fileID,"process","NXprocess");
   //Mantid:API::Workspace xxx;
   const API::WorkspaceHistory history=localworkspace->getHistory();
   const std::vector<AlgorithmHistory>& algHist = history.getAlgorithmHistories();
   std::stringstream output,algorithmNumber;
   EnvironmentHistory envHist;

    //dump output to sting
   output << envHist;
   char buffer [25];
   time_t now;
   time(&now);
   strftime (buffer,25,"%Y-%b-%d %H:%M:%S",localtime(&now));
   writeNxNote("MantidEnvironment","mantid",buffer,"Mantid Environment data",output.str());
   int histsize = static_cast<int>(algHist.size());
   for(int i=0; i < histsize;i++)
   {
       std::stringstream algNumber,algData;
       algNumber << "MantidAlgorithm_" << i;
       algHist[i].printSelf(algData);
       writeNxNote(algNumber.str(),"mantid","","Mantid Algorithm data",algData.str());
   }
   status=NXclosegroup(fileID);
   return(0);
  }

  int NexusFileWriter::findMantidWSEntries()
  {
   // search exiting file for entries of form mantid_workspace_<n> and return count
   int count=0;
   NXstatus status;
   char *nxname,*nxclass;
   int nxdatatype;
   nxname= new char[NX_MAXNAMELEN];
   nxclass = new char[NX_MAXNAMELEN];
   //
   // read nexus fields at this level
   while( (status=NXgetnextentry(fileID,nxname,nxclass,&nxdatatype)) == NX_OK )
   {
      std::string nxClass=nxclass;
      if(nxClass=="NXentry")
      {
          std::string nxName=nxname;
          if(nxName.find("mantid_workspace_")==0)
              count++;
      }
   }
   delete[] nxname;
   delete[] nxclass;
   return count;
  }
} // namespace NeXus
} // namespace Mantid
