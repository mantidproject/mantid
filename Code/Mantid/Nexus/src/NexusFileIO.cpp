// NexusFileIO
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
#include "MantidAPI/Instrument.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidNexus/NexusFileIO.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/FileValidator.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidKernel/PhysicalConstants.h"

#include <boost/tokenizer.hpp>
#include <boost/shared_ptr.hpp>
#include "Poco/File.h"

namespace Mantid
{
namespace NeXus
{
  using namespace Kernel;
  using namespace API;

  Logger& NexusFileIO::g_log = Logger::get("NexusFileIO");

  /// Empty default constructor
  NexusFileIO::NexusFileIO() :
                   m_nexusformat(NXACC_CREATE5), m_nexuscompression(NX_COMP_LZW)
  {
  }

  int NexusFileIO::writeNexusTextField( const NXhandle& h, const std::string& name, const std::string& value)
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

  int NexusFileIO::openNexusWrite(const std::string& fileName )
  {
   // open named file and entry - file may exist
   // @throw Exception::FileError if cannot open Nexus file for writing
   //
   NXaccess mode;
   NXstatus status;
   std::string className="NXentry";
   std::string mantidEntryName;
   m_filename=fileName;
   //
   // If file to write exists, then open as is else see if the extension is xml, if so open as xml
   // format otherwise as compressed hdf5
   //
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
   //
   // for existing files, search for any current mantid_workspace_<n> entries and set the
   // new name to be n+1 so that we do not over-write by default. This may need changing.
   //
   if(mode==NXACC_RDWR)
   {
       int count=findMantidWSEntries();
       std::stringstream suffix;
       suffix << (count+1);
       mantidEntryName="mantid_workspace_"+suffix.str();
   }
   //
   // make and open the new mantid_workspace_<n> group
   // file remains open until explict close
   //
   status=NXmakegroup(fileID,mantidEntryName.c_str(),className.c_str());
   if(status==NX_ERROR)
       return(2);
   status=NXopengroup(fileID,mantidEntryName.c_str(),className.c_str());
   return(0);
  }

  int NexusFileIO::openNexusRead(const std::string& fileName, int& workspaceNumber )
  {
   // Open named file and entry "mantid_workspace_<n> - file must exist
   // If workspaceNumber>0, use as value on <n>, otherwise take highest
   //
   // @throw Exception::FileError if cannot open Nexus file for reading, or no
   //                             mantid_workspace_<n> entry
   //
   NXaccess mode;
   NXstatus status;
   std::string className="NXentry";
   std::string mantidEntryName;
   m_filename=fileName;
   mode = NXACC_READ;
   // check file exists
   if(!Poco::File(fileName).exists())
   {
       g_log.error("File not found " + fileName);
       throw Exception::FileError("File not found:" , fileName);	  
   }
   // open for reading
   status=NXopen(fileName.c_str(), mode, &fileID);
   if(status==NX_ERROR)
   {
       g_log.error("Unable to open file " + fileName);
       throw Exception::FileError("Unable to open File:" , fileName);	  
   }
   // search for mantid_workspace_<n> entries
   int count=findMantidWSEntries();
   if(count<1)
   {
       g_log.error("File contains no mantid_workspace_ entries " + fileName);
       throw Exception::FileError("File contains no mantid_workspace_ entries: " , fileName);
   }
   //
   // if workspaceNumber==0 then take highest existing workspace, else look for given number
   //
   if( workspaceNumber>0 )
   {
       if( workspaceNumber<=count )
       {
           count=workspaceNumber;
       }
       else
       {
           g_log.error("Requested entry number is greater than available in file: " + fileName);
           throw Exception::FileError("File does not contains mantid_workspace_" + workspaceNumber , fileName);
       }
   }
   std::stringstream suffix;
   suffix << (count);
   mantidEntryName="mantid_workspace_"+suffix.str();
   status=NXopengroup(fileID,mantidEntryName.c_str(),className.c_str());
   return(status==NX_OK ? 0:1);
  }

  int NexusFileIO::closeNexusFile()
  {
   NXstatus status;
   status=NXclosegroup(fileID);
   status=NXclose(&fileID);
   return(0);
  }

  int NexusFileIO::writeNexusProcessedHeader( const std::string& title)
  {
   // Write Nexus mantid workspace header fields for the NXentry/IXmantid/NXprocessed field.
   // The URLs are not correct as they do not exist presently, but follow the format for other
   // Nexus specs.
   //
   std::string className="Mantid Processed Workspace";
   std::vector<std::string> attributes,avalues;
   if( ! writeNxText("title", title, attributes, avalues) )
       return(3);
   //
   attributes.push_back("URL");
   avalues.push_back("http://www.nexusformat.org/instruments/xml/NXprocessed.xml");
   attributes.push_back("Version");
   avalues.push_back("1.0");
   // this may not be the "correct" long term path, but it is valid at present 
   if( ! writeNxText( "definition", className, attributes, avalues) )
       return(3);
   avalues.clear();
   avalues.push_back("http://www.isis.rl.ac.uk/xml/IXmantid.xml");
   avalues.push_back("1.0");
   if( ! writeNxText( "definition_local", className, attributes, avalues) )
       return(3);
   return(0);
  }
  bool NexusFileIO::writeNexusInstrumentXmlName(const std::string& instrumentXml,const std::string& date,
      const std::string& version)
  {
      //
      // The name used for the instrument XML definition is stored as part of the file, rather than
      // the actual instrument data.
      //
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
      if( ! writeNxText( "instrument_source", instrumentXml, attributes, avalues) )
          return(false);
      return(true);
  }
  
  bool NexusFileIO::readNexusInstrumentXmlName(std::string& instrumentXml,std::string& date,
                    std::string& version)
  {
      std::vector<std::string> attributes,avalues;
      if( ! readNxText( "instrument_source", instrumentXml, attributes, avalues) )
          return(false);
      for(unsigned int i=1;i<=attributes.size();i++)
      {
          if(attributes[i-1].compare("date") == 0)
          {
              date=avalues[i-1];
          }
          else if(attributes[i-1].compare("version") == 0)
          {
              version=avalues[i-1];
          }
      }

      return(true);
  }
  
