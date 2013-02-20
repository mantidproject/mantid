// NexusFileIO
// @author Ronald Fowler
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <vector>
#include <sstream>
#include <stdlib.h>
#ifdef _WIN32
#include <io.h>
#endif /* _WIN32 */
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidNexus/NexusFileIO.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidGeometry/ISpectraDetectorMap.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/TableColumn.h"
#include "MantidDataObjects/RebinnedOutput.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/AlgorithmHistory.h"

#include <boost/tokenizer.hpp>
#include <boost/shared_ptr.hpp>
#include <Poco/File.h>

namespace Mantid
{
namespace NeXus
{
using namespace Kernel;
using namespace API;
using namespace DataObjects;

  Logger& NexusFileIO::g_log = Logger::get("NexusFileIO");

  /// Constructor that supplies a progress object
  NexusFileIO::NexusFileIO(boost::shared_ptr< ::NeXus::File> handle,  Progress *prog , const bool compression) :
          m_progress(prog)
  {
    m_filehandle = handle;
    if (compression)
      m_nexuscompression = ::NeXus::LZW;
    else
      m_nexuscompression = ::NeXus::NONE;

    int count=findMantidWSEntries();
    std::string mantidEntryName="mantid_workspace_"+boost::lexical_cast<std::string>(count+1);

    // make and open the new mantid_workspace_<n> group
    // file remains open until explict close
    m_filehandle->makeGroup(mantidEntryName,"NXentry", true);
  }

  /// Constructor that supplies a progress object
  NexusFileIO::NexusFileIO(const std::string & filename,  Progress *prog ) :
          m_nexuscompression(::NeXus::LZW),
          m_progress(prog)
  {
    this->openNexusWrite(filename);
  }

  //
  // Write out the data in a worksvn space in Nexus "Processed" format.
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

  NXaccess getNXaccessMode(const std::string& filename)
  {
    NXaccess mode(NXACC_CREATE5);

    //
    // If file to write exists, then open as is else see if the extension is xml, if so open as xml
    // format otherwise as compressed hdf5
    //
    if(Poco::File(filename).exists())
      mode = NXACC_RDWR;

    else
    {
      if( filename.find(".xml") < filename.size() || filename.find(".XML") < filename.size() )
      {
        mode = NXACC_CREATEXML;
      }
    }
    return mode;
  }

  void NexusFileIO::openNexusWrite(const std::string& fileName )
  {
    // open named file and entry - file may exist
    // @throw Exception::FileError if cannot open Nexus file for writing
    //
    NXaccess mode = getNXaccessMode(fileName);
    std::string mantidEntryName;
    //
    // If file to write exists, then open as is else see if the extension is xml, if so open as xml
    // format otherwise as compressed hdf5
    //
    if(!Poco::File(fileName).exists())
    {
      if( fileName.find(".xml") < fileName.size() || fileName.find(".XML") < fileName.size() )
      {
        m_nexuscompression = ::NeXus::NONE;
      }
      mantidEntryName="mantid_workspace_1";
    }

    // open the file and copy the handle into the NeXus::File object
    m_filehandle = boost::make_shared< ::NeXus::File>(fileName, mode);

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
    m_filehandle->makeGroup(mantidEntryName,"NXentry", true);
  }


  //-----------------------------------------------------------------------------------------------
  void NexusFileIO::closeNexusFile()
  {
    m_filehandle->closeGroup();
  }

  //-----------------------------------------------------------------------------------------------
  /**  Write Nexus mantid workspace header fields for the NXentry/IXmantid/NXprocessed field.
       The URLs are not correct as they do not exist presently, but follow the format for other
       Nexus specs.
       @param title :: title field.
  */
  int NexusFileIO::writeNexusProcessedHeader( const std::string& title) const
  {

    std::string className="Mantid Processed Workspace";
    m_filehandle->writeData("title", title);

    //
    std::vector<std::string> attributes,avalues;
    attributes.push_back("URL");
    avalues.push_back("http://www.nexusformat.org/instruments/xml/NXprocessed.xml");
    attributes.push_back("Version");
    avalues.push_back("1.0");
    // this may not be the "correct" long term path, but it is valid at present
    writeNxValue<std::string>( "definition", className, attributes, avalues);
    avalues.clear();
    avalues.push_back("http://www.isis.rl.ac.uk/xml/IXmantid.xml");
    avalues.push_back("1.0");
    writeNxValue<std::string>( "definition_local", className, attributes, avalues);

    return(0);
  }

