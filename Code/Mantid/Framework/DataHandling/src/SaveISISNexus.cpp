/*WIKI* 


*WIKI*/
// SaveISISNexus
// @author Freddie Akeroyd, STFC ISIS Faility
// @author Ronald Fowler, STFC eScience. Modified to fit with SaveISISNexusProcessed
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/SaveISISNexus.h"
#include "MantidAPI/FileProperty.h"

#include "LoadRaw/isisraw2.h"

#include <boost/regex.hpp>
#include <boost/scoped_array.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/lexical_cast.hpp>

#include <Poco/File.h>
#include <Poco/Path.h>
#include <Poco/DirectoryIterator.h>
#include <Poco/DateTimeParser.h>
#include <Poco/DateTimeFormat.h>

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
  this->setWikiSummary("The SaveISISNexus algorithm will convert a RAW file to a NeXus file.");
  this->setOptionalMessage("The SaveISISNexus algorithm will convert a RAW file to a NeXus file.");
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

  m_handle = new ::NeXus::File(outputFilename, NXACC_CREATE5);
  m_handle->makeGroup("raw_data_1","NXentry", true);
  write_isis_vms_compat();
  m_handle->writeData("beamline", " ");

  m_handle->writeData("collection_time", static_cast<float>(m_isisRaw->rpb.r_dur)); // could be wrong
  m_handle->openData("collection_time");
  m_handle->putAttr("units","second");
  m_handle->closeData();

  m_handle->writeData("definition","TOFRAW");
  m_handle->openData("definition");
  m_handle->putAttr("version","1.0");
  m_handle->putAttr("url","http://definition.nexusformat.org/instruments/TOFRAW/?version=1.0");
  m_handle->closeData();

  m_handle->writeData("definition_local","ISISTOFRAW");
  m_handle->openData("definition_local");
  m_handle->putAttr("version","1.0");
  m_handle->putAttr("url","http://svn.isis.rl.ac.uk/instruments/ISISTOFRAW/?version=1.0");
  m_handle->closeData();

  m_handle->writeData("duration", static_cast<float>(m_isisRaw->rpb.r_dur));
  m_handle->openData("duration");
  m_handle->putAttr("units","second");
  m_handle->closeData();

    start_time_str.assign(m_isisRaw->hdr.hd_date,m_isisRaw->hdr.hd_date+12);
    toISO8601(start_time_str);
    start_time_str += 'T';
    start_time_str += std::string(m_isisRaw->hdr.hd_time,m_isisRaw->hdr.hd_time+8);
    m_handle->writeData("start_time",start_time_str);
    m_handle->openData("start_time");
    m_handle->putAttr("units","ISO8601");
    m_handle->closeData();

    std::string str;
    str.assign(m_isisRaw->rpb.r_enddate,m_isisRaw->rpb.r_enddate+12);
    toISO8601(str);
    str += 'T';
    str += std::string(m_isisRaw->rpb.r_endtime,m_isisRaw->rpb.r_endtime+8);
    m_handle->writeData("end_time",str);
    m_handle->openData("end_time");
    m_handle->putAttr("units","ISO8601");
    m_handle->closeData();

    m_handle->writeData("title",std::string(m_isisRaw->r_title));
    m_handle->writeData("good_frames",m_isisRaw->rpb.r_goodfrm);
    
    m_handle->writeData("experiment_identifier",boost::lexical_cast<std::string>(m_isisRaw->rpb.r_prop));
    m_handle->writeData("measurement_first_run",static_cast<int>(0));
    m_handle->writeData("measurement_id"," ");
    m_handle->writeData("measurement_label"," ");
    m_handle->writeData("measurement_subid"," ");
    m_handle->writeData("measurement_type"," ");

    m_handle->writeData("name",std::string(m_isisRaw->i_inst));
    m_handle->openData("name");
    m_handle->putAttr("short_name",std::string(m_isisRaw->hdr.inst_abrv));
    m_handle->closeData();

    logNotes();

    m_handle->writeData("program_name","isisicp");

    m_handle->writeData("proton_charge",m_isisRaw->rpb.r_gd_prtn_chrg);
    m_handle->openData("proton_charge");
    m_handle->putAttr("units","uamp.hour");
    m_handle->closeData();

    m_handle->writeData("proton_charge_raw",m_isisRaw->rpb.r_tot_prtn_chrg);
    m_handle->openData("proton_charge_raw");
    m_handle->putAttr("units","uamp.hour");
    m_handle->closeData();

    m_handle->writeData("raw_frames",m_isisRaw->rpb.r_rawfrm);

    run_cycle();

    m_handle->writeData("run_number",m_isisRaw->r_number);

    //script_name
    //seci_config

    instrument();
    
    make_detector_1_link();

    write_monitors();

    user();

    sample();

    runlog();

    selog();

  m_handle->closeGroup(); // raw_data_1

  delete m_handle;
  delete m_isisRaw;
}