  std::string NexusFileIO::readNexusInstrumentName()
  {
    std::vector<std::string> attributes,avalues;
    std::string name;
    NXstatus status=NXopengroup(fileID,"instrument","NXinstrument");
    if (status==NX_ERROR) return "";
    if (! readNxText("name",name,attributes,avalues) ) name = "";
    NXclosegroup(fileID);
    return name;
  }
  
  bool NexusFileIO::writeNexusInstrument(const API::IInstrument_const_sptr& instrument)
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
   if( ! writeNxText( "name", name, attributes, avalues) )
       return(false);

   status=NXclosegroup(fileID);

   return(true);
  }
//
// write an NXdata entry with char values
//
  bool NexusFileIO::writeNxText( const std::string& name, const std::string& value, const std::vector<std::string>& attributes,
	                           const std::vector<std::string>& avalues)
  {
   NXstatus status;
   int dimensions[1];
   dimensions[0]=value.size()+1;
   status=NXmakedata(fileID, name.c_str(), NX_CHAR, 1, dimensions);
   if(status==NX_ERROR) return(false);
   status=NXopendata(fileID, name.c_str());
   for(unsigned int it=0; it<attributes.size(); ++it)
       status=NXputattr(fileID, attributes[it].c_str(), (void*)avalues[it].c_str(), avalues[it].size(), NX_CHAR);
   status=NXputdata(fileID, (void*)value.c_str());
   status=NXclosedata(fileID);
   return(true);
  }
//
// read an NXdata entry with char values along with attribute names and values
//
  bool NexusFileIO::readNxText(const std::string& name, std::string& value, std::vector<std::string>& attributes,
	                           std::vector<std::string>& avalues)
  {
   NXstatus status;
   status=NXopendata(fileID, name.c_str());
   if(status==NX_ERROR)
       return(false);
   int length,type;
   char aname[NX_MAXNAMELEN];
   char avalue[NX_MAXNAMELEN]; // value is not restricted to this, but it is a reasonably large value
   while(NXgetnextattr(fileID,aname,&length,&type)==NX_OK)
   {
       if(type==NX_CHAR) // ignoring non char attributes
       {
           attributes.push_back(aname);
           NXgetattr(fileID,aname,(void *)avalue,&length,&type);
           avalues.push_back(avalue);
       }
   }
   int rank,dim[1];
   status=NXgetinfo(fileID, &rank, dim, &type);
   if(type==NX_CHAR && dim[0]>0)
   {
       char *dvalue=new char[dim[0]+1];
       status=NXgetdata(fileID, (void *)dvalue);
       dvalue[dim[0]]='\0';
       value=dvalue;
   }
   status=NXclosedata(fileID);
   return(true);
  }
//
// write an NXdata entry with Float value
//
  bool NexusFileIO::writeNxFloat(const std::string& name, const double& value, const std::vector<std::string>& attributes,
	                           const std::vector<std::string>& avalues)
  {
   NXstatus status;
   int dimensions[1];
   dimensions[0]=1;
   status=NXmakedata(fileID, name.c_str(), NX_FLOAT64, 1, dimensions);
   if(status==NX_ERROR) return(false);
   status=NXopendata(fileID, name.c_str());
   for(unsigned int it=0; it<attributes.size(); ++it)
       status=NXputattr(fileID, attributes[it].c_str(), (void*)avalues[it].c_str(), avalues[it].size()+1, NX_CHAR);
   status=NXputdata(fileID, (void*)&value);
   status=NXclosedata(fileID);
   return(true);
  }
//
// read an NXdata entry with Float value
//
  bool NexusFileIO::readNxFloat(const std::string& name, double& value, std::vector<std::string>& attributes,
	                           std::vector<std::string>& avalues)
  {
   NXstatus status;
   status=NXopendata(fileID, name.c_str());
   if(status==NX_ERROR)
       return(false);
   int length=NX_MAXNAMELEN,type;
   char aname[NX_MAXNAMELEN];
   char avalue[NX_MAXNAMELEN]; // value is not restricted to this (64), but should be sufficient
   while(NXgetnextattr(fileID,aname,&length,&type)==NX_OK)
   {
       if(type==NX_CHAR) // ignoring non char attributes
       {
           attributes.push_back(aname);
           NXgetattr(fileID,aname,(void *)avalue,&length,&type);
           avalues.push_back(avalue);
       }
   }
   status=NXgetdata(fileID, (void*)&value);
   status=NXclosedata(fileID);
   return(true);
  }
//
// write an NXdata entry with Float array values
//
  bool NexusFileIO::writeNxFloatArray(const std::string& name, const std::vector<double>& values, const std::vector<std::string>& attributes,
	                           const std::vector<std::string>& avalues)
  {
   NXstatus status;
   int dimensions[1];
   dimensions[0]=values.size();
   status=NXmakedata(fileID, name.c_str(), NX_FLOAT64, 1, dimensions);
   if(status==NX_ERROR) return(false);
   status=NXopendata(fileID, name.c_str());
   for(unsigned int it=0; it<attributes.size(); ++it)
       status=NXputattr(fileID, attributes[it].c_str(), (void*)avalues[it].c_str(), avalues[it].size()+1, NX_CHAR);
   status=NXputdata(fileID, (void*)&(values[0]));
   status=NXclosedata(fileID);
   return(true);
  }
//
// write an NXdata entry with String array values
//
  bool NexusFileIO::writeNxStringArray(const std::string& name, const std::vector<std::string>& values, const std::vector<std::string>& attributes,
	                           const std::vector<std::string>& avalues)
  {
   NXstatus status;
   int dimensions[2];
   size_t maxlen=0;
   dimensions[0]=values.size();
   for(size_t i=0;i<values.size();i++)
       if(values[i].size()>maxlen) maxlen=values[i].size();
   dimensions[1]=maxlen;
   status=NXmakedata(fileID, name.c_str(), NX_CHAR, 2, dimensions);
   if(status==NX_ERROR) return(false);
   status=NXopendata(fileID, name.c_str());
   for(unsigned int it=0; it<attributes.size(); ++it)
       status=NXputattr(fileID, attributes[it].c_str(), (void*)avalues[it].c_str(), avalues[it].size()+1, NX_CHAR);
   char* strs=new char[values.size()*maxlen];
   for(size_t i=0;i<values.size();i++)
   {
       strncpy(&strs[i*maxlen],values[i].c_str(),maxlen);
       //strs[i]=static_cast<char*> values[i].c_str();
   }
   status=NXputdata(fileID, (void*)strs);
   status=NXclosedata(fileID);
   delete[] strs;
   return(true);
  }
//
// Write an NXnote entry with data giving parameter pair values for algorithm history and environment
// Use NX_CHAR instead of NX_BINARY for the parameter values to make more simple.
//
  bool NexusFileIO::writeNxNote(const std::string& noteName, const std::string& author, const std::string& date,
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
   if( ! writeNxText( "author", author, attributes, avalues) )
       return(false);
   attributes.clear();
   avalues.clear();
   if( ! writeNxText( "description", description, attributes, avalues) )
       return(false);
   if( ! writeNxText( "data", pairValues, attributes, avalues) )
       return(false);

   status=NXclosegroup(fileID);
   return(true);
  }

//
// Write sample related information to the nexus file
//}