  /// Add attributes to the name data
  void NexusFileIO::putAttr(const std::string& name, const std::vector<std::string>& attributes,
                            const std::vector<std::string>& avalues) const
  {
    m_filehandle->openData(name);
    this->putAttr(attributes, avalues);
    m_filehandle->closeData();
  }

  /// Add attributes to the currently open node
  void NexusFileIO::putAttr(const std::vector<std::string>& attributes,
                               const std::vector<std::string>& avalues) const
     {
       for(unsigned int it=0; it < attributes.size(); ++it)
       {
         m_filehandle->putAttr(attributes[it], avalues[it]);
       }
     }

  //-----------------------------------------------------------------------------------------------

  /**
   * Write a single valued entry to the Nexus file
   * @param name :: The name of the entry
   * @param value :: The value of the entry
   * @param attributes :: A list of attributes 1:1 mapped to their values in the <code>avalues</code> argument
   * @param avalues :: A list of attribute values in the same order as the <code>attributes</code> argument
   */
  template<class TYPE>
  void NexusFileIO::writeNxValue(const std::string& name, const TYPE& value,
                                 const std::vector<std::string>& attributes,
                                 const std::vector<std::string>& avalues) const
  {
    m_filehandle->writeData(name, value);
    m_filehandle->openData(name);
    this->putAttr(attributes, avalues);
    m_filehandle->closeData();
  }

  /**
   * Write a single valued NXLog entry to the Nexus file
   * @param name :: The name of the entry
   * @param value :: The value of the entry
   * @param nxType :: The nxType of the entry
   * @param attributes :: A list of attributes 1:1 mapped to their values in the <code>avalues</code> argument
   * @param avalues :: A list of attribute values in the same order as the <code>attributes</code> argument
   * @returns A boolean indicating success or failure
   */
  template<class TYPE>
  bool NexusFileIO::writeSingleValueNXLog(const std::string& name, const TYPE& value, const int nxType,
                      const std::vector<std::string>& attributes,
                      const std::vector<std::string>& avalues) const
  {
    m_filehandle->makeGroup(name, "NXlog", true);
    m_filehandle->writeData("value", value);
    this->putAttr(attributes, avalues);
    m_filehandle->closeGroup();
    return true;
  }

  /** Writes a numeric log to the Nexus file
   *  @tparam T A numeric type (double, int, bool)
   *  @param timeSeries :: A pointer to the log property
   */
  template<class T>
  void NexusFileIO::writeNumericTimeLog(const Kernel::TimeSeriesProperty<T> *timeSeries) const
  {
    // write NXlog section for double values
    // get a name for the log, possibly removing the the path component
    std::string logName=timeSeries->name();
    size_t ipos=logName.find_last_of("/\\");
    if(ipos!=std::string::npos)
      logName=logName.substr(ipos+1);
    // extract values from timeseries
    std::map<Kernel::DateAndTime, T> dV=timeSeries->valueAsMap();
    std::vector<double> values;
    std::vector<double> times;
    Kernel::DateAndTime t0;
    bool first=true;
    for(typename std::map<Kernel::DateAndTime, T>::const_iterator dv=dV.begin();dv!=dV.end();dv++)
    {
      T val = dv->second;
      Kernel::DateAndTime time = dv->first;
      values.push_back(val);
      if(first)
      {
        t0=time; // start time of log
        first=false;
      }
      times.push_back( Kernel::DateAndTime::secondsFromDuration(time-t0));
    }
    // create log
    m_filehandle->makeGroup(logName, "NXlog", true);

    // write log data
    std::vector<std::string> attributes,avalues;
    attributes.push_back("type");
    avalues.push_back(logValueType<T>());
    this->writeNxValue("value", values, attributes, avalues);

    // get ISO time, and save it as an attribute
    attributes.clear();
    avalues.clear();
    attributes.push_back("start");
    avalues.push_back( t0.toISO8601String() );
    this->writeNxValue("time", times, attributes, avalues);

    m_filehandle->closeGroup();
  }