/**
  * Save a vector of string in a dataset.
  * @param name :: Name of the data set
  * @param str_vec :: The vector to save
  * @param max_str_size :: The maximum string size
  * @return The line size
  */
int SaveISISNexus::saveStringVectorOpen(const char* name,const std::vector<std::string>& str_vec,int max_str_size)
{
  if (str_vec.empty())
  {
    m_handle->writeData(name," ");
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
  std::vector<int64_t> dims(2);
  dims[0] = static_cast<int64_t>(str_vec.size());
  dims[1] = static_cast<int64_t>(buff_size);
  m_handle->makeData(name, ::NeXus::CHAR, dims, true);
  for(std::size_t i = 0; i < str_vec.size(); ++i)
  {
    std::vector<int64_t> start(2,0);
    start[0] = static_cast<int64_t>(i);
    std::vector<int64_t> sizes(2,1);
    sizes[1] = static_cast<int64_t>(buff_size);
    const char* str = str_vec[i].c_str();
    std::fill_n(buff,buff_size,' ');
    int n = std::min(buff_size,int(str_vec[i].size()));
    std::copy(str,str + n,buff);
    m_handle->putSlab(buff,start,sizes);
  }
  delete [] buff;
  return buff_size;
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
  m_handle->makeGroup("isis_vms_compat","IXvms", true);
  int ndet = m_isisRaw->i_det;
  int nmon = m_isisRaw->i_mon;

  // some nasty reinterpret of a struct as an integer array
  std::vector<int64_t> dims(1,9);
  m_handle->makeData("ADD", ::NeXus::INT32, dims, true);
  m_handle->putData(&m_isisRaw->add);
  m_handle->closeData();

  m_handle->writeData("CODE", std::vector<int>(m_isisRaw->code, m_isisRaw->code+ndet));
  m_handle->writeData("CRAT", std::vector<int>(m_isisRaw->crat, m_isisRaw->crat+ndet));

  write_rpb();
  write_spb();
  write_vpb();

  // some nasty reinterpret of a struct as an integer array
  dims[0] = 64;
  m_handle->makeData("DAEP", ::NeXus::INT32, dims, true);
  m_handle->putData((void*)&m_isisRaw->daep);
  m_handle->closeData();

  m_handle->writeData("DELT",std::vector<float>(m_isisRaw->delt,m_isisRaw->delt+ndet)); // was being done as int array
  m_handle->writeData("FORM",m_isisRaw->data_format);

  // some nasty reinterpret of a struct as a character array
  dims[0] = 80;
  m_handle->makeData("HDR", ::NeXus::CHAR, dims);
  m_handle->putData((void*)&m_isisRaw->hdr);
  m_handle->closeData();

  m_handle->writeData("LEN2",std::vector<float>(m_isisRaw->len2, m_isisRaw->len2+ndet));
  m_handle->writeData("MDET",std::vector<int>(m_isisRaw->mdet,m_isisRaw->mdet+nmon));
  m_handle->writeData("MODN",std::vector<int>(m_isisRaw->modn,m_isisRaw->modn+ndet));
  m_handle->writeData("MONP",std::vector<int>(m_isisRaw->monp,m_isisRaw->monp+nmon));
  m_handle->writeData("MPOS",std::vector<int>(m_isisRaw->mpos,m_isisRaw->mpos+ndet));
  m_handle->writeData("NAME", std::string(m_isisRaw->i_inst));
  m_handle->writeData("NDET",ndet);
  m_handle->writeData("NFPP",m_isisRaw->t_nfpp);
  m_handle->writeData("NMON",nmon);
  m_handle->writeData("NPER",m_isisRaw->t_nper);
  m_handle->writeData("NSER",m_isisRaw->e_nse);
  m_handle->writeData("NSP1",m_isisRaw->t_nsp1);
  m_handle->writeData("NTC1",m_isisRaw->t_ntc1);
  m_handle->writeData("NTRG",m_isisRaw->t_ntrg);
  m_handle->writeData("NUSE",m_isisRaw->i_use);
  m_handle->writeData("PMAP",std::vector<int>(m_isisRaw->t_pmap,m_isisRaw->t_pmap+256));
  m_handle->writeData("PRE1",m_isisRaw->t_pre1);
  m_handle->writeData("RUN",m_isisRaw->r_number);
  m_handle->writeData("SPEC",std::vector<int>(m_isisRaw->spec,m_isisRaw->spec+ndet));
  m_handle->writeData("TCM1",m_isisRaw->t_tcm1[0]);
  m_handle->writeData("TCP1",std::vector<float>(&m_isisRaw->t_tcp1[0][0],&m_isisRaw->t_tcp1[0][0]+20));
  m_handle->writeData("TIMR",std::vector<int>(m_isisRaw->timr,m_isisRaw->timr+ndet));
  m_handle->writeData("TITL",std::string(m_isisRaw->r_title));
  m_handle->writeData("TTHE",std::vector<float>(m_isisRaw->tthe,m_isisRaw->tthe+ndet));
  m_handle->writeData("UDET",std::vector<int>(m_isisRaw->udet,m_isisRaw->udet+ndet));
  m_handle->writeData("ULEN",m_isisRaw->u_len);
  std::string user_info(160,' ');
  if (m_isisRaw->u_len > 0)
  {
    std::copy((char*)&m_isisRaw->user,(char*)&m_isisRaw->user+m_isisRaw->u_len,user_info.begin());
  }
  m_handle->writeData("USER",user_info);
  m_handle->writeData("VER1",m_isisRaw->frmt_ver_no);
  m_handle->writeData("VER2",m_isisRaw->ver2);
  m_handle->writeData("VER3",m_isisRaw->ver3);
  m_handle->writeData("VER4",m_isisRaw->ver4);
  m_handle->writeData("VER5",m_isisRaw->ver5);
  m_handle->writeData("VER6",m_isisRaw->ver6);
  m_handle->writeData("VER7",m_isisRaw->ver7);
  m_handle->writeData("VER8",m_isisRaw->ver8);
  m_handle->writeData("VER9",int(0));

  int n = m_isisRaw->logsect.nlines;
  log_notes.resize(n);
  for(int i = 0; i < n; ++i)
  {
    log_notes[i].assign(m_isisRaw->logsect.lines[i].data,m_isisRaw->logsect.lines[i].len);
  }
  int ll = saveStringVectorOpen("NOTE",log_notes);
  m_handle->writeData("NTNL",n);
  m_handle->writeData("NTLL",ll);

  m_handle->closeGroup(); // isis_vms_compat
}

void SaveISISNexus::instrument()
{
  m_handle->makeGroup("instrument","NXinstrument", true);
  m_handle->writeData("name",std::string(m_isisRaw->i_inst));
  m_handle->openData("name");
  m_handle->putAttr("short_name",std::string(m_isisRaw->hdr.inst_abrv));
  m_handle->closeData();
  dae();
  detector_1();
  moderator();
  source();
  m_handle->closeGroup();
}

void SaveISISNexus::detector_1()
{
  m_handle->makeGroup("detector_1","NXdata", true);

  for(int i = 0; i < nmon; ++i)
  {
    int si = int(std::distance(m_isisRaw->spec,std::find(m_isisRaw->spec,m_isisRaw->spec+nsp,m_isisRaw->mdet[i])));
    monitor_index[si] = i;
  }

  // write counts
  std::vector<int64_t> dim(3);
  dim[0] = nper;
  dim[1] = nsp - nmon;
  dim[2] = ntc;
  m_handle->makeData("counts", ::NeXus::INT32, dim, true);
  m_handle->putAttr("units","counts");
  m_handle->putAttr("signal",1);
  m_handle->putAttr("axes","period_index,spectrum_index,time_of_flight");

  std::vector<int64_t> size(3,1);
  size[2] = ntc;
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
        std::vector<int64_t> start(3,0);
        start[0] = p;
        start[1] = ispec;
        m_handle->putSlab(m_isisRaw->dat1+1,start,size);
        ++ispec;
      }
      ++index;
    }
  }
  counts_link = m_handle->getDataID();
  m_handle->closeData();

  m_handle->makeLink(period_index_link);

  std::vector<int> spec_minus_monitors(nsp-nmon);
  std::generate(spec_minus_monitors.begin(),spec_minus_monitors.end(),getWithoutMonitors<int>(this,m_isisRaw->spec));
  m_handle->writeData("spectrum_index",spec_minus_monitors);
  m_handle->openData("spectrum_index");
  spectrum_index_link = m_handle->getDataID();
  m_handle->closeData();

  m_handle->makeLink(time_of_flight_link);
  m_handle->makeLink(time_of_flight_raw_link);

  std::vector<float> float_vec(ndet-nmon);
  std::generate(float_vec.begin(),float_vec.end(),getWithoutMonitors<float>(this,m_isisRaw->delt));
  m_handle->writeData("delt",float_vec);

  m_handle->writeData("source_detector_distance",m_isisRaw->ivpb.i_sddist);

  // using the same float_vec, size unchanged ndet-nmon
  std::generate(float_vec.begin(),float_vec.end(),getWithoutMonitors<float>(this,m_isisRaw->len2));
  m_handle->writeData("distance",float_vec);
  m_handle->openData("distance");
  m_handle->putAttr("units","metre");
  m_handle->closeData();

  // using the same float_vec, size unchanged ndet-nmon
  std::generate(float_vec.begin(),float_vec.end(),getWithoutMonitors<float>(this,m_isisRaw->tthe));
  m_handle->writeData("polar_angle",float_vec);
  m_handle->openData("polar_angle");
  m_handle->putAttr("units","degree");
  m_handle->closeData();

  m_handle->closeGroup();
}

