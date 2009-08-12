//----------------------------
// Includes
//----------------------------
#include "MantidDataHandling/RawFileInfo.h"
#include "LoadRaw/isisraw2.h"
#include "MantidAPI/TableRow.h"

// Register the algorithm into the AlgorithmFactory
namespace Mantid
{
namespace DataHandling
{
  DECLARE_ALGORITHM(RawFileInfo)
}
}

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataHandling;

void RawFileInfo::init()
{
  
  std::vector<std::string> exts(2, "raw");
  exts[1] = "s*";
  declareProperty("Filename", "", new FileValidator(exts), 
		  "The RAW file from which to extract the information");
  declareProperty("RunTitle", std::string(""), "The title of the run", Direction::Output);
  declareProperty("SpectraCount", -1, "The number of spectra", Direction::Output);
  declareProperty("TimeChannelCount", -1, "The number of time channels in regime 1 ", Direction::Output);
  declareProperty("PeriodCount", -1, "The number of periods", Direction::Output);
  declareProperty("GetRunParameters", false, "Create a table workspace with each column having the title\n"
		  "of a paramter in the RPB struct of the RAW file", Direction::Input);
}

void RawFileInfo::exec()
{
  std::string filename = getPropertyValue("Filename");
  FILE* file = fopen(filename.c_str(), "rb");
  if (file == NULL)
  {
    g_log.error("Unable to open file " + filename);
    throw Exception::FileError("Unable to open File:", filename);
  }

  ISISRAW2 *isis_raw = new ISISRAW2;
  isis_raw->ioRAW(file, true);
  
  //  First get the general information about the run
  std::string title(isis_raw->hdr.hd_run, 69);
  // Insert some spaces to tidy the string up a bit
  title.insert(5, " ");
  title.insert(26, " ");
  title.insert(51, " ");
  
  g_log.debug() << "Properties retrieved from " << filename << "\n"
		<< "\tRun title: " << title << "\n";
  setProperty("RunTitle", title);
  
  // Spectra count
  int num_holder = isis_raw->t_nsp1;
  setProperty("SpectraCount", num_holder);
  g_log.debug() << "\tNumber of spectra:  " << num_holder << "\n";
   
  // Time channel count. Note here that the raw file will say N time channels which transforms into
  // (N+1) bin boundaries in Mantid
  num_holder = isis_raw->t_ntc1;
  setProperty("TimeChannelCount", num_holder);
  g_log.debug() << "\tNumber of time channels:  " << num_holder << "\n";
  // The number of periods
  num_holder = isis_raw->t_nper;
  setProperty("PeriodCount", num_holder);
  g_log.debug() << "\tNumber of periods:  " << num_holder << "\n";

  // Get the run information if we are told to
  bool get_run_info = getProperty("GetRunParameters");
  if( get_run_info )
  {
    declareProperty(new WorkspaceProperty<API::ITableWorkspace>("RunParameterTable","Raw_RPB",Direction::Output),
		    "The name of the TableWorkspace in which to store the list of run parameters" );

    API::ITableWorkspace_sptr run_table = WorkspaceFactory::Instance().createTable("TableWorkspace");
    run_table->addColumn("int", "r_dur");	// actual run duration
    run_table->addColumn("int", "r_durunits");	// scaler for above (1=seconds)
    run_table->addColumn("int", "r_dur_freq");  // testinterval for above (seconds)
    run_table->addColumn("int", "r_dmp");       // dump interval
    run_table->addColumn("int", "r_dmp_units");	// scaler for above
    run_table->addColumn("int", "r_dmp_freq");	// interval for above
    run_table->addColumn("int", "r_freq");	// 2**k where source frequency = 50 / 2**k
    run_table->addColumn("double", "r_gd_prtn_chrg");  // good proton charge (uA.hour)
    run_table->addColumn("double", "r_tot_prtn_chrg"); // total proton charge (uA.hour)
    run_table->addColumn("int", "r_goodfrm");	// good frames
    run_table->addColumn("int", "r_rawfrm");	// raw frames
    run_table->addColumn("int", "r_dur_wanted"); // requested run duration (units as for "duration" above)
    run_table->addColumn("int", "r_dur_secs");	// actual run duration in seconds
    run_table->addColumn("int", "r_mon_sum1");	// monitor sum 1
    run_table->addColumn("int", "r_mon_sum2");	// monitor sum 2
    run_table->addColumn("int", "r_mon_sum3");	// monitor sum 3
    run_table->addColumn("str", "r_enddate"); // format DD-MMM-YYYY
    run_table->addColumn("str", "r_endtime"); // format HH-MM-SS
    run_table->addColumn("int", "r_prop"); // RB (proposal) number
  
    API::TableRow t = run_table->appendRow();
    t << isis_raw->rpb.r_dur << isis_raw->rpb.r_durunits << isis_raw->rpb.r_dur_freq << isis_raw->rpb.r_dmp
      << isis_raw->rpb.r_dmp_units << isis_raw->rpb.r_dmp_freq << isis_raw->rpb.r_freq 
      << static_cast<double>(isis_raw->rpb.r_gd_prtn_chrg) << static_cast<double>(isis_raw->rpb.r_tot_prtn_chrg)
      << isis_raw->rpb.r_goodfrm << isis_raw->rpb.r_rawfrm << isis_raw->rpb.r_dur_wanted
      << isis_raw->rpb.r_dur_secs << isis_raw->rpb.r_mon_sum1 << isis_raw->rpb.r_mon_sum2 << isis_raw->rpb.r_mon_sum3
      << std::string(isis_raw->rpb.r_enddate, 11) << std::string(isis_raw->rpb.r_endtime, 8) << isis_raw->rpb.r_prop;
    
    setProperty("RunParameterTable", run_table);
  }
  
  // This is not going to be a slow algorithm
  progress(1.0);
  // Free up memory
  delete isis_raw;
}