  //-----------------------------------------------------------------------------------------------
  //
  // write an NXdata entry with String array values
  //
  void NexusFileIO::writeNxStringArray(const std::string& name, const std::vector<std::string>& values,
                                       const std::vector<std::string>& attributes,
                                       const std::vector<std::string>& avalues) const
  {
    // calculate the dimensions
    std::vector<int64_t> dimensions(2);
    size_t maxlen=0;
    dimensions[0]=static_cast<int64_t>(values.size());
    for(size_t i=0;i<values.size();i++)
      if(values[i].size()>maxlen) maxlen=values[i].size();
    dimensions[1]=static_cast<int64_t>(maxlen);

    m_filehandle->makeData(name, ::NeXus::CHAR, dimensions, true);
    this->putAttr(attributes, avalues);
    char* strs=new char[values.size()*maxlen];
    for(size_t i=0;i<values.size();i++)
    {
      strncpy(&strs[i*maxlen],values[i].c_str(),maxlen);
    }
    m_filehandle->putData(strs);
    m_filehandle->closeData();
    delete[] strs;
  }

  //
  // Write an NXnote entry with data giving parameter pair values for algorithm history and environment
  // Use NX_CHAR instead of NX_BINARY for the parameter values to make more simple.
  //
  void NexusFileIO::writeNxNote(const std::string& noteName, const std::string& author, const std::string& date,
                                const std::string& description, const std::string& pairValues) const
  {
    m_filehandle->makeGroup(noteName, "NXnote", true);

    m_filehandle->writeData("author", author);
    if(date!="")
    {
      std::vector<std::string> attributes,avalues;
      attributes.push_back("date");
      avalues.push_back(date);
      this->putAttr("author", attributes, avalues);
    }

    m_filehandle->writeData("description", description);
    m_filehandle->writeData("data", pairValues);

    m_filehandle->closeGroup();
  }