/**
  * Write instrument/moderator group
  */
void SaveISISNexus::moderator()
{
  m_handle->makeGroup("moderator","NXmoderator", true);

  float l1 = - m_isisRaw->ivpb.i_l1;
  m_handle->writeData("distance",l1);
  m_handle->openData("distance");
  m_handle->putAttr("units","metre");

  m_handle->closeGroup();
}

/**
  * Write instrument/source group
  */
void SaveISISNexus::source()
{
  m_handle->makeGroup("source","NXsource",true);

  m_handle->writeData("name","ISIS");
  m_handle->writeData("probe","neutrons");
  m_handle->writeData("type","Pulsed Neutron Source");

  m_handle->closeGroup();
}

/**
  * Create group "detector_1" at NXentry level and link to some of the data in instrument/detector_1
  */
void SaveISISNexus::make_detector_1_link()
{
  m_handle->makeGroup("detector_1","NXdata", true);

  m_handle->makeLink(counts_link);
  m_handle->makeLink(period_index_link);
  m_handle->makeLink(spectrum_index_link);
  m_handle->makeLink(time_of_flight_link);

  m_handle->closeGroup();
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
  std::vector<int64_t> dim(3,1);
  dim[0] = nper;
  dim[2] = ntc;
  std::vector<int64_t> size(3,1);
  size[2] = ntc;
  std::ostringstream ostr;
  int mon_num = i + 1;
  ostr << "monitor_" << mon_num;
  m_handle->makeGroup(ostr.str(),"NXmonitor", true);
  
//  int imon = m_isisRaw->mdet[i]; // spectrum index
  m_handle->makeData("data", ::NeXus::INT32, dim, true);
  for(int p = 0; p < nper; ++p)
  {
    std::vector<int64_t> start(3,0);
    start[0] = p;
    m_handle->putSlab(getMonitorData(p,i),start,size);
  }
  m_handle->putAttr("units","counts");
  m_handle->putAttr("signal",1);
  m_handle->putAttr("axes","period_index,spectrum_index,time_of_flight");
  m_handle->closeData();

  m_handle->writeData("monitor_number",mon_num);
  m_handle->makeLink(period_index_link);
  m_handle->writeData("spectrum_index",m_isisRaw->mdet[i]);
  m_handle->makeLink(time_of_flight_link);

  m_handle->closeGroup();
}

