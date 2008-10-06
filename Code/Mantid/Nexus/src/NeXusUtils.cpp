#include <vector>
#include <sstream>
#include <napi.h>
#include <stdlib.h>
#ifdef _WIN32
#include <io.h>
#ifndef F_OK
#define F_OK 0    /* MinGW has this defined, Visual Studio doesn't */
#endif
#endif /* _WIN32 */
#include "MantidNexus/NeXusUtils.h"
#include <boost/shared_ptr.hpp>
#include <boost/filesystem/operations.hpp>


static void write_data(NXhandle h, const char* name, const std::vector<double>& v)
{
    int dims_array[1] = { v.size() };
    NXmakedata(h, name, NX_FLOAT64, 1, dims_array);
    NXopendata(h, name);
    NXputdata(h, (void*)&(v[0]));
    NXclosedata(h);
}

int writeEntry1D(const std::string& filename, const std::string& entryName, const std::string& dataName, const std::vector<double>& x, const std::vector<double>& y, const std::vector<double>& e)
{
    NXhandle h;
    NXaccess mode;
    int status;
    //if (access(filename.c_str(), F_OK) == 0)
    boost::filesystem::path file(filename);
    if(boost::filesystem::exists(file))
    {
        mode = NXACC_RDWR;
    }
    else
    {
        mode = NXACC_CREATE5;
    }
    status=NXopen(filename.c_str(), mode, &h);
    if(status==NX_ERROR)
        return(1);
    if (mode==NXACC_CREATE5)
    {
       status=NXmakegroup(h, entryName.c_str(), "NXentry");
       if(status==NX_ERROR)
          return(2);
       status=NXopengroup(h, entryName.c_str(), "NXentry");
    }
    else
    {
        status=NXopengroup(h,entryName.c_str(),"NXentry");
        if(status==NX_ERROR)
        {
           status=NXmakegroup(h, entryName.c_str(), "NXentry");
           if(status==NX_ERROR)
              return(2);
           status=NXopengroup(h, entryName.c_str(), "NXentry");
        }
    }
    status=NXmakegroup(h, dataName.c_str(), "NXdata");
    status=NXopengroup(h, dataName.c_str(), "NXdata");
    write_data(h, "x", x);
    write_data(h, "y", y);
    write_data(h, "e", e);
    status=NXclosegroup(h);
    status=NXclosegroup(h);
    status=NXclose(&h);
    if(status==NX_ERROR)
        return(3);
    return(0);
}

int getNexusDataValue(const std::string& fileName, const std::string& dataName, std::string& value )
{
   //
   // Try to open named Nexus file and search in the first entry for the data section "dataName".
   // If found, return the string value found there.
   // return 0 for OK, -1 failed to open file
   NXhandle fileID;
   NXaccess mode= NXACC_READ;
   NXstatus stat=NXopen(fileName.c_str(), mode, &fileID);
   value="";
   if(stat==NX_ERROR) return(-1);
   char *nxname,*nxclass;
   int nxdatatype;
   nxname= new char[NX_MAXNAMELEN];
   nxclass = new char[NX_MAXNAMELEN];
   stat=NXgetnextentry(fileID,nxname,nxclass,&nxdatatype);
   // assume this is the requied NXentry
   stat=NXopengroup(fileID,nxname,nxclass);
   std::string tmp1=nxclass,tmp2=nxname; // test
   delete[] nxname;
   delete[] nxclass;
   if(stat==NX_ERROR) return(-2);
   //
   // Try and open named data and read the string value associated with it
   //
   int rank,dims[4],type;
   stat=NXopendata(fileID,dataName.c_str());
   if(stat==NX_ERROR) return(2);
   stat=NXgetinfo(fileID,&rank,dims,&type);
   if(stat==NX_ERROR || type!=NX_CHAR) return(2);
   char* cValue=new char[dims[0]+1];
   stat=NXgetdata(fileID,cValue);
   if(stat==NX_ERROR)
   {
       delete[] cValue;
       return(2);
   }
   cValue[dims[0]]='\0'; // null terminate
   value=cValue;
   delete[] cValue;
   stat=NXclosedata(fileID);
   return(0);
}