  //-------------------------------------------------------------------------------------
  /** Write out a MatrixWorkspace's data as a 2D matrix.
   * Use writeNexusProcessedDataEvent if writing an EventWorkspace.
   */
  int NexusFileIO::writeNexusProcessedData2D( const API::MatrixWorkspace_const_sptr& localworkspace,
      const bool& uniformSpectra, const std::vector<int>& spec,
      const char * group_name, bool write2Ddata) const
  {
    //write data entry
    m_filehandle->makeGroup(group_name,"NXdata", true);

    // write workspace data
    const size_t nHist=localworkspace->getNumberHistograms();
    if(nHist<1)
      return(2);
    const size_t nSpectBins=localworkspace->readY(0).size();
    const size_t nSpect=spec.size();
    std::vector<int64_t> dims_array(2);
    dims_array[0] = static_cast<int64_t>(nSpect);
    dims_array[1] = static_cast<int64_t>(nSpectBins);

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
      for (size_t i=0;i<nSpect;i++)
        axis2.push_back((*sAxis)(spec[i]));
    else
      for (size_t i=0;i<sAxis->length();i++)
        axis2.push_back((*sAxis)(i));

    std::vector<int64_t> start(2, 0);
    std::vector<int64_t> asize(2, 1);
    asize[1] = dims_array[1];

    // -------------- Actually write the 2D data ----------------------------
    if (write2Ddata)
    {
      m_filehandle->makeCompData("values", ::NeXus::FLOAT64, dims_array, m_nexuscompression, asize, true);
      for(size_t i=0;i<nSpect;i++)
      {
        int s = spec[i];
        m_filehandle->putSlab(reinterpret_cast<void*>(const_cast<double*>(&(localworkspace->readY(s)[0]))),start,asize);
        start[0]++;
      }
      if(m_progress != 0) m_progress->reportIncrement(1, "Writing data");
      m_filehandle->putAttr("signal", static_cast<int32_t>(1));

      // More properties
      m_filehandle->putAttr("axes", "axis2,axis1");
      m_filehandle->putAttr("units", localworkspace->YUnit());
      m_filehandle->putAttr("unit_label", localworkspace->YUnitLabel());
      m_filehandle->closeData();

      // error
      m_filehandle->makeCompData("errors", ::NeXus::FLOAT64, dims_array, m_nexuscompression, asize, true);
      start[0]=0;
      for(size_t i=0;i<nSpect;i++)
      {
        int s = spec[i];
        m_filehandle->putSlab(reinterpret_cast<void*>(const_cast<double*>(&(localworkspace->readE(s)[0]))),start,asize);
        start[0]++;
      }
      if(m_progress != 0) m_progress->reportIncrement(1, "Writing data");
      m_filehandle->closeData();

      // Fractional area for RebinnedOutput
      if (localworkspace->id() == "RebinnedOutput")
      {
        RebinnedOutput_const_sptr rebin_workspace = boost::dynamic_pointer_cast<const RebinnedOutput>(localworkspace);
        m_filehandle->makeCompData("frac_area", ::NeXus::FLOAT64, dims_array, m_nexuscompression, asize, true);
        start[0]=0;
        for(size_t i=0;i<nSpect;i++)
        {
          int s = spec[i];
          m_filehandle->putSlab(reinterpret_cast<void*>(const_cast<double*>(&(rebin_workspace->readF(s)[0]))),
                                start, asize);
          start[0]++;
        }
        if(m_progress != 0) m_progress->reportIncrement(1, "Writing data");
        m_filehandle->closeData();
      }

    }

    // write X data, as single array or all values if "ragged"
    if(uniformSpectra)
    {
      m_filehandle->writeData("axis1", localworkspace->readX(0));
      m_filehandle->openData("axis1");
    }
    else
    {
      dims_array[0]=static_cast<int64_t>(nSpect);
      dims_array[1]=static_cast<int64_t>(localworkspace->readX(0).size());
      m_filehandle->makeData("axis1", ::NeXus::FLOAT64, dims_array, true);
      start[0]=0; asize[1]=dims_array[1];
      for(size_t i=0;i<nSpect;i++)
      {
        m_filehandle->putSlab(reinterpret_cast<void*>(const_cast<double*>(&(localworkspace->readX(i)[0]))),start,asize);
        start[0]++;
      }
    }
    std::string dist=(localworkspace->isDistribution()) ? "1" : "0";
    m_filehandle->putAttr("distribution", dist);
    m_filehandle->putAttr("units", xLabel);
    m_filehandle->closeData();

    if ( ! sAxis->isText() )
    {
      // write axis2, maybe just spectra number
      dims_array[0]=static_cast<int64_t>(axis2.size());
      m_filehandle->makeData("axis2", ::NeXus::FLOAT64, dims_array, true);
      m_filehandle->putData((void*)&(axis2[0]));
      m_filehandle->putAttr("units", sLabel);
      m_filehandle->closeData();
    }
    else
    {
      std::string textAxis;
      for ( size_t i = 0; i < sAxis->length(); i ++ )
      {
        std::string label = sAxis->label(i);
        textAxis += label + "\n";
      }
      dims_array[0] = static_cast<int64_t>(textAxis.size());
      m_filehandle->makeData("axis2", ::NeXus::CHAR, dims_array, true);
      m_filehandle->putData(reinterpret_cast<void*>(const_cast<char*>(textAxis.c_str())));
      m_filehandle->putAttr("units", "TextAxis");
      m_filehandle->closeData();
    }

    writeNexusBinMasking(localworkspace);

    m_filehandle->closeGroup();
    return(0);
  }