void SaveISISNexus::dae()
{
  m_handle->makeGroup("dae","IXdae", true);
  
  m_handle->writeData("detector_table_file"," ");
  m_handle->writeData("spectra_table_file"," ");
  m_handle->writeData("wiring_table_file"," ");

  m_handle->writeData("period_index",std::vector<int>(m_isisRaw->t_pmap,m_isisRaw->t_pmap+nper));
  m_handle->openData("period_index");
  period_index_link =  m_handle->getDataID();
  m_handle->closeData();

  m_handle->makeGroup("time_channels_1","IXtime_channels",true);

  boost::scoped_array<float> timeChannels(new float[ntc+1]);
  m_isisRaw->getTimeChannels(timeChannels.get(),ntc+1);
  m_handle->writeData("time_of_flight",std::vector<float>(timeChannels.get(),timeChannels.get()+ntc+1));
  m_handle->openData("time_of_flight");
  m_handle->putAttr("axis",1);
  m_handle->putAttr("primary",1);
  m_handle->putAttr("units","microseconds");
  time_of_flight_link = m_handle->getDataID();
  m_handle->closeData();

  m_handle->writeData("time_of_flight_raw",std::vector<int>(m_isisRaw->t_tcb1,m_isisRaw->t_tcb1+ntc+1));
  m_handle->openData("time_of_flight_raw");
  m_handle->putAttr("units","pulses");
  m_handle->putAttr("frequency","32 MHz");
  time_of_flight_raw_link = m_handle->getDataID();
  m_handle->closeData();

  m_handle->closeGroup();   // time_channels_1

  m_handle->closeGroup();   // dae
}

