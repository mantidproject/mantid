#include <vector>
#include <sstream>
#include <napi.h>
#include "MantidNexus/MuonNexusReader.h"
#include <boost/scoped_array.hpp>


/// Default constructor
MuonNexusReader::MuonNexusReader() : nexus_instrument_name(), corrected_times(0), counts(0)
{
}

/// Destructor deletes temp storage
MuonNexusReader::~MuonNexusReader()
{
  delete[] corrected_times;
  delete[] counts;
}

// Basic NeXus Muon file reader - simple version based on contents of test files.
// Read the given Nexus file into temp storage. Following the approach of ISISRAW
// which does not use namespace.
// This reader is only used by LoadMuonNexus - the NexusProcessed files are dealt with by
// NexusFileIO.cpp
//
// Expected content of Nexus file is:
//     Entry: "run" (first entry opened, whatever name is)
//       Group: "histogram_data_1" (first NXdata section read, whatever name is)
//         Data: "counts"  (2D integer array)
//         Data: "corrected time" (1D float array)
//
// @param filename ::  name of existing NeXus Muon file to read
int MuonNexusReader::readFromFile(const std::string& filename)
{
  NXhandle fileID;
  NXaccess mode= NXACC_READ;
  NXstatus stat=NXopen(filename.c_str(), mode, &fileID);
  if(stat==NX_ERROR) return(1);
  int nxdatatype;
  boost::scoped_array<char> nxname(new char[NX_MAXNAMELEN]);
  boost::scoped_array<char> nxclass(new char[NX_MAXNAMELEN]);
  stat=NXgetnextentry(fileID,nxname.get(),nxclass.get(),&nxdatatype);
  if(stat==NX_ERROR) return(1);
  stat=NXopengroup(fileID,nxname.get(),nxclass.get());
  if(stat==NX_ERROR) return(1);
  //
  std::vector<std::string> nxnamelist,nxclasslist;
  nxnamelist.push_back(nxname.get());
  nxclasslist.push_back(nxclass.get());
  std::vector<std::string> nxdataname;
  //
  // read nexus fields at this level
  while( (stat=NXgetnextentry(fileID,nxname.get(),nxclass.get(),&nxdatatype)) == NX_OK )
  {
    nxnamelist.push_back(nxname.get());
    nxclasslist.push_back(nxclass.get());
    // record NXdata section name(s)
    if(nxclasslist.back()=="NXdata")
    {
      nxdataname.push_back(nxnamelist.back());
    }
  }
  //
  if(stat==NX_ERROR) return(1);
  // open NXdata section
  stat=NXopengroup(fileID,nxdataname.front().c_str(),"NXdata");
  if(stat==NX_ERROR) return(1);
  //
  stat=NXopendata(fileID,"counts");
  int rank,type,dims[4];
  stat=NXgetinfo(fileID,&rank,dims,&type);
  // Number of time channels and number of spectra made public
  t_ntc1=dims[1];
  t_nsp1=dims[0];
  //
  if(stat==NX_ERROR) return(1);
  // allocate temp space for histogram data
  counts = new int[dims[0]*dims[1]];
  stat=NXgetdata(fileID,counts);
  if(stat==NX_ERROR) return(1);
  //
  stat=NXclosedata(fileID);
  if(stat==NX_ERROR) return(1);
  //Get groupings
  stat=NXopendata(fileID,"grouping");
  stat=NXgetinfo(fileID,&rank,dims,&type);
  if(stat==NX_ERROR) return(1);
  // allocate temp space for grouping data
  numDetectors = dims[0];
  detectorGroupings = new int[dims[0]]; // This doesn't seem to be used?
  stat=NXgetdata(fileID,detectorGroupings);
  delete[] detectorGroupings;
  if(stat==NX_ERROR) return(1);
  //
  stat=NXclosedata(fileID);
  if(stat==NX_ERROR) return(1);

  // read corrected time
  stat=NXopendata(fileID,"corrected_time");
  if(stat==NX_ERROR) return(1);
  stat=NXgetinfo(fileID,&rank,dims,&type);
  if(stat==NX_ERROR) return(1);
  corrected_times = new float[dims[0]];
  if(stat==NX_ERROR) return(1);
  stat=NXgetdata (fileID, corrected_times);
  if(stat==NX_ERROR) return(1);
  // assume only one data set in file
  t_nper=1;
  //
  stat=NXclosedata (fileID);
  stat=NXclosegroup (fileID);
  if(stat==NX_ERROR) return(1);
  //
  // get instrument name
  stat=NXopengroup(fileID,"instrument","NXinstrument");
  if(stat==NX_ERROR) return(1);
  stat=NXopendata(fileID,"name");
  if(stat==NX_ERROR) return(1);
  stat=NXgetinfo(fileID,&rank,dims,&type);
  char* instrument=new char[dims[0]+1];
  stat=NXgetdata(fileID,instrument);
  instrument[dims[0]]='\0'; // null terminate for copy
  nexus_instrument_name=instrument;
  delete[] instrument;
  stat=NXclosedata(fileID);
  if (stat==NX_ERROR) return(1);
  stat=NXclosegroup (fileID);
  //
  // Get number of switching states if available. Take this as number of periods
  // If not available set as one period.
  stat=NXopendata(fileID,"switching_states");
  if(stat!=NX_ERROR)
  {
    //stat=NXgetinfo(fileID,&rank,dims,&type);
    int ssPeriods;
    stat=NXgetdata(fileID,&ssPeriods);
    t_nper=ssPeriods;
    t_nsp1/=t_nper; // assume that number of spectra in multiperiod file should be divided by periods
  }
  else
    t_nper=1;
  //
  // close file
  stat=NXclosegroup (fileID);
  if(stat==NX_ERROR) return(1);
  stat=NXclose (&fileID);

  return(0);
}