  //-------------------------------------------------------------------------------------
  /** Write out a table Workspace's 
   */
  int NexusFileIO::writeNexusTableWorkspace( const API::ITableWorkspace_const_sptr& itableworkspace,
      const char * group_name) const
  {

    boost::shared_ptr<const TableWorkspace> tableworkspace =
                boost::dynamic_pointer_cast<const TableWorkspace>(itableworkspace);
    boost::shared_ptr<const PeaksWorkspace> peakworkspace =
                boost::dynamic_pointer_cast<const PeaksWorkspace>(itableworkspace);

    if ( !tableworkspace && !peakworkspace )
      return(0);

    //write data entry
    m_filehandle->makeGroup(group_name,"NXdata", true);

    int nRows = static_cast<int>(itableworkspace->rowCount());

    for (size_t i = 0; i < itableworkspace->columnCount(); i++)
    {
      boost::shared_ptr<const API::Column> col = itableworkspace->getColumn(i);

      std::string dataname = "column_" + boost::lexical_cast<std::string>(i+1);
  
      if ( col->isType<double>() )  
      {  
        std::vector<double> toNexus(nRows);
        for (int ii = 0; ii < nRows; ii++)
          toNexus[ii] = col->cell<double>(ii);
        m_filehandle->writeData(dataname, toNexus);

        // attributes
        m_filehandle->openData(dataname);
        m_filehandle->putAttr("units", "Not known");
        m_filehandle->putAttr("interpret_as", "A double");
      }
      else if ( col->isType<int>() )  
      {  
        std::vector<int> toNexus(nRows);
        for (int ii = 0; ii < nRows; ii++)
          toNexus[ii] = col->cell<int>(ii);
        m_filehandle->writeData(dataname, toNexus);

        // attributes
        m_filehandle->openData(dataname);
        m_filehandle->putAttr("units", "Not known");
        m_filehandle->putAttr("interpret_as", "An integer");
      }
      else if ( col->isType<std::string>() )
      {
        // determine max string size
        size_t maxStr = 0;
        for (int ii = 0; ii < nRows; ii++)
        {
          if ( col->cell<std::string>(ii).size() > maxStr)
            maxStr = col->cell<std::string>(ii).size();
        }
        std::vector<int64_t> dims_array(2);
        dims_array[0] = nRows;
        dims_array[1] = static_cast<int64_t>(maxStr);
        std::vector<int64_t> asize(2,1);
        asize[1] = dims_array[1];

        m_filehandle->makeCompData(dataname, ::NeXus::CHAR, dims_array, m_nexuscompression, asize, true);
        char* toNexus = new char[maxStr*nRows];
        for(int ii = 0; ii < nRows; ii++)
        {
          std::string rowStr = col->cell<std::string>(ii);
          for (size_t ic = 0; ic < rowStr.size(); ic++)
            toNexus[ii*maxStr+ic] = rowStr[ic];
          for (size_t ic = rowStr.size(); ic < static_cast<size_t>(maxStr); ic++)
            toNexus[ii*maxStr+ic] = ' ';
        }
        
        m_filehandle->putData((void *)(toNexus));
        delete[] toNexus;

        // attributes
        m_filehandle->putAttr("units", "N/A");
        m_filehandle->putAttr("interpret_as", "A string");
      }

      // write out title 
      m_filehandle->putAttr("name",  col->name());
      m_filehandle->closeData();
    }

    m_filehandle->closeGroup();
    return(0);
  }




  //-------------------------------------------------------------------------------------
  /** Write out a combined chunk of event data
   *
   * @param ws :: an EventWorkspace
   * @param indices :: array of event list indexes
   * @param tofs :: array of TOFs
   * @param weights :: array of event weights
   * @param errorSquareds :: array of event squared errors
   * @param pulsetimes :: array of pulsetimes
   * @param compress :: if true, compress the entry
   */
  int NexusFileIO::writeNexusProcessedDataEventCombined( const DataObjects::EventWorkspace_const_sptr& ws,
      std::vector<int64_t> & indices,
      double * tofs, float * weights, float * errorSquareds, int64_t * pulsetimes,
      bool compress) const
  {
    //write data entry
    //m_filehandle->makeGroup("event_workspace","NXdata");

    m_filehandle->openGroup("event_workspace","NXdata");

    // The array of indices for each event list #
    std::vector<int64_t> dims_array(1);
    dims_array[0] = indices.size();
    if (indices.size() > 0)
    {
      std::string label("indices");
      if (compress)
        m_filehandle->writeCompData(label, indices, dims_array, m_nexuscompression, dims_array);
      else
        m_filehandle->writeData(label, indices);
      m_filehandle->openData(label);

      m_filehandle->putAttr("units", ws->YUnit());
      m_filehandle->putAttr("unit_label", ws->YUnitLabel());

      m_filehandle->closeData();
    }

    // Write out each field
    dims_array[0] = static_cast<int64_t>(indices.back()); // TODO big truncation error! This is the # of events
    if (tofs)
      writeNXdata("tof", ::NeXus::FLOAT64, dims_array, (void *)(tofs), compress);
    if (pulsetimes)
      writeNXdata("pulsetime", ::NeXus::INT64, dims_array, (void *)(pulsetimes), compress);
    if (weights)
      writeNXdata("weight", ::NeXus::FLOAT32, dims_array, (void *)(weights), compress);
    if (errorSquareds)
      writeNXdata("error_squared", ::NeXus::FLOAT32, dims_array, (void *)(errorSquareds), compress);

    // Close up the overall group
    m_filehandle->closeGroup();
    return(0);
  }