  int NexusFileIO::writeNexusProcessedSample( const std::string& name,
							  const boost::shared_ptr<Mantid::API::Sample>& sample)
  {
   NXstatus status;

   // create sample section
   status=NXmakegroup(fileID,"sample","NXsample");
   if(status==NX_ERROR)
       return(2);
   status=NXopengroup(fileID,"sample","NXsample");
   //
   std::vector<std::string> attributes,avalues;
   // only write name if not null
   if( name.size()>0 )
       if( ! writeNxText( "name", name, attributes, avalues) )
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
   // Examine TimeSeries (Log) data and call function to write double or string data
   std::vector<Kernel::Property*> sampleProps=sample->getLogData();
   for(unsigned int i=0;i<sampleProps.size();i++)
   {
       
       std::string name=sampleProps[i]->name();
       std::string type=sampleProps[i]->type();
       //std::string value=sampleProps[i]->value();
       TimeSeriesProperty<std::string> *s_timeSeries=dynamic_cast<TimeSeriesProperty<std::string>*>(sampleProps[i]);
       if(s_timeSeries!=0)
       {
           writeNexusStringLog(s_timeSeries);
       }
       else
       {
           TimeSeriesProperty<double> *d_timeSeries=dynamic_cast<TimeSeriesProperty<double>*>(sampleProps[i]);
           TimeSeriesProperty<int> *i_timeSeries=dynamic_cast<TimeSeriesProperty<int>*>(sampleProps[i]);
           TimeSeriesProperty<bool> *b_timeSeries=dynamic_cast<TimeSeriesProperty<bool>*>(sampleProps[i]);
           if(d_timeSeries!=0)
           {
               writeNexusNumericLog<double>(d_timeSeries);
           }
           else if(i_timeSeries!=0)
           {
               writeNexusNumericLog<int>(i_timeSeries);
           }
           else if(b_timeSeries!=0)
           {
               writeNexusNumericLog<bool>(b_timeSeries);
           }
       }
   }

   status=NXclosegroup(fileID);

   return(0);
  }

//
// Read sample related information from the nexus file
//}

  int NexusFileIO::readNexusProcessedSample( boost::shared_ptr<Mantid::API::Sample>& sample)
  {
  /*!
     Read Nexus Sample section
     @param sample pointer to the workspace sample data
  */
   NXstatus status;

   //open sample entry
   status=NXopengroup(fileID,"sample","NXsample");
   if(status==NX_ERROR)
       return(1);
   //
   std::vector<std::string> attributes,avalues;
   std::string name;
   if( checkEntryAtLevel("name") )
   {
       readNxText( "name", name, attributes, avalues);
   }
   else
       name="";  // Missing name entry, set null

   sample->setName(name);
   // Read proton_charge, if available. Note that TOFRaw has this at the NXentry level, though there is
   // some debate if this is appropriate. Hence for Mantid read it from the NXsample section as it is stored in Sample.
   double totalProtonCharge;
   if( checkEntryAtLevel("proton_charge") )
   {
       if( readNxFloat( "proton_charge", totalProtonCharge, attributes, avalues) )
       {
           sample->setProtonCharge(totalProtonCharge);
           if(attributes.size()==1) // check units if present
               if(attributes[0].compare("units")==0 && avalues[0].compare("microAmps*hour")!=0)
                   g_log.warning("Unexpected units of Proton charge ignored: " + avalues[0]);
       }
   }
   //
   char *nxname,*nxclass;
   int nxdatatype;
   nxname= new char[NX_MAXNAMELEN];
   nxclass = new char[NX_MAXNAMELEN];
   //
   // search for NXlog sections to read
   status=NXinitgroupdir(fileID); // just in case
   while( (status=NXgetnextentry(fileID,nxname,nxclass,&nxdatatype)) == NX_OK )
   {
      std::string nxClass=nxclass;
      if(nxClass=="NXlog")
      {
         readNXlog(nxname,sample);
      }
   }
   delete[] nxname;
   delete[] nxclass;
   //
   status=NXclosegroup(fileID);
   return((status==NX_ERROR)?3:0);
  }