int writeNexusTextField( const NXhandle& h, const std::string& name, const std::string& value)
{
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
int writeNexusProcessedHeader( const std::string& fileName, const std::string& entryName, const std::string& title) //, const boost::shared_ptr<Sample> sample)
{
   NXhandle fileID;
   NXaccess mode;
   NXstatus status;
   boost::filesystem::path file(fileName);
   if(boost::filesystem::exists(file))
       mode = NXACC_RDWR;
   else
       mode = NXACC_CREATE5;

   status=NXopen(fileName.c_str(), mode, &fileID);
   if(status==NX_ERROR)
       return(1);
   status=NXmakegroup(fileID,entryName.c_str(),"NXprocessed");
   if(status==NX_ERROR)
       return(2);
   status=NXopengroup(fileID,entryName.c_str(),"NXprocessed");
   //
   std::vector<std::string> attributes,avalues;
   if( ! writeNxText(fileID, "title", title, attributes, avalues) )
       return(3);
   //
   attributes.push_back("URL");
   avalues.push_back("http://www.nexusformat.org/instruments/xml/NXprocessed.xml");
   attributes.push_back("Version");
   avalues.push_back("1.0");
   // this may not be the "correct" long term path, but it is valid at present 
   if( ! writeNxText(fileID, "definition", "NXprocessed", attributes, avalues) )
       return(3);
   //
   status=NXclosegroup(fileID);
   status=NXclose(&fileID);
   return((status==NX_ERROR)?3:0);
}
//
// write an NXdata entry with char values
//
bool writeNxText(NXhandle fileID, std::string name, std::string value, std::vector<std::string> attributes, std::vector<std::string> avalues)
{
   NXstatus status;
   int dimensions[1];
   dimensions[0]=value.size()+1;
   status=NXmakedata(fileID, name.c_str(), NX_CHAR, 1, dimensions);
   if(status==NX_ERROR) return(false);
   status=NXopendata(fileID, name.c_str());
   for(int it=0; it<attributes.size(); ++it)
       status=NXputattr(fileID, attributes[it].c_str(), (void*)avalues[it].c_str(), avalues[it].size(), NX_CHAR);
   status=NXputdata(fileID, (void*)value.c_str());
   status=NXclosedata(fileID);
   return(true);
}
//
// write example from nexus website, adapted to write a data section within NXprocessed entry
//
//int writeNxProcessedData(const NXhandle& fileID)
//{
///* Open output file and output global attributes */
//   NXstatus status;
///* Open top-level NXentry group */
//   NXmakegroup (fileID, "Entry1", "NXentry");
//   NXopengroup (fileID, "Entry1", "NXentry");
///* Open NXdata group within NXentry group */
//        NXmakegroup (fileID, "Data1", "NXdata");
//        NXopengroup (fileID, "Data1", "NXdata");
///* Output time channels */
//          NXmakedata (fileID, "time_of_flight", NX_FLOAT32, 1, &n_t);
//          NXopendata (fileID, "time_of_flight");
//            NXputdata (fileID, t);
//            NXputattr (fileID, "units", "microseconds", 12, NX_CHAR);
//          NXclosedata (fileID);
///* Output detector angles */
//          NXmakedata (fileID, "polar_angle", NX_FLOAT32, 1, &n_p);
//          NXopendata (fileID, "polar_angle");
//            NXputdata (fileID, phi);
//            NXputattr (fileID, "units", "degrees", 7, NX_CHAR);
//          NXclosedata (fileID);
///* Output data */
//          dims[0] = n_t;
//          dims[1] = n_p;
//          NXmakedata (fileID, "counts", NX_INT32, 2, dims);
//          NXopendata (fileID, "counts");
//            NXputdata (fileID, counts);
//            i = 1;
//            NXputattr (fileID, "signal", &i, 1, NX_INT32);
//            NXputattr (fileID, "axes",  "polar_angle:time_of_flight", 26, NX_CHAR);
//          NXclosedata (fileID);
///* Close NXentry and NXdata groups and close file */
//        NXclosegroup (fileID);
//      NXclosegroup (fileID);
//    return;
//
//}
int writeNexusProcessedSample( const std::string& fileName, const std::string& entryName, const std::string& name,
							  const boost::shared_ptr<Mantid::API::Sample> sample)
{
   // reopen file and entry which should already exist - this could be avoided by not closing it.
   NXhandle fileID;
   NXaccess mode;
   NXstatus status;
   if (access(fileName.c_str(), F_OK) == 0)
   {
        mode = NXACC_RDWR;
   }
   else
   {
        return(1);
   }
   status=NXopen(fileName.c_str(), mode, &fileID);
   if(status==NX_ERROR)
       return(1);
   status=NXopengroup(fileID,entryName.c_str(),"NXprocessed");
   if(status==NX_ERROR)
       return(2);
   //write sample entry
   status=NXmakegroup(fileID,"sample","NXsample");
   if(status==NX_ERROR)
       return(2);
   status=NXopengroup(fileID,"sample","NXsample");
   //
   std::vector<std::string> attributes,avalues;
   if( ! writeNxText(fileID, "name", name, attributes, avalues) )
       return(3);
   status=NXclosegroup(fileID);
   status=NXclose(&fileID);
   return((status==NX_ERROR)?3:0);
}
int writeNexusProcessedData( const std::string& fileName, const std::string& entryName,
							const boost::shared_ptr<Mantid::DataObjects::Workspace2D> localworkspace,
							const bool uniformSpectra)
{
   // reopen file and entry - both should already exist - this could be avoided by not closing it.
   NXhandle fileID;
   NXaccess mode;
   NXstatus status;
   if (access(fileName.c_str(), F_OK) == 0)
   {
        mode = NXACC_RDWR;
   }
   else
   {
        return(1);
   }
   status=NXopen(fileName.c_str(), mode, &fileID);
   if(status==NX_ERROR)
       return(1);
   status=NXopengroup(fileID,entryName.c_str(),"NXprocessed");
   if(status==NX_ERROR)
       return(2);
   //write data entry
   status=NXmakegroup(fileID,"workspace","NXdata");
   if(status==NX_ERROR)
       return(2);
   status=NXopengroup(fileID,"workspace","NXdata");
   // write workspace data
   const int nHist=localworkspace->getNumberHistograms();
   if(nHist<1)
	   return(2);
   const int nSpectPoints=localworkspace->dataY(0).size();
   const int nSpectBins=localworkspace->dataX(0).size();
   int dims_array[2] = { nHist,nSpectPoints };
   std::string name=localworkspace->getTitle();
   if(name.size()==0)
	   name="values";
   status=NXmakedata(fileID, name.c_str(), NX_FLOAT64, 2, dims_array);
   status=NXopendata(fileID, name.c_str());
   int start[2]={0,0},asize[2]={1,dims_array[1]};
   for(int i=0;i<nHist;i++)
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
   if(yAxis->isSpectra())
	   yLabel="spectraNumber";
   else
	   yLabel=yAxis->unit()->unitID();
   const std::string axesNames=xLabel+":"+yLabel;
   status=NXputattr (fileID, "axes", (void*)axesNames.c_str(), axesNames.size(), NX_CHAR);
   status=NXclosedata(fileID);
   // error
   name="errors";
   status=NXmakedata(fileID, name.c_str(), NX_FLOAT64, 2, dims_array);
   status=NXopendata(fileID, name.c_str());
   start[0]=0;
   for(int i=0;i<nHist;i++)
   {
      status=NXputslab(fileID, (void*)&(localworkspace->dataE(i)[0]),start,asize);
	  start[0]++;
   }
   status=NXclosedata(fileID);
   //
//   std::vector<std::string> attributes,avalues;
//   if( ! writeNxText(fileID, "name", name, attributes, avalues) )
//       return(3);
   status=NXclosegroup(fileID);
   status=NXclose(&fileID);
   return((status==NX_ERROR)?3:0);
}