  //-------------------------------------------------------------------------------------
  /** Write out all of the event lists in the given workspace
   * @param ws :: an EventWorkspace */
  int NexusFileIO::writeNexusProcessedDataEvent( const DataObjects::EventWorkspace_const_sptr& ws)
  {
    //write data entry
    m_filehandle->makeGroup("event_workspace","NXdata", true);

    for (size_t wi=0; wi < ws->getNumberHistograms(); wi++)
    {
      std::ostringstream group_name;
      group_name << "event_list_" << wi;
      this->writeEventList( ws->getEventList(wi), group_name.str());
    }

    // Close up the overall group
    m_filehandle->closeGroup();
    return(0);
  }

  //-------------------------------------------------------------------------------------
  template<class TYPE>
  void NexusFileIO::writeNXdata( const std::string& name, std::vector<TYPE>& data, bool compress) const
  {
    if (compress)
    {
      std::vector<int64_t> dims(1, data.size());
      m_filehandle->writeCompData(name, data, dims, m_nexuscompression, dims);
    }
    else
    {
      m_filehandle->writeData(name, data);
    }
  }

  /** Write out an array to the open file. */
  void NexusFileIO::writeNXdata( const std::string& name, ::NeXus::NXnumtype datatype, std::vector<int64_t>& dims_array,
                                 void * data, bool compress) const
  {
    if (compress)
    {
      // We'll use the same slab/buffer size as the size of the array
      m_filehandle->makeCompData(name, datatype, dims_array, m_nexuscompression, dims_array, true);
    }
    else
    {
      // Write uncompressed.
      m_filehandle->makeData(name, datatype, dims_array, true);
    }

    m_filehandle->putData(data);
    m_filehandle->closeData();
  }

  //-------------------------------------------------------------------------------------
  /** Write out the event list data, no matter what the underlying event type is
   * @param events :: vector of TofEvent or WeightedEvent, etc.
   * @param writeTOF :: if true, write the TOF values
   * @param writePulsetime :: if true, write the pulse time values
   * @param writeWeight :: if true, write the event weights
   * @param writeError :: if true, write the errors
   */
  template<class T>
  void NexusFileIO::writeEventListData( std::vector<T> events, bool writeTOF, bool writePulsetime, bool writeWeight, bool writeError) const
  {
    // Do nothing if there are no events.
    if (events.empty())
      return;

    size_t num = events.size();
    std::vector<double> tofs(num);
    std::vector<double> weights(num);
    std::vector<double> errorSquareds(num);
    std::vector<int64_t> pulsetimes(num);

    typename std::vector<T>::const_iterator it;
    typename std::vector<T>::const_iterator it_end = events.end();
    size_t i = 0;

    // Fill the C-arrays with the fields from all the events, as requested.
    for (it = events.begin(); it != it_end; it++)
    {
      if (writeTOF) tofs[i] = it->tof();
      if (writePulsetime) pulsetimes[i] = it->pulseTime().totalNanoseconds();
      if (writeWeight) weights[i] = it->weight();
      if (writeError) errorSquareds[i] = it->errorSquared();
      i++;
    }

    // Write out all the required arrays.
    std::vector<int64_t> dims_array(1,static_cast<int64_t>(num));
    // In this mode, compressing makes things extremely slow! Not to be used for managed event workspaces.
    bool compress = true; //(num > 100);
    if (writeTOF)
      writeNXdata("tof", tofs, compress);
    if (writePulsetime)
      writeNXdata("pulsetime", pulsetimes, compress);
    if (writeWeight)
      writeNXdata("weight", weights, compress);
    if (writeError)
      writeNXdata("error_squared", errorSquareds, compress);
  }