// Get time boundary data as in ISISRAW. Simpler here as NeXus stores real times
// Not clear if corrected_time is what is wanted. Assume that values are bin centre
// times and that bin boundary values are wanted, as ISISRAW.
// @param  timebnds  float pointer for time values to be stored
// @param  ndnbs     int count of expected points
int MuonNexusReader::getTimeChannels(float* timebnds, const int& nbnds) const
{
  int i;
  // assume constant time bin width given by difference of first two values
  float binHalfWidth=(corrected_times[1]-corrected_times[0])/float(2.0);
  for(i=0;i<nbnds-1;i++)
    timebnds[i]=corrected_times[i]-binHalfWidth;
  timebnds[nbnds-1]=timebnds[nbnds-2]+float(2.0)*binHalfWidth;
  return(0);
}

std::string MuonNexusReader::getInstrumentName()
{
  return(nexus_instrument_name);
}

// NeXus Muon file reader for NXlog data.
// Read the given Nexus file into temp storage.
//
// Expected content of Nexus file is:
//     NXentry: "run" (or any name, ignored at present)
//            Zero or more NXlog entries which are of the form: <time>,<value>
//            <time> is 32bit float time wrt start_time and <value> either 32bit float
//            or sting.
//
// @param filename ::  name of existing NeXus Muon file to read
int MuonNexusReader::readLogData(const std::string& filename)
{
  NXhandle fileID;
  NXaccess mode= NXACC_READ;
  NXstatus stat=NXopen(filename.c_str(), mode, &fileID);
  if(stat==NX_ERROR) return(1);
  // find name and class of first entry
  int nxdatatype;
  boost::scoped_array<char> nxname(new char[NX_MAXNAMELEN]);
  boost::scoped_array<char> nxclass(new char[NX_MAXNAMELEN]);
  stat=NXgetnextentry(fileID,nxname.get(),nxclass.get(),&nxdatatype);
  if(stat==NX_ERROR) return(1);
  // assume this is the requied NXentry
  stat=NXopengroup(fileID,nxname.get(),nxclass.get());
  if(stat==NX_ERROR) return(1);
  //
  // read nexus fields at this level looking for NXlog and loading these into
  // memory
  // Also get the start_time string needed to change these times into ISO times
  std::vector<std::string> nxnamelist,nxclasslist;
  int count=0;
  int rank,dims[4],type;
  char start_time[]="start_time";
  nexusLogCount=0;
  int nexusSampleCount=0; //debug
  while( (stat=NXgetnextentry(fileID,nxname.get(),nxclass.get(),&nxdatatype)) == NX_OK )
  {
    nxnamelist.push_back(nxname.get());
    nxclasslist.push_back(nxclass.get());
    if(nxclasslist[count]=="NXlog")
    {
      stat=NXopengroup(fileID,nxname.get(),nxclass.get());
      if(stat==NX_ERROR) return(1);
      if(readMuonLogData(fileID)==0)
      {
        nexusLogCount++;
      }
      stat=NXclosegroup(fileID);
      if(stat==NX_ERROR) return(1);
    }
    if(nxclasslist[count]=="NXSample" || nxclasslist[count]=="NXsample") // NXSample should be NXsample
    {
      nexusSampleCount++; //debug
      stat=NXopengroup(fileID,nxnamelist[count].c_str(),nxclasslist[count].c_str());
      if(stat==NX_ERROR) return(1);
      stat=NXopendata(fileID,"name");
      if(stat==NX_ERROR) return(1);
      stat=NXgetinfo(fileID,&rank,dims,&type);
      char* sampleName=new char[dims[0]+1];
      stat=NXgetdata(fileID,sampleName);
      sampleName[dims[0]]='\0'; // null terminate for copy
      nexus_samplename=sampleName;
      delete[] sampleName;
      stat=NXclosedata(fileID);
      if(stat==NX_ERROR) return(1);
      stat=NXclosegroup (fileID);
    }
    if(nxnamelist[count]==start_time)
    {

      stat=NXopendata(fileID,start_time); if(stat==NX_ERROR) return(1);
      stat=NXgetinfo(fileID,&rank,dims,&type); if(stat==NX_ERROR) return(1);
      char* sTime=new char[dims[0]+1];
      stat=NXgetdata(fileID,sTime); if(stat==NX_ERROR) return(1);
      sTime[dims[0]]='\0'; // null terminate
      stat=NXclosedata(fileID); if(stat==NX_ERROR) return(1);
      startTime=sTime;
      delete[] sTime;
      if( (startTime.find('T')) >0 )
        startTime.replace(startTime.find('T'),1," ");
      boost::posix_time::ptime pt=boost::posix_time::time_from_string(startTime);
      startTime_time_t=to_time_t(pt);
    }
    count++;
  }

  NXclose(&fileID);
  if(stat==NX_ERROR) return(1);
  return(0);
}