void SaveISISNexus::user()
{
  m_handle->makeGroup("user_1","NXuser",true);

  m_handle->writeData("name",std::string(m_isisRaw->user.r_user));
  m_handle->writeData("affiliation",std::string(m_isisRaw->user.r_instit));
  
  m_handle->closeGroup();   // user_1
}

void SaveISISNexus::sample()
{
  m_handle->makeGroup("sample","NXsample",true);

  m_handle->writeData("name",std::string(m_isisRaw->spb.e_name));
  m_handle->writeData("height",m_isisRaw->spb.e_height);
  m_handle->writeData("width",m_isisRaw->spb.e_width);
  m_handle->writeData("thickness",m_isisRaw->spb.e_thick);
  m_handle->writeData("id"," ");
  m_handle->writeData("distance",float(0.0));
  std::string shape[] = {"cylinder","flat plate","HRPD slab","unknown"};
  int i = m_isisRaw->spb.e_geom - 1;
  if (i < 0 || i > 3) i = 3;
  m_handle->writeData("shape",shape[i]);
  std::string type[] = {"sample+can","empty can","vanadium","absorber","nothing","sample, no can","unknown"};
  i = m_isisRaw->spb.e_type - 1;
  if (i < 0 || i > 6) i = 6;
  m_handle->writeData("type",type[i]);
  
  m_handle->closeGroup();   // sample
}

