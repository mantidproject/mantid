//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/SaveISISNeXus.h"
#include "MantidAPI/FileProperty.h"
#include "LoadRaw/isisraw2.h"

#include <boost/scoped_array.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/lexical_cast.hpp>

#include <Poco/File.h>
#include <Poco/Path.h>
#include <Poco/DirectoryIterator.h>
#include <Poco/DateTimeParser.h>
#include <Poco/DateTimeFormat.h>
#include <Poco/RegularExpression.h>

#include <sstream>
#include <algorithm>
#include <map>
#include <iterator>
#include <fstream>

namespace Mantid
{
namespace DataHandling
{

// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(SaveISISNexus)

/// Sets documentation strings for this algorithm
void SaveISISNexus::initDocs()
{
  this->setWikiSummary("The SaveISISNexus algorithm will convert a RAW file to a Nexus file.");
  this->setOptionalMessage("The SaveISISNexus algorithm will convert a RAW file to a Nexus file.");
}

using namespace Kernel;
using namespace API;

/// Empty default constructor
SaveISISNexus::SaveISISNexus() : Algorithm() {}

/** Initialisation method.
 *
 */
void SaveISISNexus::init()
{
  std::vector<std::string> raw_exts;
  raw_exts.push_back(".raw");
  raw_exts.push_back(".s*");
  raw_exts.push_back(".add");
  declareProperty(new FileProperty("InputFilename", "", FileProperty::Load, raw_exts),
    "The name of the RAW file to read, including its full or relative\n"
    "path. (N.B. case sensitive if running on Linux).");

  // Declare required parameters, filename with ext {.nx,.nx5,xml} and input workspac
  std::vector<std::string> nxs_exts;
  nxs_exts.push_back(".nxs");
  nxs_exts.push_back(".nx5");
  nxs_exts.push_back(".xml");
  declareProperty(new FileProperty("OutputFilename", "", FileProperty::Save, nxs_exts),
		    "The name of the Nexus file to write, as a full or relative\n"
		    "path");
  //declareProperty(new WorkspaceProperty<MatrixWorkspace> ("InputWorkspace", "", Direction::Input),
  //    "Name of the workspace to be saved");
}

/**
  * To be used with std::generate to copy only those values from a dataset that don't relate to monitors
  */
template<typename T>
class getWithoutMonitors
{
public:
  /**
    * Constructor.
    * @param alg This algorithm.
    * @param data Pointer to the data to be copied from
    */
  getWithoutMonitors(SaveISISNexus* alg,T* data): m_alg(alg),m_data(data),m_index(-1){}
  /** function operator. 
    * @return copies of data values that don't relate to monitors
    */
  T operator()()
  {
    ++m_index;
    while(m_alg->monitor_index.find(m_index) != m_alg->monitor_index.end()) ++m_index;
    return m_data[m_index];
  }
private:
  SaveISISNexus* m_alg; ///< this algorithm
  T* m_data;            ///< pointer to the data
  int m_index;          ///< index to m_data array
};

/** Execute the algorithm. Currently just calls SaveISISNexusProcessed but could
 *  call write other formats if support added
 *
 *  @throw runtime_error Thrown if algorithm cannot execute
 */
void SaveISISNexus::exec()
{
  // Retrieve the filename from the properties
  inputFilename = getPropertyValue("InputFileName");

  m_isisRaw = new ISISRAW2;
  rawFile =  fopen(inputFilename.c_str(), "rb");
  if (rawFile == NULL)
  {
    throw Exception::FileError("Cannot open file ", inputFilename);
  }
  m_isisRaw->ioRAW(rawFile,true);

  nper = m_isisRaw->t_nper; // number of periods
  nsp = m_isisRaw->t_nsp1;  // number of spectra
  ntc = m_isisRaw->t_ntc1;  // number of time channels
  nmon = m_isisRaw->i_mon; // number of monitors
  ndet = m_isisRaw->i_det; // number of detectors

  std::string outputFilename = getPropertyValue("OutputFileName");

  NXstatus status;
  float flt;

  status = NXopen(outputFilename.c_str(),NXACC_CREATE5,&handle);
  if (status != NX_OK)
  {
    throw std::runtime_error("Cannot open file " + outputFilename + " for writing.");
  }
  status = NXmakegroup(handle,"raw_data_1","NXentry");
  NXopengroup(handle,"raw_data_1","NXentry");
    write_isis_vms_compat();
    saveString("beamline"," ");
    
    flt = (float)m_isisRaw->rpb.r_dur;// could be wrong
    saveFloatOpen("collection_time",&flt,1);
    putAttr("units","second");
    close();
    
    saveStringOpen("definition","TOFRAW");
    putAttr("version","1.0");
    putAttr("url","http://definition.nexusformat.org/instruments/TOFRAW/?version=1.0");
    close();

    saveStringOpen("definition_local","ISISTOFRAW");
    putAttr("version","1.0");
    putAttr("url","http://svn.isis.rl.ac.uk/instruments/ISISTOFRAW/?version=1.0");
    close();

    flt = (float)m_isisRaw->rpb.r_dur;
    saveFloatOpen("duration",&flt,1);
    putAttr("units","second");
    close();
    
    start_time_str.assign(m_isisRaw->hdr.hd_date,m_isisRaw->hdr.hd_date+12);
    toISO8601(start_time_str);
    start_time_str += 'T';
    start_time_str += std::string(m_isisRaw->hdr.hd_time,m_isisRaw->hdr.hd_time+8);
    saveCharOpen("start_time",&start_time_str[0],19);
    putAttr("units","ISO8601");
    close();

    std::string str;
    str.assign(m_isisRaw->rpb.r_enddate,m_isisRaw->rpb.r_enddate+12);
    toISO8601(str);
    str += 'T';
    str += std::string(m_isisRaw->rpb.r_endtime,m_isisRaw->rpb.r_endtime+8);
    saveCharOpen("end_time",&str[0],19);
    putAttr("units","ISO8601");
    close();

    saveChar("title",m_isisRaw->r_title,80);
    saveInt("good_frames",&m_isisRaw->rpb.r_goodfrm);
    
    std::string experiment_identifier = boost::lexical_cast<std::string>(m_isisRaw->rpb.r_prop);
    saveChar("experiment_identifier",&experiment_identifier[0],experiment_identifier.size());
    int tmp_int(0);
    saveInt("measurement_first_run",&tmp_int);
    saveString("measurement_id"," ");
    saveString("measurement_label"," ");
    saveString("measurement_subid"," ");
    saveString("measurement_type"," ");

    saveCharOpen("name",&m_isisRaw->i_inst,8);
    putAttr("short_name",m_isisRaw->hdr.inst_abrv,3);
    close();

    logNotes();

    saveString("program_name","isisicp");

    saveFloatOpen("proton_charge",&m_isisRaw->rpb.r_gd_prtn_chrg,1);
    putAttr("units","uamp.hour");
    close();

    saveFloatOpen("proton_charge_raw",&m_isisRaw->rpb.r_tot_prtn_chrg,1);
    putAttr("units","uamp.hour");
    close();

    saveInt("raw_frames",&m_isisRaw->rpb.r_rawfrm);

    run_cycle();

    saveInt("run_number",&m_isisRaw->r_number);

    //script_name
    //seci_config

    instrument();
    
    make_detector_1_link();

    write_monitors();

    user();

    sample();

    runlog();

    selog();

  NXclosegroup(handle); // raw_data_1
  status = NXclose(&handle);

  delete m_isisRaw;
}

/**
  * Save int data.
  * @param name Name of the data set
  * @param data Pointer to the data source
  * @param size size of the data in sizeof(int)
  */
void SaveISISNexus::saveInt(const char* name,void* data, int size)
{
  saveIntOpen(name,data,size);
  close();
}

/**
  * Save char data.
  * @param name Name of the data set
  * @param data Pointer to the data source
  * @param size size of the data in sizeof(char)
  */
void SaveISISNexus::saveChar(const char* name,void* data, int size)
{
  saveCharOpen(name,data,size);
  close();
}

/**
  * Save float data.
  * @param name Name of the data set
  * @param data Pointer to the data source
  * @param size size of the data in sizeof(float)
  */
void SaveISISNexus::saveFloat(const char* name,void* data, int size)
{
  saveFloatOpen(name,data,size);
  close();
}

/**
  * Save int data and leave the dataset open.
  * @param name Name of the data set
  * @param data Pointer to the data source
  * @param size size of the data in sizeof(int)
  */
void SaveISISNexus::saveIntOpen(const char* name,void* data, int size)
{
  int dim[1];
  dim[0] = size;
  NXmakedata(handle,name,NX_INT32,1,dim);
  NXopendata(handle,name);
  NXputdata(handle,data);
}

/**
  * Save char data and leave the dataset open.
  * @param name Name of the data set
  * @param data Pointer to the data source
  * @param size size of the data in sizeof(char)
  */
void SaveISISNexus::saveCharOpen(const char* name,void* data, int size)
{
  int dim[1];
  dim[0] = size;
  NXmakedata(handle,name,NX_CHAR,1,dim);
  NXopendata(handle,name);
  NXputdata(handle,data);
}

/**
  * Save float data ald leave the dataset open.
  * @param name Name of the data set
  * @param data Pointer to the data source
  * @param size size of the data in sizeof(float)
  */
void SaveISISNexus::saveFloatOpen(const char* name,void* data, int size)
{
  int dim[1];
  dim[0] = size;
  NXmakedata(handle,name,NX_FLOAT32,1,dim);
  NXopendata(handle,name);
  NXputdata(handle,data);
}

/**
  * Save a vector of string in a dataset.
  * @param name :: Name of the data set
  * @param str_vec :: The vector to save
  * @return The line size
  */
int SaveISISNexus::saveStringVectorOpen(const char* name,const std::vector<std::string>& str_vec,int max_str_size)
{
  if (str_vec.empty())
  {
    saveStringOpen(name," ");
    return 0;
  }
  int buff_size = max_str_size;
  if (buff_size <= 0)
  for(std::size_t i = 0; i < str_vec.size(); ++i)
  {
    buff_size = std::max(buff_size,int(str_vec[i].size()));
  }
  if (buff_size <= 0) buff_size = 1;
  char *buff = new char[buff_size];
  int dim[2];
  dim[0] = str_vec.size();
  dim[1] = buff_size;
  NXmakedata(handle,name,NX_CHAR,2,dim);
  NXopendata(handle,name);
  for(std::size_t i = 0; i < str_vec.size(); ++i)
  {
    int start[] = {i,0};
    int sizes[] = {1,buff_size};
    const char* str = str_vec[i].c_str();
    std::fill_n(buff,buff_size,' ');
    int n = std::min(buff_size,int(str_vec[i].size()));
    std::copy(str,str + n,buff);
    NXputslab(handle,buff,start,sizes);
  }
  delete buff;
  return buff_size;
}

/**
  * Save a string in a dataset.
  * @param name :: Name of the data set
  * @param str_vec :: The vector to save
  */
void SaveISISNexus::saveString(const char* name,const std::string& str)
{
  if (str.empty()) return;
  std::string buff(str);
  saveChar(name,&buff[0],buff.size());
}

/**
  * Save a string in a dataset.
  * @param name :: Name of the data set
  * @param str_vec :: The vector to save
  */
void SaveISISNexus::saveStringOpen(const char* name,const std::string& str)
{
  if (str.empty()) return;
  std::string buff(str);
  saveCharOpen(name,&buff[0],buff.size());
}

void SaveISISNexus::putAttr(const char* name,const std::string& value)
{
  boost::scoped_array<char> buff(new char[value.size()]);
  std::copy(value.begin(),value.end(),buff.get());
  NXputattr(handle,name,buff.get(),value.size(),NX_CHAR);
}

void SaveISISNexus::putAttr(const char* name,char* value,int size)
{
  NXputattr(handle,name,value,size,NX_CHAR);
}

void SaveISISNexus::putAttr(const char* name,int value,int size)
{
  NXputattr(handle,name,&value,size,NX_INT32);
}

void SaveISISNexus::toISO8601(std::string& str)
{
  static const std::string months[] = {"JAN","FEB","MAR","APR","MAY","JUN","JUL","AUG","SEP","OCT","NOV","DEC"};
  static const std::string monthsn[] = {"01","02","03","04","05","06","07","08","09","10","11","12"};
  const std::string mon = str.substr(3,3);
  const std::string* i = std::find(months,months+12,mon);
  std::string iso8601 = str.substr(7,4) + "-" + monthsn[i-months] + "-" + str.substr(0,2);
  str = iso8601;
}

/// Write isis_vms_compat
void SaveISISNexus::write_isis_vms_compat()
{
  NXstatus status = NXmakegroup(handle,"isis_vms_compat","IXvms");
  status = NXopengroup(handle,"isis_vms_compat","IXvms");
  int ndet = m_isisRaw->i_det;
  int nmon = m_isisRaw->i_mon;

  saveInt("ADD",&m_isisRaw->add,9);
  saveInt("CODE",m_isisRaw->code,ndet);
  saveInt("CRAT",m_isisRaw->crat,ndet);

  write_rpb();
  write_spb();
  write_vpb();
  saveInt("DAEP",&m_isisRaw->daep,64);
  saveInt("DELT",m_isisRaw->delt,ndet);
  saveInt("FORM",&m_isisRaw->data_format);
  saveChar("HDR",&m_isisRaw->hdr,80);
  saveFloat("LEN2",m_isisRaw->len2, ndet);
  saveInt("MDET",m_isisRaw->mdet,nmon);
  saveInt("MODN",m_isisRaw->modn,ndet);
  saveInt("MONP",m_isisRaw->monp,nmon);
  saveInt("MPOS",m_isisRaw->mpos,ndet);
  saveChar("NAME",m_isisRaw->i_inst,8);
  saveInt("NDET",&ndet);
  saveInt("NFPP",&m_isisRaw->t_nfpp);
  saveInt("NMON",&nmon);
  saveInt("NPER",&m_isisRaw->t_nper);
  saveInt("NSER",&m_isisRaw->e_nse);
  saveInt("NSP1",&m_isisRaw->t_nsp1);
  saveInt("NTC1",&m_isisRaw->t_ntc1);
  saveInt("NTRG",&m_isisRaw->t_ntrg);
  saveInt("NUSE",&m_isisRaw->i_use);
  saveInt("PMAP",&m_isisRaw->t_pmap,256);
  saveInt("PRE1",&m_isisRaw->t_pre1);
  saveInt("RUN",&m_isisRaw->r_number);
  saveInt("SPEC",m_isisRaw->spec,ndet); 
  saveInt("TCM1",&m_isisRaw->t_tcm1);
  saveFloat("TCP1",m_isisRaw->t_tcp1,20);
  saveInt("TIMR",m_isisRaw->timr,ndet);
  saveChar("TITL",m_isisRaw->r_title,80);
  saveFloat("TTHE",m_isisRaw->tthe,ndet);
  saveInt("UDET",m_isisRaw->udet,ndet);
  saveInt("ULEN",&m_isisRaw->u_len);
  std::string user_info(160,' ');
  if (m_isisRaw->u_len > 0)
  {
    std::copy((char*)&m_isisRaw->user,(char*)&m_isisRaw->user+m_isisRaw->u_len,user_info.begin());
  }
  saveString("USER",user_info);
  saveInt("VER1",&m_isisRaw->frmt_ver_no);
  saveInt("VER2",&m_isisRaw->ver2);
  saveInt("VER3",&m_isisRaw->ver3);
  saveInt("VER4",&m_isisRaw->ver4);
  saveInt("VER5",&m_isisRaw->ver5);
  saveInt("VER6",&m_isisRaw->ver6);
  saveInt("VER7",&m_isisRaw->ver7);
  saveInt("VER8",&m_isisRaw->ver8);
  int tmp_int(0);
  saveInt("VER9",&tmp_int);

  int n = m_isisRaw->logsect.nlines;
  log_notes.resize(n);
  for(int i = 0; i < n; ++i)
  {
    log_notes[i].assign(m_isisRaw->logsect.lines[i].data,m_isisRaw->logsect.lines[i].len);
  }
  int ll = saveStringVectorOpen("NOTE",log_notes);
  saveInt("NTNL",&n);
  saveInt("NTLL",&ll);

  NXclosegroup(handle); // isis_vms_compat
}

void SaveISISNexus::instrument()
{
  NXstatus status = NXmakegroup(handle,"instrument","NXinstrument");
  status = NXopengroup(handle,"instrument","NXinstrument");
    saveCharOpen("name",&m_isisRaw->i_inst,8);
    putAttr("short_name",m_isisRaw->hdr.inst_abrv,3);
    close();
  dae();
  detector_1();
  moderator();
  source();
  NXclosegroup(handle);
}

void SaveISISNexus::detector_1()
{
  NXstatus status = NXmakegroup(handle,"detector_1","NXdata");
  status = NXopengroup(handle,"detector_1","NXdata");

  for(int i = 0; i < nmon; ++i)
  {
    int si = int(std::distance(m_isisRaw->spec,std::find(m_isisRaw->spec,m_isisRaw->spec+nsp,m_isisRaw->mdet[i])));
    monitor_index[si] = i;
  }

  // write counts
  int dim[3];
  dim[0] = nper;
  dim[1] = nsp - nmon;
  dim[2] = ntc;
  status = NXmakedata(handle,"counts",NX_INT32,3,dim);
  NXopendata(handle,"counts");
  putAttr("units","counts");
  putAttr("signal",1);
  putAttr("axes","period_index,spectrum_index,time_of_flight");

  int size[] = {1,1,ntc};
  int index = 0;
  for(int p = 0; p < nper; ++p)
  {
    int ispec = 0;
    m_isisRaw->skipData(rawFile,index++);
    for(int si = 0; si < nsp; ++si)
    {
      if (monitor_index.find(si) != monitor_index.end())
      {
        m_isisRaw->readData(rawFile,index);
        monitorData.insert(monitorData.end(),m_isisRaw->dat1+1,m_isisRaw->dat1+ntc+1);
      }
      else
      {
        m_isisRaw->readData(rawFile,index);
        int start[] = {p,ispec,0};
        NXputslab(handle,m_isisRaw->dat1+1,start,size);
        ++ispec;
      }
      ++index;
    }
  }
  NXgetdataID(handle,&counts_link);
  NXclosedata(handle);

  NXmakelink(handle,&period_index_link);

  std::vector<int> spec_minus_monitors(nsp-nmon);
  std::generate(spec_minus_monitors.begin(),spec_minus_monitors.end(),getWithoutMonitors<int>(this,m_isisRaw->spec));
  saveIntOpen("spectrum_index",&spec_minus_monitors[0],nsp-nmon);
  NXgetdataID(handle,&spectrum_index_link);
  close();

  NXmakelink(handle,&time_of_flight_link);
  NXmakelink(handle,&time_of_flight_raw_link);

  std::vector<float> float_vec(ndet-nmon);
  std::generate(float_vec.begin(),float_vec.end(),getWithoutMonitors<float>(this,m_isisRaw->delt));
  saveFloat("delt",&float_vec[0],ndet-nmon);

  saveFloat("source_detector_distance",&m_isisRaw->ivpb.i_sddist,1);

  // using the same float_vec, size unchanged ndet-nmon
  std::generate(float_vec.begin(),float_vec.end(),getWithoutMonitors<float>(this,m_isisRaw->len2));
  saveFloatOpen("distance",&float_vec[0],ndet-nmon);
  putAttr("units","metre");
  close();

  // using the same float_vec, size unchanged ndet-nmon
  std::generate(float_vec.begin(),float_vec.end(),getWithoutMonitors<float>(this,m_isisRaw->tthe));
  saveFloatOpen("polar_angle",&float_vec[0],ndet-nmon);
  putAttr("units","degree");
  close();

  NXclosegroup(handle);
}

/**
  * Write instrument/moderator group
  */
void SaveISISNexus::moderator()
{
  NXstatus status = NXmakegroup(handle,"moderator","NXmoderator");
  status = NXopengroup(handle,"moderator","NXmoderator");

  float l1 = - m_isisRaw->ivpb.i_l1;
  saveFloatOpen("distance",&l1,1);
  putAttr("units","metre");

  NXclosegroup(handle);
}

/**
  * Write instrument/source group
  */
void SaveISISNexus::source()
{
  NXstatus status = NXmakegroup(handle,"source","NXsource");
  status = NXopengroup(handle,"source","NXsource");

  saveString("name","ISIS");
  saveString("probe","neutrons");
  saveString("type","Pulsed Neutron Source");

  NXclosegroup(handle);
}

/**
  * Create group "detector_1" at NXentry level and link to some of the data in instrument/detector_1
  */
void SaveISISNexus::make_detector_1_link()
{
  NXstatus status = NXmakegroup(handle,"detector_1","NXdata");
  status = NXopengroup(handle,"detector_1","NXdata");

  NXmakelink(handle,&counts_link);
  NXmakelink(handle,&period_index_link);
  NXmakelink(handle,&spectrum_index_link);
  NXmakelink(handle,&time_of_flight_link);

  NXclosegroup(handle);
}

/**
  * Get a pointer to the saved monitor data
  * @param period Period, starts with 0
  * @param imon Monitor index (not its spectrum number)
  */
int *SaveISISNexus::getMonitorData(int period,int imon)
{
  return & monitorData[period*m_isisRaw->i_mon*m_isisRaw->t_ntc1 + imon*m_isisRaw->t_ntc1];
}

void SaveISISNexus::write_monitors()
{
  int nmon = m_isisRaw->i_mon;
  for(int i = 0; i < nmon; ++i)
  {
    monitor_i(i);
  }
}

/**
  * Write monitor_i gorup
  * @param i Index of a monitor
  */
void SaveISISNexus::monitor_i(int i)
{
  int nper = m_isisRaw->t_nper; // number of periods
  int ntc = m_isisRaw->t_ntc1;  // number of time channels
  int dim[] = {nper,1,ntc};
  int size[] = {1,1,ntc};
  std::ostringstream ostr;
  int mon_num = i + 1;
  ostr << "monitor_" << mon_num;
  NXmakegroup(handle,ostr.str().c_str(),"NXmonitor");
  NXopengroup(handle,ostr.str().c_str(),"NXmonitor");
  
//  int imon = m_isisRaw->mdet[i]; // spectrum index
  NXmakedata(handle,"data",NX_INT32,3,dim);
  NXopendata(handle,"data");
  for(int p = 0; p < nper; ++p)
  {
    int start[] = {p,0,0};
    NXputslab(handle,getMonitorData(p,i),start,size);
  }
  putAttr("units","counts");
  putAttr("signal",1);
  putAttr("axes","period_index,spectrum_index,time_of_flight");
  NXclosedata(handle);

  saveInt("monitor_number",&mon_num);
  NXmakelink(handle,&period_index_link);
  saveInt("spectrum_index",&m_isisRaw->mdet[i]);
  NXmakelink(handle,&time_of_flight_link);

  NXclosegroup(handle);
}

void SaveISISNexus::dae()
{
  NXmakegroup(handle,"dae","IXdae");
  NXopengroup(handle,"dae","IXdae");
  
  saveString("detector_table_file"," ");
  saveString("spectra_table_file"," ");
  saveString("wiring_table_file"," ");

  saveIntOpen("period_index",m_isisRaw->t_pmap,nper);
  NXgetdataID(handle,&period_index_link);
  close();

  NXmakegroup(handle,"time_channels_1","IXtime_channels");
  NXopengroup(handle,"time_channels_1","IXtime_channels");

  boost::scoped_array<float> timeChannels(new float[ntc+1]);
  m_isisRaw->getTimeChannels(timeChannels.get(),ntc+1);
  saveFloatOpen("time_of_flight",timeChannels.get(),ntc+1);
  putAttr("axis",1);
  putAttr("primary",1);
  putAttr("units","microseconds");
  NXgetdataID(handle,&time_of_flight_link);
  close();

  saveIntOpen("time_of_flight_raw",m_isisRaw->t_tcb1,ntc+1);
  putAttr("units","pulses");
  putAttr("frequency","32 MHz");
  NXgetdataID(handle,&time_of_flight_raw_link);
  close();

  NXclosegroup(handle); // time_channels_1

  NXclosegroup(handle); // dae
}

void SaveISISNexus::user()
{
  NXmakegroup(handle,"user_1","NXuser");
  NXopengroup(handle,"user_1","NXuser");

  saveChar("name",m_isisRaw->user.r_user,20);
  saveChar("affiliation",m_isisRaw->user.r_instit,20);
  
  NXclosegroup(handle); // user_1
}

void SaveISISNexus::sample()
{
  NXmakegroup(handle,"sample","NXsample");
  NXopengroup(handle,"sample","NXsample");

  saveChar("name",m_isisRaw->spb.e_name,40);
  saveFloat("height",&m_isisRaw->spb.e_height,1);
  saveFloat("width",&m_isisRaw->spb.e_width,1);
  saveFloat("thickness",&m_isisRaw->spb.e_thick,1);
  saveString("id"," ");
  float tmp(0.0);
  saveFloat("distance",&tmp,1);
  std::string shape[] = {"cylinder","flat plate","HRPD slab","unknown"};
  int i = m_isisRaw->spb.e_geom - 1;
  if (i < 0 || i > 3) i = 3;
  saveString("shape",shape[i]);
  std::string type[] = {"sample+can","empty can","vanadium","absorber","nothing","sample, no can","unknown"};
  i = m_isisRaw->spb.e_type - 1;
  if (i < 0 || i > 6) i = 6;
  saveString("type",type[i]);
  
  NXclosegroup(handle); // sample
}

/**
  * Create and write run logs form <RawFilename>_ICPstatus.txt log file
  */
void SaveISISNexus::runlog()
{
  progress(0);
  std::string ICPstatus_filename = inputFilename;
  std::string ICPevent_filename;
  std::string::size_type i = ICPstatus_filename.find_last_of('.');
  if (i != std::string::npos) // remove the file extension
  {
    ICPstatus_filename.erase(i);
  }
  ICPevent_filename = ICPstatus_filename + "_ICPevent.txt";
  ICPstatus_filename+= "_ICPstatus.txt";

  std::ifstream fil(ICPstatus_filename.c_str());
  if (!fil)
  {
    g_log.warning("Cannot find the ICPstatus file. Skipping runlog");
    progress(0.5);
    return;
  }

  std::vector<float> time_vec;
  std::vector<int> period_vec;
  std::vector<int> is_running_vec;
  std::vector<int> is_waiting_vec;
  std::vector<int> good_frames_vec;
  std::vector<int> raw_frames_vec;
  std::vector<int> monitor_sum_1_vec;
  std::vector<int> total_counts_vec;
  std::vector<int> run_status_vec;
  std::vector<float> proton_charge_vec;
  std::vector<float> proton_charge_raw_vec;
  std::vector<float> dae_beam_current_vec;
  std::vector<float> count_rate_vec;
  std::vector<float> np_ratio_vec;

  start_time_str[10] = ' '; // make it compatible with boost::posix_time::ptime
  boost::posix_time::ptime start_time(boost::posix_time::time_from_string(start_time_str));
  start_time_str[10] = 'T'; // revert
  std::string line;
  std::getline(fil,line); // skip the first line
  while(std::getline(fil,line))
  {
    std::string date_time_str;
    int period;
    int is_running;
    int is_waiting;
    int good_frames;
    int raw_frames;
    int monitor_sum_1;
    int total_counts;
    float proton_charge;
    float proton_charge_raw;
    float dae_beam_current;
    float count_rate;
    float np_ratio;
    std::istringstream istr(line);
    istr >> date_time_str >> period >> is_running >> is_waiting >> good_frames >> raw_frames 
      >> proton_charge >> proton_charge_raw >> monitor_sum_1 >> dae_beam_current >> total_counts >> count_rate >> np_ratio;
    date_time_str[10] = ' ';
    boost::posix_time::ptime time(boost::posix_time::time_from_string(date_time_str));
    boost::posix_time::time_duration dt = time - start_time;
    time_vec.push_back(float(dt.total_seconds()));
    period_vec.push_back(period);
    is_running_vec.push_back(is_running);
    is_waiting_vec.push_back(is_waiting);
    good_frames_vec.push_back(good_frames);
    raw_frames_vec.push_back(raw_frames);
    monitor_sum_1_vec.push_back(monitor_sum_1);
    total_counts_vec.push_back(total_counts);
    proton_charge_vec.push_back(proton_charge);
    proton_charge_raw_vec.push_back(proton_charge_raw);
    dae_beam_current_vec.push_back(dae_beam_current);
    count_rate_vec.push_back(count_rate);
    np_ratio_vec.push_back(np_ratio);
  }
  fil.close();

  run_status_vec.resize(time_vec.size());
  std::transform(is_running_vec.begin(),is_running_vec.end(),run_status_vec.begin(),std::bind2nd(std::plus<int>(),1));

  NXmakegroup(handle,"runlog","IXrunlog");
  NXopengroup(handle,"runlog","IXrunlog");

  write_runlog("period",&time_vec[0],&period_vec[0],NX_INT32,time_vec.size(),"none");
  write_runlog("is_running",&time_vec[0],&is_running_vec[0],NX_INT32,time_vec.size(),"none");
  write_runlog("is_waiting",&time_vec[0],&is_waiting_vec[0],NX_INT32,time_vec.size(),"none");
  write_runlog("good_frames",&time_vec[0],&good_frames_vec[0],NX_INT32,time_vec.size(),"frames");
  write_runlog("raw_frames",&time_vec[0],&raw_frames_vec[0],NX_INT32,time_vec.size(),"frames");
  write_runlog("monitor_sum_1",&time_vec[0],&monitor_sum_1_vec[0],NX_INT32,time_vec.size(),"counts");
  write_runlog("total_counts",&time_vec[0],&total_counts_vec[0],NX_INT32,time_vec.size(),"counts");
  write_runlog("proton_charge",&time_vec[0],&proton_charge_vec[0],NX_FLOAT32,time_vec.size(),"uAh");
  write_runlog("proton_charge_raw",&time_vec[0],&proton_charge_raw_vec[0],NX_FLOAT32,time_vec.size(),"uAh");
  write_runlog("dae_beam_current",&time_vec[0],&dae_beam_current_vec[0],NX_FLOAT32,time_vec.size(),"uAh");
  write_runlog("count_rate",&time_vec[0],&count_rate_vec[0],NX_FLOAT32,time_vec.size(),"counts");
  write_runlog("np_ratio",&time_vec[0],&np_ratio_vec[0],NX_FLOAT32,time_vec.size(),"nones");

  write_runlog("run_status",&time_vec[0],&run_status_vec[0],NX_INT32,time_vec.size(),"none");

  // read in ICPevent file and create icp_event log
  std::ifstream icpevent_fil(ICPevent_filename.c_str());
  if (!icpevent_fil)
  {
    g_log.warning("Cannot find the ICPevent file");
    progress(0.5);
    return;
  }

  time_vec.clear();
  std::vector<std::string> event_vec;
  while(std::getline(icpevent_fil,line))
  {
    if (line.empty()) continue;
    std::string date_time_str = line.substr(0,19);
    date_time_str[10] = ' ';
    boost::posix_time::ptime time(boost::posix_time::time_from_string(date_time_str));
    boost::posix_time::time_duration dt = time - start_time;
    time_vec.push_back(float(dt.total_seconds()));
    event_vec.push_back(line.substr(20));
  }
  icpevent_fil.close();

  NXmakegroup(handle,"icp_event","NXlog");
  NXopengroup(handle,"icp_event","NXlog");

  saveFloatOpen("time",&time_vec[0],time_vec.size());
  putAttr("start",start_time_str);
  putAttr("units","seconds");
  close();

  saveStringVectorOpen("value",event_vec,72);
  putAttr("units"," ");
  close();
  NXclosegroup(handle); // icp_event

  NXclosegroup(handle); // runlog
  progress(0.5);
}

/**
  * Write one run log.
  * @param name The log name
  * @param times The pointer to the time array
  * @param data The pointer to the data
  * @param type The type of the data
  * @param size The size of the data
  * @param units The units of the data
  */
void SaveISISNexus::write_runlog(const char* name, void* times, void* data,int type,int size,const std::string& units)
{
  write_logOpen(name,times,data,type,size,units);
  closegroup();
}

/**
  * Writes a NXlog and leaves it open.
  * @param name The log name
  * @param times The pointer to the time array
  * @param data The pointer to the data
  * @param type The type of the data
  * @param size The size of the data
  * @param units The units of the data
  */
void SaveISISNexus::write_logOpen(const char* name, void* times, void* data,int type,int size,const std::string& units)
{
  NXmakegroup(handle,name,"NXlog");
  NXopengroup(handle,name,"NXlog");

  saveFloatOpen("time",times,size);
  putAttr("start",start_time_str);
  putAttr("units","seconds");
  close();

  if (type == NX_INT32)
  {
    saveIntOpen("value",data,size);
  }
  else if (type == NX_FLOAT32)
  {
    saveFloatOpen("value",data,size);
  }
  putAttr("units",units);
  close();
}

void SaveISISNexus::selog()
{
  // find log files with names <RawFilenameWithoutExt>_LogName.txt and save them in potentialLogFiles
  std::vector<std::string> potentialLogFiles;
  Poco::File l_path( inputFilename );
  std::string l_filenamePart = Poco::Path(l_path.path()).getFileName();
  std::string::size_type i = l_filenamePart.find_last_of('.');
  if (i != std::string::npos) // remove the file extension
  {
    l_filenamePart.erase(i);
  }
  std::string base_name = l_filenamePart;
  Poco::RegularExpression regex(l_filenamePart + "_.*\\.txt", Poco::RegularExpression::RE_CASELESS );
  Poco::DirectoryIterator end_iter;
  for ( Poco::DirectoryIterator dir_itr(Poco::Path(inputFilename).parent()); dir_itr != end_iter; ++dir_itr )
  {
    if ( !Poco::File(dir_itr->path() ).isFile() ) continue;

    l_filenamePart = Poco::Path(dir_itr->path()).getFileName();

    if ( regex.match(l_filenamePart) )
    {
      potentialLogFiles.push_back( dir_itr->path() );
    }
  }

  Progress prog(this,0.5,1,potentialLogFiles.size());

  NXmakegroup(handle,"selog","IXselog");
  NXopengroup(handle,"selog","IXselog");

  // create a log for each of the found log files
  int nBase = base_name.size() + 1;
  for(std::size_t i = 0; i < potentialLogFiles.size(); ++i)
  {
    std::string logName = Poco::Path(potentialLogFiles[i]).getFileName();
    logName.erase(0,nBase);
    logName.erase(logName.size() - 4);
    if (logName.size() > 3)
    {
      std::string icp = logName.substr(0,3);
      std::transform(icp.begin(),icp.end(),icp.begin(),toupper);
      if (icp == "ICP") continue;
    }

    std::ifstream fil(potentialLogFiles[i].c_str());
    if (!fil)
    {
      g_log.warning("Cannot open log file " + potentialLogFiles[i]);
      continue;
    }

    start_time_str[10] = ' '; // make it compatible with boost::posix_time::ptime
    boost::posix_time::ptime start_time(boost::posix_time::time_from_string(start_time_str));
    start_time_str[10] = 'T'; // revert
    std::vector<float> time_vec;
    std::vector<std::string> str_vec;
    std::vector<float> flt_vec;
    std::string line;
    bool isNotNumeric = false;
    while(std::getline(fil,line))
    {
      if (line.empty()) continue;
      std::string date_time_str = line.substr(0,19);
      date_time_str[10] = ' ';
      boost::posix_time::ptime time(boost::posix_time::time_from_string(date_time_str));
      boost::posix_time::time_duration dt = time - start_time;
      time_vec.push_back(float(dt.total_seconds()));
      std::istringstream istr(line.substr(20));
      // check if the data are numeric then save them in flt_vec
      if (!isNotNumeric)
      {
        float flt;
        istr >> flt;
        if (istr.bad() || istr.fail())
        {
          isNotNumeric = true;
        }
        else
        {
          flt_vec.push_back(flt);
        }
      }
      str_vec.push_back(istr.str());
    }
    fil.close();
    NXmakegroup(handle,&logName[0],"IXseblock");
    NXopengroup(handle,&logName[0],"IXseblock");

    {
    saveString("vi_name"," ");
    saveString("set_control"," ");
    saveString("read_control"," ");
    float tmp = 0.0;
    saveFloatOpen("setpoint",&tmp,1);
    putAttr("units","mV");
    close();
    }

    NXmakegroup(handle,"value_log","NXlog");
    NXopengroup(handle,"value_log","NXlog");

    saveFloatOpen("time",&time_vec[0],time_vec.size());
    putAttr("start",start_time_str);
    putAttr("units","seconds");
    close();

    if (flt_vec.size() == str_vec.size())
    {
      saveFloatOpen("value",&flt_vec[0],flt_vec.size());
    }
    else
    {
      saveStringVectorOpen("value",str_vec);
    }
    putAttr("units"," ");
    close();

    saveString("name"," ");

    NXclosegroup(handle); // value_log

    NXclosegroup(handle); // logName

    prog.report();
  }

  NXclosegroup(handle); // selog

  progress(1);
}

void SaveISISNexus::logNotes()
{
  saveStringVectorOpen("notes",log_notes);
  close();
}

void SaveISISNexus::run_cycle()
{
  saveString("run_cycle"," ");
}

void SaveISISNexus::write_rpb()
{
  int dim[] = {32,4};
  NXmakedata(handle,"CRPB",NX_CHAR,2,dim);
  NXopendata(handle,"CRPB");
  NXputdata(handle,&m_isisRaw->rpb);
  NXclosedata(handle);

  saveInt("IRPB",&m_isisRaw->rpb,32);
  saveFloat("RRPB",&m_isisRaw->rpb,32);

}

void SaveISISNexus::write_spb()
{
  int dim[] = {64,4};
  NXmakedata(handle,"CSPB",NX_CHAR,2,dim);
  NXopendata(handle,"CSPB");
  NXputdata(handle,&m_isisRaw->spb);
  NXclosedata(handle);

  saveInt("SPB",&m_isisRaw->spb,64);
  saveInt("ISPB",&m_isisRaw->spb,64);
  saveFloat("RSPB",&m_isisRaw->spb,64);

}

void SaveISISNexus::write_vpb()
{
  saveInt("IVPB",&m_isisRaw->ivpb,64);
  saveFloat("RVPB",&m_isisRaw->ivpb,64);

}

} // namespace DataHandling
} // namespace Mantid