  void NexusFileIO::writeNexusDoubleLog(const TimeSeriesProperty<double> *d_timeSeries)
  {
   // write NXlog section for double values
   NXstatus status;
   // get a name for the log, possibly removing the the path component
   std::string logName=d_timeSeries->name();
   size_t ipos=logName.find_last_of("/\\");
   if(ipos!=std::string::npos)
       logName=logName.substr(ipos+1);
   // extract values from timeseries
   std::vector<std::string> dV=d_timeSeries->time_tValue();
   std::vector<double> values;
   std::vector<double> times;
   time_t t0; // ,time;
   bool first=true;
   for(size_t i=0;i<dV.size();i++)
   {
       std::stringstream ins;
       double val;
       time_t time;
       ins << dV[i];
       ins >> time >> val;
       values.push_back(val);
       if(first)
       {
           t0=time; // start time of log
           first=false;
       }
       times.push_back(static_cast<double>(time-t0));
   }
   // create log
   status=NXmakegroup(fileID,logName.c_str(),"NXlog");
   if(status==NX_ERROR)
       return;
   status=NXopengroup(fileID,logName.c_str(),"NXlog");
   // write log data
   std::vector<std::string> attributes,avalues;
   writeNxFloatArray("value", values,  attributes, avalues);
   // get ISO time, if t0 valid
   char buffer [25];
   if(t0>0)
   {
      strftime (buffer,25,"%Y-%m-%dT%H:%M:%S",localtime(&t0));
      attributes.push_back("start");
      avalues.push_back(buffer);
   }
   else
   {
       g_log.warning("Bad start time in log file " + logName);
   }

   writeNxFloatArray("time", times,  attributes, avalues);
   //
   status=NXclosegroup(fileID);
   //
  }

  void NexusFileIO::writeNexusStringLog(const TimeSeriesProperty<std::string> *s_timeSeries)
  {
  // write NXlog section for string log values - not yet testedF and may not be supported by Nexus NXlog
   NXstatus status;
   // get a name for the log, possibly removing the the path component
   std::string logName=s_timeSeries->name();
   size_t ipos=logName.find_last_of("/\\");
   if(ipos!=std::string::npos)
       logName=logName.substr(ipos+1);
   // extract values from timeseries
   std::vector<std::string> dV=s_timeSeries->time_tValue();
   std::vector<std::string> values;
   std::vector<double> times;
   time_t t0;
   bool first=true;
   for(size_t i=0;i<dV.size();i++)
   {
       std::stringstream ins;
       std::string val;
       time_t time;
       ins << dV[i];
       ins >> time;
       std::getline(ins,val); // this is wrong, val only gets first word from string
       size_t found;
       found=val.find_first_not_of(" \t");
       if(found==std::string::npos)
           values.push_back(val);
       else
           values.push_back(val.substr(found));
       if(first)
       {
           t0=time; // start time of log
           first=false;
       }
       times.push_back(static_cast<double>(time-t0));
   }
   // create log
   status=NXmakegroup(fileID,logName.c_str(),"NXlog");
   if(status==NX_ERROR)
       return;
   status=NXopengroup(fileID,logName.c_str(),"NXlog");
   // write log data
   std::vector<std::string> attributes,avalues;
   writeNxStringArray("value", values,  attributes, avalues);
   // get ISO time, if t0 valid
   char buffer [25];
   if(t0>0)
   {
      strftime (buffer,25,"%Y-%m-%dT%H:%M:%S",localtime(&t0));
      attributes.push_back("start");
      avalues.push_back(buffer);
   }
   else
   {
       g_log.warning("Bad start time in log file " + logName);
   }

   writeNxFloatArray("time", times,  attributes, avalues);
   //
   status=NXclosegroup(fileID);
   //
  }

  void NexusFileIO::readNXlog(const char* nxname, boost::shared_ptr<Mantid::API::Sample>& sample)
  {
   // read an NXlog section and save the timeseries data within the sample
   NXstatus status;
   status=NXopengroup(fileID,nxname,"NXlog");
   if(status==NX_ERROR) return;
   //
   int stat,rank,dims[4],type;
   char values[]="value",time[]="time"; // section names
   //
   // read time values
   status=NXopendata(fileID,time);
   if(status==NX_ERROR)
   {
       status=NXclosegroup(fileID);
       return;
   }
   status=NXgetinfo(fileID,&rank,dims,&type);
   double* timeVals= new double[dims[0]];
   status=NXgetdata(fileID,timeVals);

   char buffer[26];
   int length=25;
   type=NX_CHAR;
   status=NXgetattr(fileID, "start", buffer, &length, &type);
   if(status==NX_ERROR)
   {
       g_log.warning("readNXlog found NXlog with no start time - ignoring log ");
       NXclosedata(fileID);
       NXclosegroup(fileID);
       return;
   }
   buffer[length]='\0';
   //struct tm *tm;
   std::string startT=buffer;
   if( (startT.find('T')) >0 )
       startT.replace(startT.find('T'),1," ");
   //boost::posix_time::ptime pt=boost::posix_time::time_from_string(startT);
   //time_t startTimeOld=to_time_t(pt);
   time_t startTime=createTime_t_FromString(startT);
   //strptime(buffer,"%Y-%m-%dT%H:%M:%S",tm);
   //time_t startTime = mktime(tm);
   status=NXclosedata(fileID);
   // read data values
   stat=NXopendata(fileID,values);
   stat=NXgetinfo(fileID,&rank,dims,&type);
   if(type==NX_FLOAT64)
   {
      TimeSeriesProperty<double> *l_PropertyDouble = new TimeSeriesProperty<double>(nxname);
      double* dataVals= new double[dims[0]];
      status=NXgetdata(fileID,dataVals);
      for( int j=0;j<dims[0];j++)
      {
          l_PropertyDouble->addValue(startTime+static_cast<time_t>(timeVals[j]), dataVals[j]);
      }
      char aname[] = "type";
      char adata[NX_MAXNAMELEN];
      int iDataLen = NX_MAXNAMELEN,iType = NX_CHAR;
      NXgetattr(fileID, aname, adata, &iDataLen, &iType);
      std::string logType = std::string(adata,iDataLen);
      if (logType == "int")
      {
          TimeSeriesProperty<int> *l_Property = new TimeSeriesProperty<int>(nxname);
          std::map<dateAndTime, double> pd = l_PropertyDouble->valueAsMap();
          for(std::map<dateAndTime, double>::const_iterator p=pd.begin();p!=pd.end();p++)
          {
              l_Property->addValue(p->first,int(p->second));
          }
          delete l_PropertyDouble;
          sample->addLogData(l_Property);
      }
      else if (logType == "bool")
      {
          TimeSeriesProperty<bool> *l_Property = new TimeSeriesProperty<bool>(nxname);
          std::map<dateAndTime, double> pd = l_PropertyDouble->valueAsMap();
          for(std::map<dateAndTime, double>::const_iterator p=pd.begin();p!=pd.end();p++)
          {
              l_Property->addValue(p->first,p->second != 0.0);
          }
          delete l_PropertyDouble;
          sample->addLogData(l_Property);
      }
      else
      {
          sample->addLogData(l_PropertyDouble);
      }
   }
   else if(type==NX_CHAR)
   {
      TimeSeriesProperty<std::string> *l_PropertyString = new TimeSeriesProperty<std::string>(nxname);
      char* dataVals= new char[dims[0]*dims[1]];
      status=NXgetdata(fileID,(void *)dataVals);
      char* value=new char[dims[1]+1];
      value[dims[1]]='\0';
      for( int j=0;j<dims[0];j++)
      {
          value=strncpy(value,&dataVals[j*dims[1]],dims[1]);
          l_PropertyString->addValue(startTime+static_cast<time_t>(timeVals[j]), value);
      }
      delete[] value;
      sample->addLogData(l_PropertyString);
   }
   status=NXclosedata(fileID);
   status=NXclosegroup(fileID);
  }