int MuonNexusReader::readMuonLogData(NXhandle fileID)
{
  // read the name/values/times data of the currently opened NXlog section of a Nexus Muon file
  // given by fileID
  // The values are stored so they can be saved into the workspace.
  int stat,rank,dims[4],type;
  char name[]="name",values[]="values",time[]="time"; // section names used in example files
  // read name of Log data
  stat=NXopendata(fileID,name); if(stat==NX_ERROR) return(1);
  stat=NXgetinfo(fileID,&rank,dims,&type); if(stat==NX_ERROR) return(1);
  boost::scoped_array<char> dataName(new char[dims[0]+1]);
  stat=NXgetdata(fileID,dataName.get());
  if(stat==NX_ERROR) return(1);
  dataName[dims[0]]='\0'; // null terminate
  stat=NXclosedata(fileID); if(stat==NX_ERROR) return(1);
  logNames.push_back(dataName.get());
  //
  // read data values
  stat=NXopendata(fileID,values); if(stat==NX_ERROR) return(1);
  stat=NXgetinfo(fileID,&rank,dims,&type); if(stat==NX_ERROR) return(1);
  boost::scoped_array<float> dataVals(new float[dims[0]]);
  if(type==NX_FLOAT32 && rank==1)
  {
    stat=NXgetdata(fileID,dataVals.get());
    logType.push_back(true);
  }
  else
  {
    logType.push_back(false);
    return(1);
  }
  std::vector<float> tmpf(dataVals.get(),dataVals.get()+dims[0]);
  logValues.push_back(tmpf);
  stat=NXclosedata(fileID); if(stat==NX_ERROR) return(1);
  //
  // read time values
  stat=NXopendata(fileID,time); if(stat==NX_ERROR) return(1);
  stat=NXgetinfo(fileID,&rank,dims,&type); if(stat==NX_ERROR) return(1);
  boost::scoped_array<float> timeVals(new float[dims[0]]);
  if(type==NX_FLOAT32 && rank==1)
  {
    stat=NXgetdata(fileID,timeVals.get());
  }
  else
    return(1);
  std::vector<float> tmp(timeVals.get(),timeVals.get()+dims[0]);
  logTimes.push_back(tmp);
  stat=NXclosedata(fileID); if(stat==NX_ERROR) return(1);
  return(0);
}
void MuonNexusReader::getLogValues(const int& logNumber, const int& logSequence,
    std::time_t& logTime, double& value)
{
  // for the given log find the logTime and value at given sequence in log
  double time=logTimes[logNumber][logSequence];
  //boost::posix_time::ptime pt=boost::posix_time::time_from_string(startTime);
  //std::time_t atime=to_time_t(pt);
  //atime+=time;
  logTime=static_cast<std::time_t>(time)+startTime_time_t;
  //DateAndTime="2008-08-12T09:00:01"; //test
  value=logValues[logNumber][logSequence];
}
int MuonNexusReader::numberOfLogs() const
{
  return(nexusLogCount);
}

int MuonNexusReader::getLogLength(const int i) const
{
  return(logTimes[i].size());
}

bool MuonNexusReader::logTypeNumeric(const int i) const
{
  return(logType[i]);
}

/** return log name of i'th NXlog section
 * @param i :: the number of the NXlog section find name of.
 * @return the log name at the given index
 */
std::string MuonNexusReader::getLogName(const int i) const
{
  return(logNames[i]);
}