  //-------------------------------------------------------------------------------------
  /** Write out an event list into an already-opened group
   * @param el :: reference to the EventList to write.
   * @param group_name :: group_name to create.
   * */
  int NexusFileIO::writeEventList( const DataObjects::EventList & el, std::string group_name) const
  {
    //write data entry
    m_filehandle->makeGroup(group_name, "NXdata", true);

    // Copy the detector IDs to an array.
    const std::set<detid_t>& dets = el.getDetectorIDs();

    // Write out the detector IDs
    if (!dets.empty())
    {
      const std::vector<detid_t> temp(dets.begin(), dets.end());
      m_filehandle->writeData("detector_IDs", temp);
    }

    // Save an attribute with the type of each event.
    std::string eventType("UNKNOWN");
    size_t num = el.getNumberEvents();
    switch (el.getEventType())
    {
    case TOF:
      eventType = "TOF";
      writeEventListData( el.getEvents(), true, true, false, false );
      break;
    case WEIGHTED:
      eventType = "WEIGHTED";
      writeEventListData( el.getWeightedEvents(), true, true, true, true );
      break;
    case WEIGHTED_NOTIME:
      eventType = "WEIGHTED_NOTIME";
      writeEventListData( el.getWeightedEventsNoTime(), true, false, true, true );
      break;
    }
    m_filehandle->putAttr("event_type", eventType);

    // --- Save the type of sorting -----
    std::string sortType;
    switch (el.getSortType())
    {
    case TOF_SORT:
      sortType = "TOF_SORT";
      break;
    case PULSETIME_SORT:
      sortType = "PULSETIME_SORT";
      break;
    case UNSORTED:
    default:
      sortType = "UNSORTED";
      break;
    }
    m_filehandle->putAttr("sort_type", sortType);

    // Save an attribute with the number of events
    m_filehandle->putAttr("num_events", static_cast<int64_t>(num));

    // Close it up!
    m_filehandle->closeGroup();
    return(0);
  }

  //-------------------------------------------------------------------------------------
  bool NexusFileIO::checkAttributeName(const std::string& target) const
  {
    // see if the given attribute name is in the current level
    // return true if it is.
    const std::vector< ::NeXus::AttrInfo> infos = m_filehandle->getAttrInfos();
    for (auto it = infos.begin(); it != infos.end(); ++it)
    {
      if (target.compare(it->name)==0)
        return true;
    }

    return false;
  }

  int NexusFileIO::findMantidWSEntries() const
  {
    // search exiting file for entries of form mantid_workspace_<n> and return count
    int count=0;
    std::map<std::string, std::string> entries = m_filehandle->getEntries();
    for (auto it = entries.begin(); it != entries.end(); ++it)
    {
      if (it->second == "NXentry")
      {
        if (it->first.find("mantid_workspace_")==0)
          count++;
      }
    }

    return count;
  }

  bool NexusFileIO::checkEntryAtLevel(const std::string& item) const
  {
    // Search the currently open level for name "item"
    std::map<std::string, std::string> entries = m_filehandle->getEntries();
    for (auto it = entries.begin(); it != entries.end(); ++it)
    {
        if (it->first== item)
          return true;
    }

    return(false);
  }


  bool NexusFileIO::checkEntryAtLevelByAttribute(const std::string& attribute, std::string& entry) const
  {
    // Search the currently open level for a section with "attribute" and return entry name
    std::map<std::string, std::string> entries = m_filehandle->getEntries();
    for (auto it = entries.begin(); it != entries.end(); ++it)
    {
      if (it->second == "SDS")
      {
        m_filehandle->openData(it->first);
        bool result = checkAttributeName(attribute);
        m_filehandle->closeData();
        if (result)
        {
          entry = it->first;
          return true;
        }
      }
    }

    return(false);

  }


