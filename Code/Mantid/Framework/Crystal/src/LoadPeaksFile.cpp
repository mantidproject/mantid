#include "MantidAPI/FileProperty.h"
#include "MantidCrystal/LoadPeaksFile.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/System.h"
#include <algorithm>
#include <boost/shared_ptr.hpp>
#include <exception>
#include <fstream>
#include <iostream>
#include <math.h>
#include <ostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>

namespace Mantid
{
namespace Crystal
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(LoadPeaksFile)
  
  using namespace Mantid::Kernel;
  using namespace Mantid::API;
  using namespace Mantid::DataObjects;
  using namespace Mantid::Geometry;


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  LoadPeaksFile::LoadPeaksFile()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  LoadPeaksFile::~LoadPeaksFile()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void LoadPeaksFile::initDocs()
  {
    this->setWikiSummary("Load an ISAW-style .peaks file into a [[PeaksWorkspace]].");
    this->setOptionalMessage("Load an ISAW-style .peaks file into a PeaksWorkspace.");
    this->setWikiDescription(
        "Reads an ISAW-style .peaks or .integrate file into a PeaksWorkspace. Any detector calibration information is ignored."
        "\n\n"
        "NOTE: The instrument used is determined by reading the 'Instrument:' and 'Date:' tags at the start of the file."
        "If the date is not present, the latest [[Instrument Definition File]] is used."
        );

  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void LoadPeaksFile::init()
  {
    std::vector<std::string> exts;
    exts.push_back(".peaks");
    exts.push_back(".integrate");

    declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
        "Path to an ISAW-style .peaks filename.");
    declareProperty(new WorkspaceProperty<PeaksWorkspace>("OutputWorkspace","",Direction::Output), "Name of the output workspace.");
  }




  //-----------------------------------------------------------------------------------------------
  /** Get a word from a line and strips spaces */
  std::string getWord( std::ifstream &in ,  bool consumeEOL )
  {
    std::string s;
    char c = 0;
    if( in.good() )
      for(  c = in.get() ; c == ' ' && in.good() ; c = in.get() )
      {}
    else
      return std::string();

    if( c == '\n' )
    {
      if( !consumeEOL )
        in.putback( c );

      return std::string();
    }

    s.push_back( c );

    if( in.good() )
      for(  c = in.get() ; in.good()&&c != ' ' && c != '\n' && c != '\r' ; c = in.get() )
        s.push_back( c );

    if( ((c == '\n') || (c == '\r')) && !consumeEOL )
      in.putback( c );

    return s;
  }

  //-----------------------------------------------------------------------------------------------
  /** Read up to the eol */
  void readToEndOfLine( std::ifstream& in ,  bool ConsumeEOL )
  {
    while( in.good() && getWord( in ,  false ).length() > 0  )
      getWord( in ,  false );
    if( !ConsumeEOL )
      return ;
    getWord( in ,  true );
  }



  //-----------------------------------------------------------------------------------------------
  /** Reads the header of a .peaks file
   *
   * @param in :: stream of the input file
   * @return the first word on the next line
   */
  std::string LoadPeaksFile::readHeader( PeaksWorkspace_sptr outWS, std::ifstream& in )
  {
    std::string tag;
    std::string r = getWord( in ,  false );

    if( r.length() < 1 )
      throw std::logic_error( std::string( "No first line of Peaks file" ) );

    if( r.compare( std::string( "Version:" ) ) != 0 )
      throw std::logic_error(
          std::string( "No Version: on first line of Peaks file" ) );

    std::string C_version = getWord( in ,  false );
    if( C_version.length() < 1 )
      throw  std::logic_error( std::string( "No Version for Peaks file" ) );

    tag = getWord( in ,  false );
    std::string C_Facility = getWord( in ,  false );

    tag = getWord( in ,  false );
    std::string C_Instrument = getWord( in ,  false );

    if( C_Instrument.length() < 1 )
      throw std::logic_error(
          std::string( "No Instrument for Peaks file" ) );

    // Date: use the current date/time if not found
    Kernel::DateAndTime C_experimentDate;
    std::string date;
    tag = getWord( in ,  false );
    if(tag.empty())
      date = Kernel::DateAndTime::get_current_time().to_ISO8601_string();
    else if(tag == "Date:")
      date = getWord( in ,  false );
    readToEndOfLine( in ,  true );

    // Now we load the instrument using the name and date
    MatrixWorkspace_sptr tempWS = WorkspaceFactory::Instance().create("Workspace2D", 1, 1, 1);
    tempWS->mutableRun().addProperty<std::string>("run_start", date);

    IAlgorithm_sptr loadInst= createSubAlgorithm("LoadInstrument");
    loadInst->setPropertyValue("InstrumentName", C_Instrument);
    loadInst->setProperty<MatrixWorkspace_sptr> ("Workspace", tempWS);
    loadInst->executeAsSubAlg();

    // Populate the instrument parameters in this workspace - this works around a bug
    tempWS->populateInstrumentParameters();
    outWS->setInstrument( tempWS->getInstrument() );

    // Now skip all lines on L1, detector banks, etc. until we get to a block of peaks. They start with 0.
    readToEndOfLine( in ,  true );
    readToEndOfLine( in ,  true );
    std::string s = getWord(in, false);
    while (s != "0")
    {
      readToEndOfLine( in ,  true );
      s = getWord(in, false);
    }

    return s;
  }