/**
  * Create and write run logs form \<RawFilename\>_ICPstatus.txt log file
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

  m_handle->makeGroup("runlog","IXrunlog",true);

  int time_vec_size = static_cast<int>(time_vec.size());

  write_runlog("period",&time_vec[0],&period_vec[0],NX_INT32,time_vec_size,"none");
  write_runlog("is_running",&time_vec[0],&is_running_vec[0],NX_INT32,time_vec_size,"none");
  write_runlog("is_waiting",&time_vec[0],&is_waiting_vec[0],NX_INT32,time_vec_size,"none");
  write_runlog("good_frames",&time_vec[0],&good_frames_vec[0],NX_INT32,time_vec_size,"frames");
  write_runlog("raw_frames",&time_vec[0],&raw_frames_vec[0],NX_INT32,time_vec_size,"frames");
  write_runlog("monitor_sum_1",&time_vec[0],&monitor_sum_1_vec[0],NX_INT32,time_vec_size,"counts");
  write_runlog("total_counts",&time_vec[0],&total_counts_vec[0],NX_INT32,time_vec_size,"counts");
  write_runlog("proton_charge",&time_vec[0],&proton_charge_vec[0],NX_FLOAT32,time_vec_size,"uAh");
  write_runlog("proton_charge_raw",&time_vec[0],&proton_charge_raw_vec[0],NX_FLOAT32,time_vec_size,"uAh");
  write_runlog("dae_beam_current",&time_vec[0],&dae_beam_current_vec[0],NX_FLOAT32,time_vec_size,"uAh");
  write_runlog("count_rate",&time_vec[0],&count_rate_vec[0],NX_FLOAT32,time_vec_size,"counts");
  write_runlog("np_ratio",&time_vec[0],&np_ratio_vec[0],NX_FLOAT32,time_vec_size,"nones");

  write_runlog("run_status",&time_vec[0],&run_status_vec[0],NX_INT32,time_vec_size,"none");

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

  m_handle->makeGroup("icp_event","NXlog", true);

  m_handle->writeData("time",time_vec);
  m_handle->openData("time");
  m_handle->putAttr("start",start_time_str);
  m_handle->putAttr("units","seconds");
  m_handle->closeData();

  saveStringVectorOpen("value",event_vec,72);
  m_handle->putAttr("units"," ");
  m_handle->closeData();
  m_handle->closeGroup();   // icp_event

  m_handle->closeGroup(); // runlog
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
  m_handle->closeGroup();
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
  m_handle->makeGroup(name,"NXlog",true);
  std::vector<int64_t> dims(1,size);

  m_handle->makeData("time", ::NeXus::FLOAT32, dims, true);
  m_handle->putData(times);
  m_handle->putAttr("start",start_time_str);
  m_handle->putAttr("units","seconds");
  m_handle->closeData();

  if (type == NX_INT32)
  {
    m_handle->makeData("value", ::NeXus::INT32, dims, true);
  }
  else if (type == NX_FLOAT32)
  {
    m_handle->makeData("value", ::NeXus::FLOAT32, dims, true);
  }
  m_handle->putData(data);
  m_handle->putAttr("units",units);
  m_handle->closeData();
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
  boost::regex regex(l_filenamePart + "_.*\\.txt", boost::regex_constants::icase);
  Poco::DirectoryIterator end_iter;
  for ( Poco::DirectoryIterator dir_itr(Poco::Path(inputFilename).parent()); dir_itr != end_iter; ++dir_itr )
  {
    if ( !Poco::File(dir_itr->path() ).isFile() ) continue;

    l_filenamePart = Poco::Path(dir_itr->path()).getFileName();

    if ( boost::regex_match(l_filenamePart, regex) )
    {
      potentialLogFiles.push_back( dir_itr->path() );
    }
  }

  Progress prog(this,0.5,1,potentialLogFiles.size());

  m_handle->makeGroup("selog","IXselog",true);

  // create a log for each of the found log files
  std::size_t nBase = base_name.size() + 1;
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
    m_handle->makeGroup(&logName[0],"IXseblock", true);

    {
    m_handle->writeData("vi_name"," ");
    m_handle->writeData("set_control"," ");
    m_handle->writeData("read_control"," ");
    m_handle->writeData("setpoint", float(0.));
    m_handle->openData("setpoint");
    m_handle->putAttr("units","mV");
    m_handle->closeData();
    }

    m_handle->makeGroup("value_log","NXlog",true);

    m_handle->writeData("time",time_vec);
    m_handle->openData("time");
    m_handle->putAttr("start",start_time_str);
    m_handle->putAttr("units","seconds");
    m_handle->closeData();

    if (flt_vec.size() == str_vec.size())
    {
      m_handle->writeData("value",flt_vec);
      m_handle->openData("value");
    }
    else
    {
      saveStringVectorOpen("value",str_vec);
    }
    m_handle->putAttr("units"," ");
    m_handle->closeData();

    m_handle->writeData("name"," ");

    m_handle->closeGroup(); // value_log

    m_handle->closeGroup(); // logName

    prog.report();
  }

  m_handle->closeGroup(); // selog

  progress(1);
}

void SaveISISNexus::logNotes()
{
  saveStringVectorOpen("notes",log_notes);
  m_handle->closeData();
}

void SaveISISNexus::run_cycle()
{
  m_handle->writeData("run_cycle"," ");
}

void SaveISISNexus::write_rpb()
{
  std::vector<int64_t> dim(2);
  dim[0] = 32;
  dim[1] = 4;
  m_handle->makeData("CRPB", ::NeXus::CHAR,dim, true);
  m_handle->putData(&m_isisRaw->rpb);
  m_handle->closeData();

  // some nasty reinterpret of a struct as an integer array
  dim.pop_back();
  m_handle->makeData("IRPB", ::NeXus::INT32, dim, true);
  m_handle->putData((void*)&m_isisRaw->rpb);
  m_handle->closeData();

  // some nasty reinterpret of a struct as a float array
  m_handle->makeData("RRPB", ::NeXus::FLOAT32, dim, true);
  m_handle->putData((void*)&m_isisRaw->rpb);
  m_handle->closeData();

}

void SaveISISNexus::write_spb()
{
  std::vector<int64_t> dim(2);
  dim[0] = 64;
  dim[1] = 4;
  m_handle->makeData("CSPB",::NeXus::CHAR,dim, true);
  m_handle->putData(&m_isisRaw->spb);
  m_handle->closeData();

  // some nasty reinterpret of a struct as a float array
  dim.pop_back();
  m_handle->makeData("SPB",::NeXus::INT32, dim);
  m_handle->putData((void*)&m_isisRaw->spb);
  m_handle->closeData();

  // some nasty reinterpret of a struct as a float array
  m_handle->makeData("ISPB",::NeXus::INT32, dim);
  m_handle->putData((void*)&m_isisRaw->spb);
  m_handle->closeData();

  // some nasty reinterpret of a struct as a float array
  m_handle->makeData("RSPB",::NeXus::FLOAT32, dim);
  m_handle->putData((void*)&m_isisRaw->spb);
  m_handle->closeData();

}

void SaveISISNexus::write_vpb()
{
  std::vector<int64_t> dim(1,64);

  // some nasty reinterpret of a struct as a float array
  m_handle->makeData("IVPB", ::NeXus::INT32, dim, true);
  m_handle->putData((void*)&m_isisRaw->ivpb);
  m_handle->closeData();

  // some nasty reinterpret of a struct as a float array
  m_handle->makeData("RVPB", ::NeXus::FLOAT32, dim, true);
  m_handle->putData((void*)&m_isisRaw->ivpb);
  m_handle->closeData();
}

} // namespace NeXus
} // namespace Mantid
