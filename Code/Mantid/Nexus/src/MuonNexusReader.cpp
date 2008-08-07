#include <vector>
#include <sstream>
#include <napi.h>
#include "MantidNexus/MuonNexusReader.h"



MuonNexusReader::MuonNexusReader() : counts(0)
{
}

MuonNexusReader::~MuonNexusReader()
{
   delete[] corrected_times;
   delete[] counts;
}

// Basic NeXus Muon file reader - simple version based on contents of test files.
// Read the given Nexus file into temp storage. Following the approach of ISISRAW
// which does not use namespace.
//
// Expected content of Nexus file is:
//     Group: "run"
//       Group: "histogram_data_1"
//         Data: "counts"  (2D integer array"
//         Data: "corrected time" (1D float array)
//
// @param filename  name of existing NeXus Muon file to read
int MuonNexusReader::readFromFile(const std::string& filename)
{
   NXhandle fileID;
   NXaccess mode= NXACC_READ;
   NXstatus stat=NXopen(filename.c_str(), mode, &fileID);
   if(stat==NX_ERROR) return(1);
   //
   stat=NXopengroup(fileID,"run","NXentry");
   if(stat==NX_ERROR) return(1);
   // read histogram data
   stat=NXopengroup(fileID,"histogram_data_1","NXdata");
   if(stat==NX_ERROR) return(1);
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
   // temp check on data vaues
   //int i,j,sum=0;
   //for (i=0;i<dims[0]*dims[1];i++)
   //  sum+=counts[i];
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
   // close file
   if(stat==NX_ERROR) return(1);
   stat=NXclosegroup (fileID);
   if(stat==NX_ERROR) return(1);
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
int MuonNexusReader::getTimeChannels(float* timebnds, int nbnds)
{
   int i;
   // assume constant time bin width given by difference of first two values
   float binHalfWidth=(corrected_times[1]-corrected_times[0])/2.0;
   for(i=0;i<nbnds-1;i++)
      timebnds[i]=corrected_times[i]-binHalfWidth;
   timebnds[nbnds-1]=timebnds[nbnds-2]+2.0*binHalfWidth;
   return(0);
}