  int NexusFileIO::writeNexusProcessedData( const API::MatrixWorkspace_const_sptr& localworkspace,
							const bool& uniformSpectra, const std::vector<int>& spec)
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
   const int nSpectBins=localworkspace->readY(0).size();
   const int nSpect=int(spec.size());
   int dims_array[2] = { nSpect,nSpectBins };
   std::string name=localworkspace->getTitle();
   if(name.size()==0)
	   name="values";
   int start[2]={0,0},asize[2]={1,dims_array[1]};
   status=NXcompmakedata(fileID, name.c_str(), NX_FLOAT64, 2, dims_array,m_nexuscompression,asize);
   status=NXopendata(fileID, name.c_str());
   for(int i=0;i<nSpect;i++)
   {
       int s = spec[i];
      status=NXputslab(fileID, (void*)&(localworkspace->readY(s)[0]),start,asize);
	  start[0]++;
   }
   int signal=1;
   status=NXputattr (fileID, "signal", &signal, 1, NX_INT32);
   // Set the axis labels and values
   Mantid::API::Axis *xAxis=localworkspace->getAxis(0);
   Mantid::API::Axis *sAxis=localworkspace->getAxis(1);
   std::string xLabel,sLabel;
   if ( xAxis->isSpectra() ) xLabel = "spectraNumber";
   else
   {
     if ( xAxis->unit() ) xLabel = xAxis->unit()->unitID();
     else xLabel = "unknown";
   }
   if ( sAxis->isSpectra() ) sLabel = "spectraNumber";
   else
   {
     if ( sAxis->unit() ) sLabel = sAxis->unit()->unitID();
     else sLabel = "unknown";
   }
   std::vector<double> histNumber;
   for(int i=0;i<nSpect;i++) 
   {
       int s = spec[i];
       histNumber.push_back((*sAxis)(s));
   }
   //const std::string axesNames=xLabel+":"+sLabel;
   const std::string axesNames="axis1,axis2";
   status=NXputattr (fileID, "axes", (void*)axesNames.c_str(), axesNames.size(), NX_CHAR);
   std::string yUnits=localworkspace->YUnit();
   status=NXputattr (fileID, "units", (void*)yUnits.c_str(), yUnits.size(), NX_CHAR);
   status=NXclosedata(fileID);
   // error
   name="errors";
   status=NXcompmakedata(fileID, name.c_str(), NX_FLOAT64, 2, dims_array,m_nexuscompression,asize);
   status=NXopendata(fileID, name.c_str());
   start[0]=0;
   for(int i=0;i<nSpect;i++)
   {
       int s = spec[i];
      status=NXputslab(fileID, (void*)&(localworkspace->readE(s)[0]),start,asize);
	  start[0]++;
   }
   status=NXclosedata(fileID);
   // write X data, as single array or all values if "ragged"
   if(uniformSpectra)
   {
	   dims_array[0]=localworkspace->readX(0).size();
	   status=NXmakedata(fileID, "axis1", NX_FLOAT64, 1, dims_array);
       status=NXopendata(fileID, "axis1");
	   status=NXputdata(fileID, (void*)&(localworkspace->readX(0)[0]));
   }
   else
   {
	   dims_array[0]=nSpect;
	   dims_array[1]=localworkspace->readX(0).size();
	   status=NXmakedata(fileID, "axis1", NX_FLOAT64, 2, dims_array);
	   status=NXopendata(fileID, "axis1");
	   start[0]=0; asize[1]=dims_array[1];
	   for(int i=0;i<nSpect;i++)
	   {
	     status=NXputslab(fileID, (void*)&(localworkspace->readX(i)[0]),start,asize);
	     start[0]++;
	   }
   }
   std::string dist=(localworkspace->isDistribution()) ? "1" : "0";
   status=NXputattr(fileID, "distribution", (void*)dist.c_str(), 2, NX_CHAR);
   NXputattr (fileID, "units", (void*)xLabel.c_str(), xLabel.size(), NX_CHAR);
   status=NXclosedata(fileID);
   // write axis2, maybe just spectra number
   dims_array[0]=nSpect;
   status=NXmakedata(fileID, "axis2", NX_FLOAT64, 1, dims_array);
   status=NXopendata(fileID, "axis2");
   status=NXputdata(fileID, (void*)&(histNumber[0]));
   NXputattr (fileID, "units", (void*)sLabel.c_str(), sLabel.size(), NX_CHAR);
   status=NXclosedata(fileID);