  //-----------------------------------------------------------------------------------------------
  /** Read one peak in a line of an ISAW peaks file.
   *
   * @param outWS :: workspace to add peaks to
   * @param lastSt[in,out] :: last word (the one at the start of the line)
   * @param in :: input stream
   * @param[out] seqNum :: the sequence number of the peak
   * @param bankName :: the bank number from the ISAW file.
   * @return the Peak the Peak object created
   */
  Mantid::DataObjects::Peak readPeak( PeaksWorkspace_sptr outWS, std::string & lastStr,  std::ifstream& in, int & seqNum, std::string bankName)
  {
    double h ; double k ;  double l ;  double col ;
    double row ; double chan ;  double L2 ;
    double  ScatAng ; double Az ;  double wl ;
    double D ;  double IPK ; double Inti ;
    double SigI ;  int iReflag;

    seqNum = -1;

    std::string s = lastStr;
    if( s.length() < 1 && in.good() )//blank line
    {
      readToEndOfLine( in ,  true );
      s = getWord( in ,  false );;
    }

    if( s.length() < 1 )
      throw std::runtime_error("Empty peak line encountered.");

    if( s.compare( "2" ) == 0 )
    {
      readToEndOfLine( in ,  true );
      for( s = getWord( in ,  false ) ; s.length() < 1 && in.good() ;
          s = getWord( in ,  true ) )
      {
        s = getWord( in ,  false );
      }
    }

    if( s.length() < 1 )
      throw std::runtime_error("Empty peak line encountered.");

    if( s.compare( "3" ) != 0 )
      throw std::runtime_error("Empty peak line encountered.");

    seqNum = atoi( getWord( in ,  false ).c_str() );

    h = strtod( getWord( in ,  false ).c_str() ,  0 ) ;
    k = strtod( getWord( in ,  false ).c_str() ,  0 ) ;
    l = strtod( getWord( in ,  false ).c_str() ,  0 ) ;

    col = strtod( getWord( in ,  false ).c_str() ,  0 ) ;
    row = strtod( getWord( in ,  false ).c_str() ,  0 ) ;
    chan = strtod( getWord( in ,  false ).c_str() ,  0 ) ;
    L2 = strtod( getWord( in ,  false ).c_str() ,  0 )/100 ;
    ScatAng = strtod( getWord( in ,  false ).c_str() ,  0 ) ;

    Az = strtod( getWord( in , false ).c_str() , 0 ) ;
    wl = strtod( getWord( in , false ).c_str() , 0 ) ;
    D = strtod( getWord( in , false ).c_str() , 0 ) ;
    IPK = strtod( getWord( in , false ).c_str() , 0 ) ;

    Inti = strtod( getWord( in , false ).c_str() , 0 ) ;
    SigI = strtod( getWord( in , false ).c_str() , 0 ) ;
    iReflag = atoi( getWord( in , false ).c_str() ) ;

    // Finish the line and get the first word of next line
    readToEndOfLine( in ,  true );
    lastStr = getWord( in , false );

    // Find the detector ID from row/col
    IInstrument_sptr inst = outWS->getInstrument();
    if (!inst) throw std::runtime_error("No instrument in PeaksWorkspace!");
    IComponent_sptr bank = inst->getComponentByName(bankName);
    if (!bank) throw std::runtime_error("Bank named " + bankName + " not found!");
    RectangularDetector_sptr rect = boost::dynamic_pointer_cast<RectangularDetector>(bank);
    if (!rect) throw std::runtime_error("Bank named " + bankName + " is not a RectangularDetector!");
    IDetector_sptr det = rect->getAtXY(int(col), int(row));
    if (!det) throw std::runtime_error("Detector not found on " + bankName + "!");

    //Create the peak object
    Peak peak(outWS->getInstrument(), det->getID(), wl);
    peak.setHKL(h,k,l);
    peak.setIntensity(Inti);
    peak.setSigmaIntensity(SigI);
    peak.setBinCount(IPK);

    // Return the peak
    return peak;
  }



