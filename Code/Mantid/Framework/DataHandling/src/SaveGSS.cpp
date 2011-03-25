//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidDataHandling/SaveGSS.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceValidators.h"
//#include "MantidGeometry/IInstrument.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include <boost/math/special_functions/fpclassify.hpp>
#include <Poco/File.h>
#include <fstream>
#include <iomanip>

namespace Mantid
{
namespace DataHandling
{

using namespace API;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveGSS)

/// Sets documentation strings for this algorithm
void SaveGSS::initDocs()
{
  this->setWikiSummary("Saves a focused data set into a three column GSAS format. ");
  this->setOptionalMessage("Saves a focused data set into a three column GSAS format.");
}


const std::string RALF("RALF");
const std::string SLOG("SLOG");

//---------------------------------------------------
// Private member functions
//---------------------------------------------------
/**
 * Initialise the algorithm
 */
void SaveGSS::init()
{
  // Data must be in TOF
  declareProperty(new API::WorkspaceProperty<>("InputWorkspace", "", Kernel::Direction::Input,
    new API::WorkspaceUnitValidator<>("TOF")),
    "The input workspace, which must be in time-of-flight");
  declareProperty(new API::FileProperty("Filename", "", API::FileProperty::Save),
    "The filename to use for the saved data");
  std::vector<std::string> Split(2);
  Split[0] = "True";
  Split[1] = "False";
  declareProperty("SplitFiles", "True", new Kernel::ListValidator(Split),
    "Save each spectrum in a different file (default true)" );
  declareProperty("Append",true,"If true and Filename already exists, append, else overwrite");
  declareProperty("Bank",1, "Start bank (spectrum) numbers at this number in the file. The bank number in the file will be the workspace index + StartAtBankNumber. Default = 1.");
  std::vector<std::string> formats;
  formats.push_back(RALF);
  formats.push_back(SLOG);
  declareProperty("Format", RALF, new Kernel::ListValidator(formats), "GSAS format to save as");
  declareProperty("MultiplyByBinWidth", true, "Multiply the intensity (Y) by the bin width; default TRUE.");
}

/**
 * Determine the focused position for the supplied spectrum. The position
 * (l1, l2, tth) is returned via the references passed in.
 */
void getFocusedPos(MatrixWorkspace_const_sptr wksp, const size_t spectrum, double &l1, double &l2, double &tth) {
  Geometry::IInstrument_const_sptr instrument = wksp->getInstrument();
  if (instrument == NULL)
  {
    l1 = 0.;
    l2 = 0.;
    tth = 0.;
    return;
  }
  Geometry::IObjComponent_const_sptr source = instrument->getSource();
  Geometry::IObjComponent_const_sptr sample = instrument->getSample();
  if ( source == NULL || sample == NULL )
  {
    l1 = 0.;
    l2 = 0.;
    tth = 0.;
    return;
  }
  l1 = source->getDistance(*sample);
  Geometry::IDetector_sptr det = wksp->getDetector(spectrum);
  l2 = det->getDistance(*sample);
  tth = wksp->detectorTwoTheta(det);
}

/**
 * Execute the algorithm
 */
void SaveGSS::exec()
{
  using namespace Mantid::API;
  //Retrieve the input workspace
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  const int nHist=inputWS->getNumberHistograms();

  std::string filename = getProperty("Filename");
  std::string inputWSName = getProperty("InputWorkspace");
 
  std::size_t pos=filename.find_first_of(".");
  std::string ext;
  if (pos!=std::string::npos) //Remove the extension
  {
    ext=filename.substr(pos+1,filename.npos);
    filename=filename.substr(0,pos);
  }

  const int bank=getProperty("Bank");
  const bool MultiplyByBinWidth = getProperty("MultiplyByBinWidth");
  std::string split_string = getProperty("SplitFiles");
  bool split = (split_string == "True");
  std::string outputFormat = getProperty("Format");

  std::ostringstream number;
  std::ofstream out;
  // Check whether to append to an already existing file or overwrite
  const bool append = getProperty("Append");
  using std::ios_base;
  ios_base::openmode mode = ( append ? (ios_base::out | ios_base::app) : ios_base::out );
  Progress p(this,0.0,1.0,nHist);
  double l1, l2, tth;
  Geometry::IInstrument_const_sptr instrument = inputWS->getInstrument();
  Geometry::IObjComponent_const_sptr source;
  Geometry::IObjComponent_const_sptr sample;
  if (instrument != NULL) 
  {
    source = instrument->getSource();
    sample = instrument->getSample();
  }
  for (int i=0;i<nHist;i++)
  {
    if (instrument != NULL) 
    {
      if ( source != NULL && sample != NULL ) 
      {
        Geometry::IDetector_const_sptr det = inputWS->getDetector(static_cast<size_t>(i));
        if ( det->isMasked() ) continue;
      }
    }
    getFocusedPos(inputWS, i, l1, l2, tth);
    if (!split && i==0) // Assign only one file
    {
      const std::string file(filename+'.'+ext);
      Poco::File fileobj(file);
      const bool exists = fileobj.exists();
      out.open(file.c_str(),mode);
      if ( !exists || !append )  writeHeaders(outputFormat, out, inputWS);

    }
    else if (split)//Several files will be created with names: filename-i.ext
    {
      number << "-" << i;
      const std::string file(filename+number.str()+"."+ext);
      Poco::File fileobj(file);
      const bool exists = fileobj.exists();
      out.open(file.c_str(),mode);
      number.str("");
      if ( !exists || !append ) writeHeaders(outputFormat, out, inputWS);
    }

    { // New scope
      if (!out.is_open())
      {
        g_log.information("Could not open filename: "+filename);
        throw std::runtime_error("Could not open filename: "+filename);
      }

      if (l1 != 0. || l2 != 0. || tth != 0.)
        out << "# Total flight path " << (l1+l2) << "m, tth " << (tth*180./M_PI)
            << "deg, DIFC "
            << (( 2.0 * PhysicalConstants::NeutronMass * sin(tth/2.0) * ( l1 + l2 ) )
            / (PhysicalConstants::h * 1e4)) << "\n";
      out << "# Data for spectrum :"<< i << std::endl;
      if (RALF.compare(outputFormat) == 0) {
        this->writeRALFdata(bank+i, MultiplyByBinWidth, out,
                            inputWS->readX(i), inputWS->readY(i), inputWS->readE(i));
      }
      else if (SLOG.compare(outputFormat) == 0) {
        this->writeSLOGdata(bank+i, MultiplyByBinWidth, out,
                            inputWS->readX(i), inputWS->readY(i), inputWS->readE(i));
      }
      else {
        throw std::runtime_error("Do not know how to write output format " + outputFormat);
      }

    } // End separate scope

    //Close at each iteration
    if (split)
    {
      out.close();
    }
    p.report();
  }
  // Close if single file
  if (!split)
  {
    out.close();
  }
  return;
}

/** Ensures that when a workspace group is passed as output to this workspace
 *  everything is saved to one file and the bank number increments for each
 *  group member.
 *  @param alg ::           Pointer to the algorithm
 *  @param propertyName ::  Name of the property
 *  @param propertyValue :: Value  of the property
 *  @param periodNum ::     Effectively a counter through the group members
 */
void SaveGSS::setOtherProperties(IAlgorithm* alg,const std::string& propertyName,const std::string& propertyValue,int periodNum)
{
  // We want to append subsequent group members to the first one
  if( propertyName == "Append")
  {
    if(periodNum!=1)
    {
      alg->setPropertyValue(propertyName,"1");
    }
    else alg->setPropertyValue(propertyName,propertyValue);
  }
  // We want the bank number to increment for each member of the group
  else if ( propertyName == "Bank" )
  {
    alg->setProperty("Bank",atoi(propertyValue.c_str())+periodNum-1);
  }
  else Algorithm::setOtherProperties(alg,propertyName,propertyValue,periodNum);
}

void writeValue(std::ostream &os, const Run& runinfo, const std::string& name) {
  if (!runinfo.hasProperty(name)) {
    os << "UNKNOWN";
    return;
  }
  Kernel::Property* prop = runinfo.getProperty(name);
  if (prop == NULL) {
    os << "UNKNOWN";
    return;
  }
  Kernel::TimeSeriesProperty<double> *log = dynamic_cast<Kernel::TimeSeriesProperty<double> *> (prop);
  if (log != NULL)
  {
    os << log->getStatistics().mean;
  }
  else
  {
    os << prop->value();
  }
  std::string units = prop->units();
  if (!units.empty())
    os << " " << units;
}

/**
 * Write the header information for the given workspace
 * @param os :: The stream to use to write the information
 * @param workspace :: A shared pointer to MatrixWorkspace
 */
void SaveGSS::writeHeaders(const std::string &format, std::ostream& os, Mantid::API::MatrixWorkspace_const_sptr& workspace) const
{
  if (format.compare(SLOG) == 0) {
    const Run& runinfo = workspace->run();
    os << "# Sample Run: ";
    writeValue(os, runinfo, "run_number");
    os << " Vanadium Run: ";
    writeValue(os, runinfo, "van_number");
    os << " Wavelength: ";
    writeValue(os, runinfo, "LambdaRequest");
    os << "\n";
    os << "# Sample Temperature: ";
    writeValue(os, runinfo, "SampleTemp");
    os << " Freq: ";
    writeValue(os, runinfo, "SpeedRequest1");
    os << " Guide: ";
    writeValue(os, runinfo, "guide");
    os << "\n";

    // print whether it is normalized by monitor or pcharge
    bool norm_by_current = false;
    bool norm_by_monitor = false;
    const std::vector<AlgorithmHistory>& algohist = workspace->getHistory().getAlgorithmHistories();
    for (std::vector<AlgorithmHistory>::const_iterator it = algohist.begin(); it != algohist.end();
         ++it) {
      if (it->name().compare("NormaliseByCurrent") == 0)
        norm_by_current = true;
      if (it->name().compare("NormaliseToMonitor") == 0)
        norm_by_monitor = true;
    }
    os << "#";
    if (norm_by_current)
      os << " Normalised to pCharge";
    if (norm_by_monitor)
      os << " Normalised to monitor";
    os << "\n";
  }
  os << workspace->getTitle() << "\n";
  os <<"# " << workspace->getNumberHistograms() << " Histograms\n";
  os <<"# File generated by Mantid:\n";
  os <<"# Instrument: " << workspace->getBaseInstrument()->getName() << "\n";
  os <<"# From workspace named : " << workspace->getName();
  if (getProperty("MultiplyByBinWidth"))
    os << ", with Y multiplied by the bin widths.";
  os << "\n";

  return;
}

inline void writeBankLine(std::ostream& out, const std::string& bintype,
                          const int banknum, const size_t datasize)
{
  out << "BANK "
    << std::fixed << std::setprecision(0) << banknum // First bank should be 1 for GSAS; this can be changed
    << std::fixed << " " << datasize
    << std::fixed << " " << datasize
    << std::fixed << " " << bintype;
}

inline double fixErrorValue(const double value) 
{
  if(value<0 || boost::math::isnan(value) || boost::math::isinf(value)) //Negative errors cannot be read by GSAS
    return 0.;
  else
    return value;
}

void SaveGSS::writeRALFdata(const int bank, const bool MultiplyByBinWidth, std::ostream& out,
                            const MantidVec& X, const MantidVec& Y, const MantidVec& E) const 
{
  const size_t datasize = Y.size();
  double bc1=X[0]*32;
  double bc2=(X[1]-X[0])*32;
  // Logarithmic step
  double bc4=(X[1]-X[0])/X[0];
  if(boost::math::isnan(fabs(bc4)) || boost::math::isinf(bc4)) bc4=0; //If X is zero for BANK

  //Write out the data header
  writeBankLine(out, "RALF", bank, datasize);
  out << std::fixed << " " << std::setprecision(0) << std::setw(8) << bc1
    << std::fixed << " " << std::setprecision(0) << std::setw(8) << bc2
    << std::fixed << " " << std::setprecision(0) << std::setw(8) << bc1
    << std::fixed << " " << std::setprecision(5) << std::setw(7) << bc4
    << " FXYE"<<std::endl;

  //Do each Y entry
  for (size_t j = 0; j < datasize; j++)
  {
    //Calculate the error
    double Epos;
    if (MultiplyByBinWidth)
      Epos = E[j] * (X[j+1] - X[j]); // E[j]*X[j]*bc4;
    else
      Epos = E[j];
    Epos = fixErrorValue(Epos);

    //The center of the X bin.
    out << std::fixed << std::setprecision(5) << std::setw(15) << 0.5*(X[j]+X[j+1]);

    // The Y value
    if (MultiplyByBinWidth)
      out << std::fixed << std::setprecision(8) << std::setw(18) << Y[j] * (X[j+1] - X[j]);
    else
      out << std::fixed << std::setprecision(8) << std::setw(18) << Y[j];

    //The error
    out << std::fixed << std::setprecision(8) << std::setw(18) << Epos << "\n";
  }
}

void SaveGSS::writeSLOGdata(const int bank, const bool MultiplyByBinWidth, std::ostream& out,
                            const MantidVec& X, const MantidVec& Y, const MantidVec& E) const 
{
  const size_t datasize = Y.size();
  double bc1 = *(X.begin()); // minimum TOF in microseconds
  if (bc1 <= 0.) {
    throw std::runtime_error("Cannot write out logorithmic data starting at zero");
  }
  double bc2 = *(X.rbegin()+1); // maximum TOF (in microseconds?)
  double bc3 = (*(X.begin()+1)-bc1)/bc1; // deltaT/T

  writeBankLine(out, "SLOG", bank, datasize);
  out << std::fixed << " " << std::setprecision(0) << std::setw(10) << bc1
      << std::fixed << " " << std::setprecision(0) << std::setw(10) << bc2
      << std::fixed << " " << std::setprecision(7) << std::setw(10) << bc3
      << std::fixed << " 0 FXYE" << std::endl;

  double delta, y,e;

  for (size_t i = 0; i < datasize; i++) {
    y = Y[i];
    e = E[i];
    if (MultiplyByBinWidth) {
      delta = X[i+1] - X[i];
      y *= delta;
      e *= delta;
    }
    e = fixErrorValue(e);

    out << "  " << std::fixed << std::setprecision(9) << std::setw(20) << 0.5 * (X[i] +X[i+1])
        << "  " << std::fixed << std::setprecision(9) << std::setw(20) << y
        << "  " << std::fixed << std::setprecision(9) << std::setw(20) << e
        << std::setw(12) << " " << "\n"; // let it flush its own buffer
  }
  out << std::flush;
}

} // namespace DataHandling
} // namespace Mantid