   status=NXclosegroup(fileID);
   return((status==NX_ERROR)?3:0);
  }

  int NexusFileIO::getWorkspaceSize( int& numberOfSpectra, int& numberOfChannels, int& numberOfXpoints ,
      bool& uniformBounds, std::string& axesUnits, std::string& yUnits )
  {
   //
   // Read the size of the data section in a mantid_workspace_entry and also get the names of axes
   //
   NXstatus status;
   //open workspace group
   status=NXopengroup(fileID,"workspace","NXdata");
   if(status==NX_ERROR)
       return(1);
   // open "values" data which is identified by attribute "signal", if it exists
   std::string entry;
   if(checkEntryAtLevelByAttribute("signal", entry))
       status=NXopendata(fileID, entry.c_str());
   else
   {
       status=NXclosegroup(fileID);
       return(2);
   }
   if(status==NX_ERROR)
   {
       status=NXclosegroup(fileID);
       return(2);
   }
   // read workspace data size
   int rank,dim[2],type;
   status=NXgetinfo(fileID, &rank, dim, &type);
   if(status==NX_ERROR)
       return(3);
   numberOfSpectra=dim[0];
   numberOfChannels=dim[1];
   // get axes attribute
   char sbuf[NX_MAXNAMELEN];
   int len=NX_MAXNAMELEN;
   type=NX_CHAR;
   //
   len=NX_MAXNAMELEN;
   if(checkAttributeName("units"))
   {
      status=NXgetattr(fileID,"units",(void *)sbuf,&len,&type);
      if(status!=NX_ERROR)
         yUnits=sbuf;
      status=NXclosedata(fileID);
   }
   //
   // read axis1 size
   status=NXopendata(fileID,"axis1");
   if(status==NX_ERROR)
       return(4);
   len=NX_MAXNAMELEN;
   type=NX_CHAR;
   NXgetattr(fileID,"units",(void *)sbuf,&len,&type);
   axesUnits = std::string(sbuf,len);
   status=NXgetinfo(fileID, &rank, dim, &type);
   // non-uniform X has 2D axis1 data
   if(rank==1)
   {
       numberOfXpoints=dim[0];
       uniformBounds=true;
   }
   else
   {
       numberOfXpoints=dim[1];
       uniformBounds=false;
   }
   NXclosedata(fileID);
   status=NXopendata(fileID,"axis2");
   len=NX_MAXNAMELEN;
   type=NX_CHAR;
   NXgetattr(fileID,"units",(void *)sbuf,&len,&type);
   axesUnits += std::string(":") + std::string(sbuf,len);
   NXclosedata(fileID);
   status=NXclosegroup(fileID);
   return(0);
  }

  bool NexusFileIO::checkAttributeName(const std::string& target) const
  {
    // see if the given attribute name is in the current level
    // return true if it is.
    NXstatus status;
    int length=NX_MAXNAMELEN,type;
    status=NXinitattrdir(fileID);
    char aname[NX_MAXNAMELEN];
    //    char avalue[NX_MAXNAMELEN]; // value is not restricted to this, but it is a reasonably large value
    while(NXgetnextattr(fileID,aname,&length,&type)==NX_OK)
    {
       if(target.compare(aname)==0)
       {
          return true;
       }
    }
    return false;
  }

  int NexusFileIO::getXValues(std::vector<double>& xValues, const int& spectra)
  {
   //
   // find the X values for spectra. If uniform, the spectra number is ignored.
   //
   NXstatus status;
   int rank,dim[2],type,nx;

   //open workspace group
   status=NXopengroup(fileID,"workspace","NXdata");
   if(status==NX_ERROR)
       return(1);
   // read axis1 size
   status=NXopendata(fileID,"axis1");
   if(status==NX_ERROR)
       return(2);
   status=NXgetinfo(fileID, &rank, dim, &type);
   // non-uniform X has 2D axis1 data
   if(rank==2)
       nx=dim[1];
   else
       nx=dim[0];
   if(rank==1)
   {
       status=NXgetdata(fileID,&xValues[0]);
   }
   else
   {
       int start[2]={spectra,0};
       int  size[2]={1,dim[1]};
       status=NXgetslab(fileID,&xValues[0],start,size);
   }
   status=NXclosedata(fileID);
   status=NXclosegroup(fileID);
   return(0);
  }

  int NexusFileIO::getSpectra(std::vector<double>& values, std::vector<double>& errors, const int& spectra)
  {
   //
   // read the values and errors for spectra
   //
   NXstatus status;
   int rank,dim[2],type;

   //open workspace group
   status=NXopengroup(fileID,"workspace","NXdata");
   if(status==NX_ERROR)
       return(1);
   std::string entry;
   if(checkEntryAtLevelByAttribute("signal", entry))
       status=NXopendata(fileID, entry.c_str());
   else
   {
       status=NXclosegroup(fileID);
       return(2);
   }
   if(status==NX_ERROR)
   {
       status=NXclosegroup(fileID);
       return(2);
   }
   status=NXgetinfo(fileID, &rank, dim, &type);
   // get buffer and block size
   int start[2]={spectra-1,0};
   int  size[2]={1,dim[1]};
   status=NXgetslab(fileID,&values[0],start,size);
   status=NXclosedata(fileID);

   // read errors
   status=NXopendata(fileID,"errors");
   if(status==NX_ERROR)
       return(2);
   status=NXgetinfo(fileID, &rank, dim, &type);
   // set block size;
   size[1]=dim[1];
   status=NXgetslab(fileID,&errors[0],start,size);
   status=NXclosedata(fileID);

   status=NXclosegroup(fileID);
   
   return(0);
  }


  /** Write the algorithm and environment information.
   *  @param localworkspace The workspace
   */
  int NexusFileIO::writeNexusProcessedProcess(const API::MatrixWorkspace_const_sptr& localworkspace)
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
   for(unsigned int i=0;i<algHist.size();i++)
   {
       std::stringstream algNumber,algData;
       algNumber << "MantidAlgorithm_" << i;
       algHist[i].printSelf(algData);
       writeNxNote(algNumber.str(),"mantid","","Mantid Algorithm data",algData.str());
   }
   status=NXclosegroup(fileID);
   return(0);
  }

  int NexusFileIO::findMantidWSEntries()
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

  bool NexusFileIO::checkEntryAtLevel(const std::string& item) const
  {
   // Search the currently open level for name "item"
   NXstatus status;
   char *nxname,*nxclass;
   int nxdatatype;
   nxname= new char[NX_MAXNAMELEN];
   nxclass = new char[NX_MAXNAMELEN];
   //
   // read nexus fields at this level
   status=NXinitgroupdir(fileID); // just in case
   while( (status=NXgetnextentry(fileID,nxname,nxclass,&nxdatatype)) == NX_OK )
   {
      std::string nxName=nxname;
      if(nxName==item)
      {
         delete[] nxname;
         delete[] nxclass;
         return(true);
      }
   }
   delete[] nxname;
   delete[] nxclass;
   return(false);
  }
  

  bool NexusFileIO::checkEntryAtLevelByAttribute(const std::string& attribute, std::string& entry) const
  {
   // Search the currently open level for a section with "attribute" and return entry name
   NXstatus status;
   char *nxname,*nxclass;
   int nxdatatype;
   nxname= new char[NX_MAXNAMELEN];
   nxclass = new char[NX_MAXNAMELEN];
   //
   // read nexus fields at this level
   status=NXinitgroupdir(fileID); // just in case
   while( (status=NXgetnextentry(fileID,nxname,nxclass,&nxdatatype)) == NX_OK )
   {
      std::string nxName=nxname;
      status=NXopendata(fileID,nxname);
      if(checkAttributeName("signal"))
      {
         entry=nxname;
         delete[] nxname;
         delete[] nxclass;
         status=NXclosedata(fileID);
         return(true);
      }
      status=NXclosedata(fileID);
   }
   delete[] nxname;
   delete[] nxclass;
   return(false);

  }
  

  bool NexusFileIO::writeNexusProcessedSpectraMap(const API::MatrixWorkspace_const_sptr& localWorkspace, const std::vector<int>& spec)
  {
   /*! Write the details of the spectra detector mapping to the Nexus file using the format proposed for
       Muon data, but using only one NXdetector section for the whole instrument.
       Also do not place other data the Muon NXdetector would hold.
       NXdetector section to be placed in existing NXinstrument.
       return should leave Nexus at entry level.
       @param localWorkspace The workspace
       @param spec A vector with spectra indeces
   */

   const SpectraDetectorMap& spectraMap=localWorkspace->spectraMap();
   API::Axis *spectraAxis = localWorkspace->getAxis(1);
   const int nDetectors = spectraMap.nElements();
   if(nDetectors<1)
   {
       // No data in spectraMap to write
       g_log.warning("No spectramap data to write");
       return(false);
   }
   NXstatus status;
   status=NXopengroup(fileID,"instrument","NXinstrument");
   if(status==NX_ERROR)
   {
       return(false);
   }
   //
   status=NXmakegroup(fileID,"detector","NXdetector");
   if(status==NX_ERROR)
   {
       NXclosegroup(fileID);
       return(false);
   }
   status=NXopengroup(fileID,"detector","NXdetector");
   //
   int numberSpec=int(spec.size());
   // allocate space for the Nexus Muon format of spctra-detector mapping
   int *detector_index=new int[numberSpec+1];  // allow for writing one more than required
   int *detector_count=new int[numberSpec];
   int *detector_list=new int[nDetectors];
   int *spectra=new int[numberSpec];
   double *detPos = new double[nDetectors*3];
   detector_index[0]=0;
   int id=0;

   int ndet = 0;
   // get data from map into Nexus Muon format
   for(int i=0;i<numberSpec;i++)
   {
       int si = spec[i];
       spectra[i] = spectraAxis->spectraNo(si);
       const int ndet1=spectraMap.ndet(spectra[i]);
       detector_index[i+1]=detector_index[i]+ndet1; // points to start of detector list for the next spectrum
       detector_count[i]=ndet1;
       ndet += ndet1;

       const std::vector<int> detectorgroup = spectraMap.getDetectors(spectra[i]);
       std::vector<int>::const_iterator it;
       for (it=detectorgroup.begin();it!=detectorgroup.end();it++)
       {
           detector_list[id++]=(*it);
       }
   }
   // write data as Nexus sections detector{index,count,list}
   int dims[2] = { numberSpec, 0 };
   status=NXcompmakedata(fileID, "detector_index", NX_INT32, 1, dims,m_nexuscompression,dims);
   status=NXopendata(fileID, "detector_index");
   status=NXputdata(fileID, (void*)detector_index);
   status=NXclosedata(fileID);
   //
   status=NXcompmakedata(fileID, "detector_count", NX_INT32, 1, dims,m_nexuscompression,dims);
   status=NXopendata(fileID, "detector_count");
   status=NXputdata(fileID, (void*)detector_count);
   status=NXclosedata(fileID);
   //
   dims[0]=ndet;
   status=NXcompmakedata(fileID, "detector_list", NX_INT32, 1, dims, m_nexuscompression,dims);
   status=NXopendata(fileID, "detector_list");
   status=NXputdata(fileID, (void*)detector_list);
   status=NXclosedata(fileID);
   //
   dims[0]=numberSpec;
   status=NXcompmakedata(fileID, "spectra", NX_INT32, 1, dims, m_nexuscompression,dims);
   status=NXopendata(fileID, "spectra");
   status=NXputdata(fileID, (void*)spectra);
   status=NXclosedata(fileID);
   //
   try
   {
       Mantid::Geometry::IObjComponent_const_sptr sample = localWorkspace->getInstrument()->getSample();
       Mantid::Geometry::V3D sample_pos = sample->getPos();
       for(int i=0;i<ndet;i++)
       {
           double R,Theta,Phi;
           try
           {
               boost::shared_ptr<Mantid::Geometry::IDetector> det = localWorkspace->getInstrument()->getDetector(detector_list[i]);
               Mantid::Geometry::V3D pos = det->getPos() - sample_pos;
               pos.getSpherical(R,Theta,Phi);
               R = det->getDistance(*sample);
               Theta = localWorkspace->detectorTwoTheta(det)*180.0/M_PI;
           }
           catch(...)
           {
               R = 0.;
               Theta = 0.;
               Phi = 0.;
           }
           // Need to get R & Theta through these methods to be correct for grouped detectors
           detPos[3*i] = R;
           detPos[3*i + 1] = Theta;
           detPos[3*i + 2] = Phi;
       }
       dims[0]=ndet;
       dims[1]=3;
       status=NXcompmakedata(fileID, "detector_positions", NX_FLOAT64, 2, dims, m_nexuscompression,dims);
       status=NXopendata(fileID, "detector_positions");
       status=NXputdata(fileID, (void*)detPos);
       status=NXclosedata(fileID);
   }
   catch(...)
   {
       g_log.error("Unknown error cought when saving detector positions.");
   }
   // tidy up
   delete[] detector_list;
   delete[] detector_index;
   delete[] detector_count;
   delete[] spectra;
   delete[] detPos;
   //
   status=NXclosegroup(fileID); // close detector group
   status=NXclosegroup(fileID); // close instrument group
   return(true);
  }
  
  bool NexusFileIO::readNexusProcessedSpectraMap(API::MatrixWorkspace_sptr localWorkspace)
  {
   /*! read the details of the spectra detector mapping to the Nexus file using the format proposed for
       Muon data. Use this to build spectraMap
       @param localWorkspace The workspace
       @return true for OK, false for error
   */

   SpectraDetectorMap& spectraMap = localWorkspace->mutableSpectraMap();
   NXstatus status;
   status=NXopengroup(fileID,"instrument","NXinstrument");
   if(status==NX_ERROR)
       return(false);
   //
   if(!checkEntryAtLevel("detector"))  // to avoid Nexus Error messages
   {
       NXclosegroup(fileID);
       return(false);
   }
   status=NXopengroup(fileID,"detector","NXdetector");
   if(status==NX_ERROR)
   {
       NXclosegroup(fileID);
       return(false);
   }
   //
   // read data from Nexus sections detector_{index,count,list}
   int dim[1],rank,type;
   status=NXopendata(fileID, "detector_index");
   status=NXgetinfo(fileID, &rank, dim, &type);
   if(status==NX_ERROR || rank!=1 || type!=NX_INT32)
   {
       status=NXclosedata(fileID);
       status=NXclosegroup(fileID);
       return(false);
   }
   int nSpectra=dim[0];
   int *detector_index=new int[nSpectra];
   int *detector_count=new int[nSpectra];
   status=NXgetdata(fileID, (void*)detector_index);
   status=NXclosedata(fileID);
   //
   status=NXopendata(fileID, "detector_count");
   status=NXgetdata(fileID, (void*)detector_count);
   status=NXclosedata(fileID);
   //
   status=NXopendata(fileID, "detector_list");
   status=NXgetinfo(fileID, &rank, dim, &type);
   int nDet=dim[0];
   int *detector_list=new int[nDet];
   status=NXgetdata(fileID, (void*)detector_list);
   status=NXclosedata(fileID);

   int *spectra = NULL;
   status=NXopendata(fileID, "spectra");
   if (status == NX_OK)
   {
       status=NXgetinfo(fileID, &rank, dim, &type);
       int nSpec=dim[0];
       assert(nSpec == nSpectra);
       spectra=new int[nSpec];
       status=NXgetdata(fileID, (void*)spectra);
       status=NXclosedata(fileID);
   }
   else
   {
       spectra = new int[nSpectra];
       for(int i=0;i<nSpectra;i++)
           spectra[i] = i + 1;
   }

   // build spectra_list for populate method
   int *spectra_list=new int[nDet];
   for(int i=0;i<nSpectra;i++)
   {
       int s = spectra[i];
       int offset=detector_index[i];
       for(int j=0;j<detector_count[i];j++)
       {
           spectra_list[offset+j]=s;
       }
   }
   spectraMap.populate(spectra_list,detector_list,nDet); //Populate the Spectra Map with parameters

   for(int i=0;i<nSpectra;i++)
       localWorkspace->getAxis(1)->spectraNo(i) = spectra[i];
   // tidy up
   delete[] detector_list;
   delete[] detector_index;
   delete[] detector_count;
   delete[] spectra_list;
   delete[] spectra;
   //
   status=NXclosegroup(fileID); // close detector group
   status=NXclosegroup(fileID); // close instrument group
   return(true);
  }


  bool NexusFileIO::writeNexusParameterMap(API::MatrixWorkspace_const_sptr ws)
  {
   /*! Writes the instrument parameter map if not empty. Must be called inside NXentry group.
       @param ws The workspace
       @return true for OK, false for error
   */

      const boost::shared_ptr<const Geometry::ParameterMap> params = ws->instrumentParameters();
      std::string str = params->asString();
      if (str.empty()) str = " ";
      return writeNxNote("instrument_parameter_map"," "," "," ",str);
   
  }

  bool NexusFileIO::readNexusParameterMap(API::MatrixWorkspace_sptr ws)
  {
   /*! Reads the instrument parameter map from the data string in "instrument_parameter_map" group (NXnote).
       Must be called inside NXentry group.
       @param ws The workspace
       @return true for OK, false for error
   */

      if (NX_ERROR == NXopengroup(fileID,"instrument_parameter_map","NXnote")) return false;
      //
      std::string value;
      std::vector<std::string> avalues,attributes;

      if( ! readNxText( "data", value, attributes, avalues) )
          return(false);

      boost::shared_ptr<Geometry::ParameterMap> params = ws->instrumentParameters();
      params->clear();
      IInstrument_sptr instr = ws->getBaseInstrument();

      typedef boost::tokenizer<boost::char_separator<char> > tokenizer;

      boost::char_separator<char> sep("|");
      tokenizer parTok(value, sep);
      // Iterate over the properties
      for (tokenizer::iterator it = parTok.begin(); it != parTok.end(); ++it)
      {
          if ( ! it->empty() )
          {
              tokenizer itemTok(*it,boost::char_separator<char>(";"));
              tokenizer::iterator i = itemTok.begin();
              std::string compName = *i++;
              std::string type = *i++;
              std::string name = *i++;
              std::string val = *i;
              Geometry::IComponent* comp = instr->getComponentByName(compName).get();
              if (!comp) continue;
              params->add(type,comp,name,val);
          }
      }

      NXclosegroup(fileID);
      return(true);
   
  }

  template<>
  std::string NexusFileIO::logValueType<double>()const{return "double";}

  template<>
  std::string NexusFileIO::logValueType<int>()const{return "int";}

  template<>
  std::string NexusFileIO::logValueType<bool>()const{return "bool";}

} // namespace NeXus
} // namespace Mantid