  //-----------------------------------------------------------------------------------------------
  /** Read the header of each peak block section */
  std::string readPeakBlockHeader( std::string lastStr ,  std::ifstream &in ,
      int&run ,int& detName ,
      double&chi , double&phi ,
      double&omega , double&monCount )
  {
    std::string s = lastStr;

    if( s.length() < 1 && in.good() )//blank line
    {
      readToEndOfLine( in ,  true );
      s = getWord( in , false );
    }

    if( s.length() < 1 )
      return std::string();

    if( s.compare( "0" ) == 0 )
    {
      readToEndOfLine( in ,  true );
      s = getWord( in , false );
      while( s.length() < 1 )
      {
        readToEndOfLine( in ,  true );
        s = getWord( in , false );
      }
    }

    if( s.compare( std::string( "1" ) ) != 0 )
      return s;

    run = atoi( getWord( in , false ).c_str() );
    detName =atoi( getWord( in , false ).c_str());
    chi = strtod( getWord( in , false ).c_str() , 0 );
    phi = strtod( getWord( in , false ).c_str() , 0 );

    omega = strtod( getWord( in , false ).c_str() , 0 );
    monCount = strtod( getWord( in , false ).c_str() , 0 );
    readToEndOfLine( in ,  true );

    return getWord( in , false );

  }
  //special formatted file Use clear peaks to no append



  //-----------------------------------------------------------------------------------------------
  /** Append the peaks from a .peaks file into the workspace
   *
   * @param filename :: path to the .peaks file
   */
  void LoadPeaksFile::appendFile( PeaksWorkspace_sptr outWS, std::string filename)
  {

    // Open the file
    std::ifstream in( filename.c_str() );

    // Read the header, load the instrument
    std::string s = readHeader( outWS, in );

    if( !in.good() || s.length() < 1 )
      throw std::runtime_error( "End of Peaks file before peaks" );

    if( s.compare( std::string( "0" ) ) != 0 )
      throw std::logic_error( "No header for Peak segments"  );

    readToEndOfLine( in ,  true );
    s = getWord( in , false );

    int run, bankNum;
    double chi , phi , omega , monCount;

    // Build the universal goniometer that will build the rotation matrix.
    Mantid::Geometry::Goniometer uniGonio;
    uniGonio.pushAxis("phi",   0., 1., 0.,   0., Mantid::Geometry::CCW, Mantid::Geometry::angDegrees);
    uniGonio.pushAxis("chi",   1., 0., 0.,   0., Mantid::Geometry::CCW, Mantid::Geometry::angDegrees);
    uniGonio.pushAxis("omega", 0., 1., 0.,   0., Mantid::Geometry::CCW, Mantid::Geometry::angDegrees);

    while( in.good() )
    {
      // Read the header if necessary
      s = readPeakBlockHeader( s ,  in  , run , bankNum , chi , phi ,
          omega , monCount );

      std::ostringstream oss;
      oss << "bank" << bankNum;
      std::string bankName = oss.str();

      int seqNum = -1;

      try
      {
        // Read the peak
        Peak peak = readPeak(outWS, s, in, seqNum, bankName);

        // Build the Rotation matrix using phi,chi,omega
        uniGonio.setRotationAngle(0, phi);
        uniGonio.setRotationAngle(1, chi);
        uniGonio.setRotationAngle(2, omega);

        // Get the calculated goniometer matrix
        Matrix<double> gonMat = uniGonio.getR();

        peak.setGoniometerMatrix(gonMat);
        peak.setRunNumber(run);

        // Add the peak to workspace
        outWS->addPeak(peak);
      }
      catch (std::runtime_error & e)
      {
        g_log.warning() << "Error reading peak SEQN " << seqNum << " : " << e.what() << std::endl;
      }
    }
  }



  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void LoadPeaksFile::exec()
  {
    // Create the workspace
    PeaksWorkspace_sptr ws(new PeaksWorkspace());
    ws->setName(getPropertyValue("OutputWorkspace"));

    // This loads (appends) the peaks
    this->appendFile( ws, getPropertyValue("Filename") );

    // Save it in the output
    setProperty("OutputWorkspace", boost::dynamic_pointer_cast<PeaksWorkspace>(ws));
  }



} // namespace Mantid
} // namespace Crystal

