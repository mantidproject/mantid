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
#include "MantidGeometry/Instrument/Instrument.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidNexus/NexusFileIO.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAPI/NumericAxis.h"
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

    int NexusFileIO::closeNexusFile()
    {

      NXstatus status;
      status=NXclosegroup(fileID);
      status=NXclose(&fileID);   
      return(0);
    }

    int NexusFileIO::writeNexusProcessedHeader( const std::string& title) const
    {
      // Write Nexus mantid workspace header fields for the NXentry/IXmantid/NXprocessed field.
      // The URLs are not correct as they do not exist presently, but follow the format for other
      // Nexus specs.
      //
      std::string className="Mantid Processed Workspace";
      std::vector<std::string> attributes,avalues;
      if( ! writeNxValue<std::string>("title", title, NX_CHAR, attributes, avalues) )
        return(3);
      //
      attributes.push_back("URL");
      avalues.push_back("http://www.nexusformat.org/instruments/xml/NXprocessed.xml");
      attributes.push_back("Version");
      avalues.push_back("1.0");
      // this may not be the "correct" long term path, but it is valid at present 
      if( ! writeNxValue<std::string>( "definition", className, NX_CHAR, attributes, avalues) )
        return(3);
      avalues.clear();
      avalues.push_back("http://www.isis.rl.ac.uk/xml/IXmantid.xml");
      avalues.push_back("1.0");
      if( ! writeNxValue<std::string>( "definition_local", className, NX_CHAR, attributes, avalues) )
        return(3);
      return(0);
    }
    bool NexusFileIO::writeNexusInstrumentXmlName(const std::string& instrumentXml,const std::string& date,
      const std::string& version) const
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
      if( ! writeNxValue<std::string>( "instrument_source", instrumentXml, NX_CHAR, attributes, avalues) )
        return(false);
      return(true);
    }

    bool NexusFileIO::writeNexusInstrument(const Geometry::IInstrument_const_sptr& instrument) const
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
      if( ! writeNxValue<std::string>( "name", name, NX_CHAR, attributes, avalues) )
        return(false);

      status=NXclosegroup(fileID);

      return(true);
    }
    /** Returns true if the given property is a time series property
     * @param prop The property to test
     * @returns True if it is a time series, false otherwise
     */
    bool NexusFileIO::isTimeSeries(Kernel::Property* prop) const
    {
      if( dynamic_cast<TimeSeriesProperty<std::string>*>(prop) ||
	  dynamic_cast<TimeSeriesProperty<int>*>(prop) ||
	  dynamic_cast<TimeSeriesProperty<double>*>(prop) ||
	  dynamic_cast<TimeSeriesProperty<bool>*>(prop) )
      {
	return true;
      }
      else
      {
	return false;
      }
    }
    /** Write a time series log entry.
     * @param prop The time series property containing the data. This must be a time series or it will fail. 
     * @returns A boolean indicating success or failure
     */
    bool NexusFileIO::writeTimeSeriesLog(Kernel::Property* prop) const
    {
      bool success(true);
      if( TimeSeriesProperty<std::string> *s_timeSeries = dynamic_cast<TimeSeriesProperty<std::string>* >(prop) )
      {
	writeNumericTimeLog_String(s_timeSeries);
      }
      else if( TimeSeriesProperty<double> *d_timeSeries=dynamic_cast<TimeSeriesProperty<double>*>(prop) )
      {
	writeNumericTimeLog<double>(d_timeSeries);
      }
      else if( TimeSeriesProperty<int> *i_timeSeries=dynamic_cast<TimeSeriesProperty<int>*>(prop) )
      {
	writeNumericTimeLog<int>(i_timeSeries);
      }
      else if( TimeSeriesProperty<bool> *b_timeSeries=dynamic_cast<TimeSeriesProperty<bool>*>(prop) )
      {
	writeNumericTimeLog<bool>(b_timeSeries);
      }
      else
      {
	success = false;
      }
      return success;
    }
    /** Write a single-valued log entry 
     * @param prop The property containing the data.
     * @returns A boolean indicating success or failure
     */
    bool NexusFileIO::writeSingleValueLog(Kernel::Property* prop) const
    {
      std::vector<std::string> attrs(0);
      std::vector<std::string> attrValues(0);
      bool success(true);
      if( PropertyWithValue<std::string> *strProp = dynamic_cast<PropertyWithValue<std::string>*>(prop) )
      {
	std::string value = strProp->value();
	if( value.empty() ) value = " ";
	writeSingleValueNXLog(strProp->name(), strProp->value(), NX_CHAR, attrs, attrValues);
      }
      else if(PropertyWithValue<double> *dblProp = dynamic_cast<PropertyWithValue<double>*>(prop) )
      {
	try
	{
	  double value = boost::lexical_cast<double>(dblProp->value());
	  writeSingleValueNXLog(dblProp->name(), value, NX_FLOAT64, attrs, attrValues);
	}
	catch(boost::bad_lexical_cast &)
	{
	  success = true;
	}
      }
      else if(PropertyWithValue<int> *intProp = dynamic_cast<PropertyWithValue<int>*>(prop) )
      {
	try
	{
	  int value = boost::lexical_cast<int>(intProp->value());
	  writeSingleValueNXLog(intProp->name(), value, NX_INT32, attrs, attrValues);
	}
	catch(boost::bad_lexical_cast &)
	{
	  success = true;
	}
      }
      else if(PropertyWithValue<bool> *boolProp = dynamic_cast<PropertyWithValue<bool>*>(prop) )
      {
	try
	{
	  bool value = boost::lexical_cast<bool>(boolProp->value());
	  writeSingleValueNXLog(boolProp->name(), value, NX_UINT8, attrs, attrValues);
	}
	catch(boost::bad_lexical_cast &)
	{
	  success = true;
	}
      }
      else
      {
	success = false;
      }
      return success;
    }

    //
    // write an NXdata entry with Float array values
    //
    bool NexusFileIO::writeNxFloatArray(const std::string& name, const std::vector<double>& values, const std::vector<std::string>& attributes,
      const std::vector<std::string>& avalues) const
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
      const std::vector<std::string>& avalues) const
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
      const std::string& description, const std::string& pairValues) const
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
      if( ! writeNxValue<std::string>( "author", author, NX_CHAR, attributes, avalues) )
        return(false);
      attributes.clear();
      avalues.clear();
      if( ! writeNxValue<std::string>( "description", description, NX_CHAR, attributes, avalues) )
        return(false);
      if( ! writeNxValue<std::string>( "data", pairValues, NX_CHAR, attributes, avalues) )
        return(false);

      status=NXclosegroup(fileID);
      return(true);
    }


    //-------------------------------------------------------------------------------------
    //
    // Write sample related information to the nexus file
    //

    int NexusFileIO::writeNexusProcessedSample( const std::string& name, const API::Sample & sample,
						const Mantid::API::Run& runProperties) const
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
        if( ! writeNxValue<std::string>( "name", name, NX_CHAR, attributes, avalues) )
          return(3);
      // Write proton_charge here, if available and there's no log with the same name. Note that TOFRaw has this at the NXentry level, though there is
      // some debate if this is appropriate. Hence for Mantid write it to the NXsample section as it is stored in Sample.
      if( !runProperties.hasProperty("proton_charge") )
      {
        try
        {
          double totalProtonCharge = runProperties.getProtonCharge();
          attributes.push_back("units");
          avalues.push_back("microAmps*hour");
          if( ! writeNxValue<double>( "proton_charge", totalProtonCharge, NX_FLOAT64, attributes, avalues) )
            return(4);
        }
        catch(Exception::NotFoundError&)
        {
        }
      }

      // Examine (Log) data and call function to write double or string data
      std::vector<Kernel::Property*> sampleProps = runProperties.getLogData();
      size_t nlogs = sampleProps.size();
      for(unsigned int i=0; i < nlogs; i++)
      {
        Kernel::Property *prop = sampleProps[i];
        if( isTimeSeries(prop) )
        {
          if( !writeTimeSeriesLog(prop) ) return 5;
        }
        else
        {
          if( !writeSingleValueLog(prop) ) return 5;
        }
      }

      // Sample geometry
      attributes = std::vector<std::string>(0);
      avalues = std::vector<std::string>(0);
      writeNxValue<int>("geom_id", sample.getGeometryFlag(), NX_INT32, attributes, avalues);
      writeNxValue<double>("geom_thickness", sample.getThickness(), NX_FLOAT64, attributes, avalues);
      writeNxValue<double>("geom_width", sample.getWidth(), NX_FLOAT64, attributes, avalues);
      writeNxValue<double>("geom_height", sample.getHeight(), NX_FLOAT64, attributes, avalues);
      status=NXclosegroup(fileID);

      return(0);
    }


    //-------------------------------------------------------------------------------------
    void NexusFileIO::writeNumericTimeLog_String(const TimeSeriesProperty<std::string> *s_timeSeries) const
    {
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
      Kernel::dateAndTime t0;
      bool first=true;
      for(size_t i=0;i<dV.size();i++)
      {
        std::stringstream ins;
        std::string val;
        Kernel::dateAndTime time;
        ins << dV[i];
        ins >> time;
        /** MG 27/07/2010: The comment below was here when refactored. As the code is being used heavily and no bug has
            been reported about chopped off entries, nothing was changed. */
        // this is wrong, val only gets first word from string
        std::getline(ins,val); 
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
        times.push_back(Kernel::DateAndTime::durationInSeconds(time-t0));
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
      avalues.push_back(Kernel::DateAndTime::create_ISO8601_String(t0));

      writeNxFloatArray("time", times,  attributes, avalues);
      status=NXclosegroup(fileID);
    }


    //-------------------------------------------------------------------------------------
    int NexusFileIO::writeNexusProcessedData( const API::MatrixWorkspace_const_sptr& localworkspace,
      const bool& uniformSpectra, const std::vector<int>& spec) const
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
      std::string name="values";
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
      // Get the values on the vertical axis
      std::vector<double> axis2;
      if (nSpect < nHist)
        for (int i=0;i<nSpect;i++) 
          axis2.push_back((*sAxis)(spec[i]));
      else
        for (int i=0;i<sAxis->length();i++)
          axis2.push_back((*sAxis)(i));

      const std::string axesNames="axis1,axis2";
      status=NXputattr (fileID, "axes", (void*)axesNames.c_str(), axesNames.size(), NX_CHAR);
      std::string yUnits=localworkspace->YUnit();
      std::string yUnitLabel=localworkspace->YUnitLabel();
      status=NXputattr (fileID, "units", (void*)yUnits.c_str(), yUnits.size(), NX_CHAR);
      status=NXputattr (fileID, "unit_label", (void*)yUnitLabel.c_str(), yUnitLabel.size(), NX_CHAR);
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
      dims_array[0]=axis2.size();
      status=NXmakedata(fileID, "axis2", NX_FLOAT64, 1, dims_array);
      status=NXopendata(fileID, "axis2");
      status=NXputdata(fileID, (void*)&(axis2[0]));
      NXputattr (fileID, "units", (void*)sLabel.c_str(), sLabel.size(), NX_CHAR);
      status=NXclosedata(fileID);

      writeNexusBinMasking(localworkspace);

      status=NXclosegroup(fileID);
      return((status==NX_ERROR)?3:0);
    }

    int NexusFileIO::getWorkspaceSize( int& numberOfSpectra, int& numberOfChannels, int& numberOfXpoints ,
      bool& uniformBounds, std::string& axesUnits, std::string& yUnits ) const
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
        status=NXgetattr(fileID,(char*)"units",(void *)sbuf,&len,&type);
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
      NXgetattr(fileID,(char*)"units",(void *)sbuf,&len,&type);
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
      NXgetattr(fileID,(char*)"units",(void *)sbuf,&len,&type);
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

    int NexusFileIO::getXValues(MantidVec& xValues, const int& spectra) const
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

    int NexusFileIO::getSpectra(MantidVec& values, MantidVec& errors, const int& spectra) const
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
    *  @return 0 on success
    */
    int NexusFileIO::writeNexusProcessedProcess(const API::MatrixWorkspace_const_sptr& localworkspace) const
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
	    typedef std::map <unsigned int,std::string> orderedHistMap;
	  orderedHistMap ordMap;
      for(unsigned int i=0;i<algHist.size();i++)
      {
        std::stringstream algNumber,algData;
       // algNumber << "MantidAlgorithm_" << i;
        algHist[i].printSelf(algData);
		
		 //get execute count
	   int nexecCount=algHist[i].execCount();
	   //order by execute count
	    ordMap.insert(orderedHistMap::value_type(nexecCount,algData.str()));
	    //writeNxNote(algNumber.str(),"mantid","","Mantid Algorithm data",algData.str());
      }
	  int num=0;
	  std::map <unsigned int,std::string>::iterator m_Iter;
	  for (m_Iter=ordMap.begin( );m_Iter!=ordMap.end( );++m_Iter)
	  {
		  ++num;
		  std::stringstream algNumber;
		  algNumber << "MantidAlgorithm_" << num;
		  writeNxNote(algNumber.str(),"mantid","","Mantid Algorithm data",m_Iter->second);
	  }
      status=NXclosegroup(fileID);
      return(0);
    }

    int NexusFileIO::findMantidWSEntries() const
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

      /*! Write the details of the spectra detector mapping to the Nexus file using the format proposed for
      Muon data, but using only one NXdetector section for the whole instrument.
      Also do not place other data the Muon NXdetector would hold.
      NXdetector section to be placed in existing NXinstrument.
      return should leave Nexus at entry level.
      @param localWorkspace The workspace
      @param spec A vector with spectra indeces
      @return true on success
      */
    bool NexusFileIO::writeNexusProcessedSpectraMap(const API::MatrixWorkspace_const_sptr& localWorkspace, 
						    const std::vector<int>& spec) const
    {


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
        if (sample)
        {
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
        }
        else
          for(int i=0;i<3*ndet;i++)
            detPos[i] = 0.;
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

    bool NexusFileIO::writeNexusParameterMap(API::MatrixWorkspace_const_sptr ws) const
    {
      /*! Writes the instrument parameter map if not empty. Must be called inside NXentry group.
      @param ws The workspace
      @return true for OK, false for error
      */

      const Geometry::ParameterMap& params = ws->constInstrumentParameters();
      std::string str = params.asString();
      if (str.empty()) str = " ";
      return writeNxNote("instrument_parameter_map"," "," "," ",str);

    }

    /** 
      * Write bin masking information
      * @param ws The workspace
      * @return true for OK, false for error
      */
    bool NexusFileIO::writeNexusBinMasking(API::MatrixWorkspace_const_sptr ws) const
    {
      std::vector< int > spectra;
      std::vector< int > bins;
      std::vector< double > weights;
      size_t spectra_count = 0;
      size_t offset = 0;
      for(int i=0;i<ws->getNumberHistograms(); ++i)
      {
        if (ws->hasMaskedBins(i))
        {
          const API::MatrixWorkspace::MaskList& mList = ws->maskedBins(i);
          spectra.push_back(spectra_count);
          spectra.push_back(offset);
          API::MatrixWorkspace::MaskList::const_iterator it = mList.begin();
          for(;it != mList.end(); ++it)
          {
            bins.push_back(it->first);
            weights.push_back(it->second);
          }
          ++spectra_count;
          offset += mList.size();
        }
      }

      if (spectra_count == 0) return false;

      NXstatus status;

      // save spectra offsets as a 2d array of ints
      int dimensions[2];
      dimensions[0]=spectra_count;
      dimensions[1]=2;
      status=NXmakedata(fileID, "masked_spectra", NX_INT32, 2, dimensions);
      if(status==NX_ERROR) return false;
      status=NXopendata(fileID, "masked_spectra");
      const std::string description = "spectra index,offset in masked_bins and mask_weights";
      NXputattr(fileID, "description", (void*)description.c_str(), description.size()+1, NX_CHAR);
      status=NXputdata(fileID, (void*)&spectra[0]);
      status=NXclosedata(fileID);

      // save masked bin indices
      dimensions[0]=bins.size();
      status=NXmakedata(fileID, "masked_bins", NX_INT32, 1, dimensions);
      if(status==NX_ERROR) return false;
      status=NXopendata(fileID, "masked_bins");
      status=NXputdata(fileID, (void*)&bins[0]);
      status=NXclosedata(fileID);

      // save masked bin weights
      dimensions[0]=bins.size();
      status=NXmakedata(fileID, "mask_weights", NX_FLOAT64, 1, dimensions);
      if(status==NX_ERROR) return false;
      status=NXopendata(fileID, "mask_weights");
      status=NXputdata(fileID, (void*)&weights[0]);
      status=NXclosedata(fileID);

      return true;
    }


    template<>
    std::string NexusFileIO::logValueType<double>()const{return "double";}

    template<>
    std::string NexusFileIO::logValueType<int>()const{return "int";}

    template<>
    std::string NexusFileIO::logValueType<bool>()const{return "bool";}

    int getNexusEntryTypes(const std::string& fileName, std::vector<std::string>& entryName,
                           std::vector<std::string>& definition )
    {
     //
     // Try to open named Nexus file and return all entries plus the definition found for each.
     // If definition not found, try and return "analysis" field (Muon V1 files)
     // Return count of entries if OK, -1 failed to open file. Close file on exit.
     //
     NXhandle fileH;
     NXaccess mode= NXACC_READ;
     NXstatus stat=NXopen(fileName.c_str(), mode, &fileH);
     if(stat==NX_ERROR) return(-1);
     //
     entryName.clear();
     definition.clear();
     char *nxname,*nxclass;
     int nxdatatype;
     nxname= new char[NX_MAXNAMELEN];
     nxclass = new char[NX_MAXNAMELEN];
     int rank,dims[2],type;
     //
     // Loop through all entries looking for the definition section in each (or analysis for MuonV1)
     //
     std::vector<std::string> entryList;
     while( ( stat=NXgetnextentry(fileH,nxname,nxclass,&nxdatatype) ) == NX_OK )
     {
         std::string nxc(nxclass);
         if(nxc.compare("NXentry")==0)
             entryList.push_back(nxname);
     }
     // for each entry found, look for "analysis" or "definition" text data fields and return value plus entry name
     for(size_t i=0;i<entryList.size();i++)
     {
         //
         stat=NXopengroup(fileH,entryList[i].c_str(),"NXentry");
         // loop through field names in this entry
         while( ( stat=NXgetnextentry(fileH,nxname,nxclass,&nxdatatype) ) == NX_OK )
         {
             std::string nxc(nxclass),nxn(nxname);
             // if a data field
             if(nxc.compare("SDS")==0)
                 // if one of the two names we are looking for
                 if(nxn.compare("definition")==0 || nxn.compare("analysis")==0)
                 {
                     stat=NXopendata(fileH,nxname);
                     stat=NXgetinfo(fileH,&rank,dims,&type);
                     if(stat==NX_ERROR)
                         continue;
                     char* value=new char[dims[0]+1];
                     stat=NXgetdata(fileH,value);
                     if(stat==NX_ERROR)
                         continue;
                     value[dims[0]]='\0';
                     // return e.g entryName "analysis"/definition "muonTD"
                     definition.push_back(value);
                     entryName.push_back(entryList[i]);
                     delete[] value;
                     stat=NXclosegroup(fileH); // close data group, then entry
                     stat=NXclosegroup(fileH);
                     break;
                 }
         }
         /*
         stat=NXopendata(fileH,"definition");
         if(stat==NX_ERROR)
             {
             stat=NXopendata(fileH,"analysis");
       if(stat==NX_ERROR)
                 continue;
             }
         stat=NXgetinfo(fileH,&rank,dims,&type);
         if(stat==NX_ERROR || type!=NX_CHAR)
             continue;
         char* value=new char[dims[0]+1];
         stat=NXgetdata(fileH,value);
         if(stat==NX_ERROR)
             continue;
         value[dims[0]]='\0';
         definition.push_back(value);
         entryName.push_back(entryList[i]);
         delete[] value;
         stat=NXclosegroup(fileH); // close data group, then entry
         stat=NXclosegroup(fileH);
         */
     }
     stat=NXclose(&fileH);
     delete[] nxname;
     delete[] nxclass;
     return(entryName.size());
    }
  } // namespace NeXus
} // namespace Mantid