  /**
   * Write bin masking information
   * @param ws :: The workspace
   * @return true for OK, false for error
   */
  bool NexusFileIO::writeNexusBinMasking(API::MatrixWorkspace_const_sptr ws) const
  {
    std::vector< int > spectra;
    std::vector< int32_t > bins;
    std::vector< double > weights;
    int spectra_count = 0;
    int offset = 0;
    for(std::size_t i=0;i<ws->getNumberHistograms(); ++i)
    {
      if (ws->hasMaskedBins(i))
      {
        const API::MatrixWorkspace::MaskList& mList = ws->maskedBins(i);
        spectra.push_back(spectra_count);
        spectra.push_back(offset);
        API::MatrixWorkspace::MaskList::const_iterator it = mList.begin();
        for(;it != mList.end(); ++it)
        {
          bins.push_back(static_cast<int32_t>(it->first));
          weights.push_back(it->second);
        }
        ++spectra_count;
        offset += static_cast<int>(mList.size());
      }
    }

    if (spectra_count == 0) return false;

    // save spectra offsets as a 2d array of ints
    std::vector<int64_t> dimensions(2);
    dimensions[0]=spectra_count;
    dimensions[1]=2;
    m_filehandle->makeData("masked_spectra", ::NeXus::INT32, dimensions, true);
    m_filehandle->putAttr("description", "spectra index,offset in masked_bins and mask_weights");
    m_filehandle->putData((void*)&spectra[0]);
    m_filehandle->closeData();

    // save masked bin indices
    m_filehandle->writeData("masked_bins", bins);

    // save masked bin weights
    m_filehandle->writeData("mask_weights", weights);

    return true;
  }


  template<>
  std::string NexusFileIO::logValueType<double>()const{return "double";}

  template<>
  std::string NexusFileIO::logValueType<int>()const{return "int";}

  template<>
  std::string NexusFileIO::logValueType<bool>()const{return "bool";}


  /** Get all the Nexus entry types for a file
   *
   * Try to open named Nexus file and return all entries plus the definition found for each.
   * If definition not found, try and return "analysis" field (Muon V1 files)
   * Closes file on exit.
   *
   * @param fileName :: file to open
   * @param entryName :: vector that gets filled with strings with entry names
   * @param definition :: vector that gets filled with the "definition" or "analysis" string.
   * @return count of entries if OK, -1 failed to open file.
   */
  int getNexusEntryTypes(const std::string& fileName, std::vector<std::string>& entryName,
      std::vector<std::string>& definition )
  {
    ::NeXus::File *handle = new ::NeXus::File(fileName);
    int result = getNexusEntryTypes(handle, entryName, definition);
    delete handle;
    return result;
  }

  /** Get all the Nexus entry types for a file
   *
   * Try to open named Nexus file and return all entries plus the definition found for each.
   * If definition not found, try and return "analysis" field (Muon V1 files)
   * Closes file on exit.
   *
   * @param handle :: file handle to use
   * @param entryName :: vector that gets filled with strings with entry names
   * @param definition :: vector that gets filled with the "definition" or "analysis" string.
   * @return count of entries if OK, -1 failed to open file.
   */
  int getNexusEntryTypes(::NeXus::File * handle, std::vector<std::string>& entryName,
                         std::vector<std::string>& definition )
  {
    entryName.clear();
    definition.clear();

    // Loop through all entries looking for the definition section in each (or analysis for MuonV1)
    std::map<std::string, std::string> entries = handle->getEntries();
    std::vector<std::string> entryList;
    const std::string NXENTRY("NXentry");
    for (auto entry = entries.begin(); entry != entries.end(); ++entry)
    {
      if (entry->second == NXENTRY)
        entryList.push_back(entry->first);
    }

    // for each entry found, look for "analysis" or "definition" text data fields and return value plus entry name
    const std::string SDS("SDS");
    const std::string DFN("definition");
    const std::string ANALYSIS("analysis");
    for(size_t i=0;i<entryList.size();i++)
    {
      handle->openGroup(entryList[i], NXENTRY);

      std::map<std::string, std::string> possibilities = handle->getEntries();
      for (auto it = possibilities.begin(); it != possibilities.end(); ++it)
      {
        if (it->second == SDS)
        {
          if (it->first == ANALYSIS || it->first == DFN)
          {
            handle->openData(it->first);
            std::string value = handle->getStrData();
            handle->closeData();

            // return e.g entryName "analysis"/definition "muonTD"
            definition.push_back(value);
            entryName.push_back(entryList[i]);

            break;
          }
        }
      }

      handle->closeGroup();
    }

    return(static_cast<int>(entryName.size()));
  }

} // namespace NeXus
} // namespace Mantid
