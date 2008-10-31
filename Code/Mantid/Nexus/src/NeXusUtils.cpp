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

namespace Mantid
{
namespace NeXus
{
  //
  // Some utility routines, now made into a class
  //
  using namespace Kernel;
  using namespace API;

  Logger& NeXusUtils::g_log = Logger::get("NeXusUtils");

  /// Empty default constructor
  NeXusUtils::NeXusUtils()
  {
  }

  void NeXusUtils::write_data(NXhandle h, const char* name, const std::vector<double>& v)
  {
    int dims_array[1] = { v.size() };
    NXmakedata(h, name, NX_FLOAT64, 1, dims_array);
    NXopendata(h, name);
    NXputdata(h, (void*)&(v[0]));
    NXclosedata(h);
  }

  int NeXusUtils::writeEntry1D(const std::string& filename, const std::string& entryName, const std::string& dataName,
	                           const std::vector<double>& x, const std::vector<double>& y, const std::vector<double>& e)
  {
    //
    // Simple write of 1D data, no longer supported, all workspaces will be 2D
    //
    NXhandle h;
    NXaccess mode;
    int status;
    boost::filesystem::path file(filename);
    if(boost::filesystem::exists(file))
        mode = NXACC_RDWR;
    else
        mode = NXACC_CREATE5;
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

  int NeXusUtils::getNexusDataValue(const std::string& fileName, const std::string& dataName, std::string& value )
  {
   //
   // Try to open named Nexus file and search in the first entry for the data section "dataName".
   // If found, return the string value found there.
   // return 0 for OK, -1 failed to open file
   //NXhandle fileID;
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

} // namespace NeXus
} // namespace Mantid
