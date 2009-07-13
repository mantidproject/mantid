//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidDataHandling/SaveRKH.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "Poco/Timestamp.h"
#include "Poco/DateTimeFormatter.h"

#include <fstream>
#include <iomanip>

using namespace Mantid::DataHandling;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveRKH)

//---------------------------------------------------
// Private member functions
//---------------------------------------------------
/**
 * Initialise the algorithm
 */
void SaveRKH::init()
{
  declareProperty(
    new API::WorkspaceProperty<>("InputWorkspace", "", Kernel::Direction::Input),
    "The name of the workspace to save");
  declareProperty("Filename", "",
    "The name to use when saving the file");

  //Get the units registered with the UnitFactory
  std::vector<std::string> propOptions = Kernel::UnitFactory::Instance().getKeys();
  m_unitKeys.insert(propOptions.begin(), propOptions.end());
  //Add some others that will make this orient the other way
  m_RKHKeys.insert("SpectraNumber");
  propOptions.insert(propOptions.end(), m_RKHKeys.begin(), m_RKHKeys.end());
  declareProperty("FirstColumnValue", "Wavelength",
    new Kernel::ListValidator(propOptions),
    "The units of the first column (defualt Wavelengh)" );
 
}

/**
 * Execute the algorithm
 */
void SaveRKH::exec()
{
  using namespace Mantid::API;
  //Retrieve the input workspace
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");

  std::string filename = getProperty("Filename");
  std::ofstream outRKH(filename.c_str());

  if( !outRKH )
  {
    g_log.error() << "An error occurred while attempting to open the file " << filename << "\n";
    throw std::runtime_error("An error occurred while trying to open the output file for writing");
  }
  
  std::string firstColVal = getProperty("FirstColumnValue");
  bool isFactoryUnit(true);
  if( m_RKHKeys.find(firstColVal) != m_RKHKeys.end() ) isFactoryUnit = false;
  
  if( isFactoryUnit )
  {
    //Find the number of data points in the workspace
    int noDataPoints = (int)inputWS->dataY(0).size();
    int noHist = inputWS->getNumberHistograms();

    if( noHist != 1 )
    {
      g_log.error() << "The input workspace is incompatible with the SaveRKH algorithm and the selected FirstColumnValue.\n";
      throw std::runtime_error("The input workspace does not have the correct form to be saved in an RKH format");
    }
    outRKH <<  " LOQ ";
    Poco::Timestamp timestamp;
    //The sample file has the format of the data/time as in this example Thu 28-OCT-2004 12:23
    outRKH << Poco::DateTimeFormatter::format(timestamp, std::string("%w")) << " " << Poco::DateTimeFormatter::format(timestamp, std::string("%d")) 
           << "-";
    std::string month = Poco::DateTimeFormatter::format(timestamp, std::string("%b"));
    std::transform(month.begin(), month.end(), month.begin(), toupper);
    outRKH << month << "-" << Poco::DateTimeFormatter::format(timestamp, std::string("%Y %H:%M")) << " W 26  INST_DIRECT_BEAM\n"
           << " rwr50 standard, new mon2, 800v\n"
           << "  " << noDataPoints << " 0    0    0    1     " << noDataPoints << "    0\n"
           << "      0         0         0         0\n"
           << " 3 (F12.5,2E16.6)\n";

    //Now the data (The x data is bin edged so convert to bin centered
    const std::vector<double> & xdata = inputWS->dataX(0);
    const std::vector<double> & ydata = inputWS->dataY(0);
    const std::vector<double> & edata = inputWS->dataE(0);

    assert( (int)xdata.size() == noDataPoints + 1);
    Progress prg(this,0.0,1.0,noDataPoints);
    for( int i = 0; i < noDataPoints; ++i )
    {
      outRKH << "     " << std::fixed << std::setprecision(5) << (xdata[i] + xdata[i + 1])/2.0 << "    " << std::scientific << std::setprecision(6) << ydata[i] << "    " 
              << std::scientific << std::setprecision(6) << edata[i] << "\n"; 
      prg.report();
    }
  }
  else
  {
    if( inputWS->dataY(0).size() != 1 )
    {
        g_log.error() << "The input workspace is incompatible with the SaveRKH algorithm and the selected FirstColumnValue.\n";
        throw std::runtime_error("The input workspace is incompatible with the SaveRKH algorithm and the selected FirstColumnValue.");
    };

    int noHist = inputWS->getNumberHistograms();
    outRKH <<  "LOQ ";
    Poco::Timestamp timestamp;
    //The sample file has the format of the data/time as in this example Thu 28-OCT-2004 12:23
    outRKH << Poco::DateTimeFormatter::format(timestamp, std::string("%w")) << " " << Poco::DateTimeFormatter::format(timestamp, std::string("%d")) 
           << "-";
    std::string month = Poco::DateTimeFormatter::format(timestamp, std::string("%b"));
    std::transform(month.begin(), month.end(), month.begin(), toupper);
    outRKH << month << "-" << Poco::DateTimeFormatter::format(timestamp, std::string("%Y %H:%M")) << " W  3  LOQ_DATA:?????.???\n"
           << "LOQ  Run no. ??????.???    Summed spectra ~15140/pixel\n"
           << noHist << " 0    0    0    0     " << noHist << "    0\n"
           << " 0         0         0         0\n"
           << " 3 (F12.5,2E16.6)\n";
    //The x data in this workspace is arbitrary, the file should have the spectrum number in the x column
	Progress prg(this,0.0,1.0,noHist);
   for( int index = 0; index < noHist; ++index )
   {
     const std::vector<double> & ydata = inputWS->dataY(index);
     const std::vector<double> & edata = inputWS->dataE(index);
     outRKH << "     " << std::fixed << std::setprecision(5) << (double)(index + 1) << "    " << std::scientific << std::setprecision(6) << ydata[0] << "    " 
              << std::scientific << std::setprecision(6) << edata[0] << "\n"; 
	 prg.report();
   }
  }
  
  outRKH.close();
}
