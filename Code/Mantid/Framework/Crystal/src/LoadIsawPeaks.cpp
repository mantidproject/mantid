/*WIKI* 



Reads an ISAW-style .peaks or .integrate file into a PeaksWorkspace. Any detector calibration information is ignored.

NOTE: The instrument used is determined by reading the 'Instrument:' and 'Date:' tags at the start of the file.If the date is not present, the latest [[Instrument Definition File]] is used.


*WIKI*/
#include "MantidAPI/FileProperty.h"
#include "MantidCrystal/LoadIsawPeaks.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/System.h"
#include "MantidCrystal/SCDCalibratePanels.h"
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
#include "MantidKernel/Unit.h"
#include "MantidAPI/LoadAlgorithmFactory.h"

using Mantid::Kernel::Strings::readToEndOfLine;
using Mantid::Kernel::Strings::getWord;
using Mantid::Kernel::Units::Wavelength;

namespace Mantid
{
namespace Crystal
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(LoadIsawPeaks)
  DECLARE_LOADALGORITHM(LoadIsawPeaks)
  
  using namespace Mantid::Kernel;
  using namespace Mantid::API;
  using namespace Mantid::DataObjects;
  using namespace Mantid::Geometry;

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  LoadIsawPeaks::LoadIsawPeaks()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  LoadIsawPeaks::~LoadIsawPeaks()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void LoadIsawPeaks::initDocs()
  {
    this->setWikiSummary("Load an ISAW-style .peaks file into a [[PeaksWorkspace]].");
    this->setOptionalMessage("Load an ISAW-style .peaks file into a PeaksWorkspace.");
   }
  //----------------------------------------------------------------------------------------------

 /// @copydoc Mantid::API::IDataFileChecker::quickFileCheck
  bool LoadIsawPeaks::quickFileCheck(const std::string& filePath,size_t nread,const file_header& header)
  {
    UNUSED_ARG(nread);
    UNUSED_ARG(header);

    std::string ext = this->extension(filePath);
    // If the extension is peaks or integrate then give it a go
    if( ext.compare("peaks") == 0 ) return true;
    else if( ext.compare("integrate") == 0 ) return true;
    else return false;
  }

  /// @copydoc Mantid::API::IDataFileChecker::fileCheck
  int LoadIsawPeaks::fileCheck(const std::string& filePath)
  {
      int confidence(0);
      try
      {
	    // Open the file
	    std::ifstream in( filePath.c_str() );
	    // Read the header, load the instrument
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

	    getWord( in ,  false ); //tag
	    // cppcheck-suppress unreadVariable
	    std::string C_Facility = getWord( in ,  false );

	    getWord( in ,  false ); //tag
	    std::string C_Instrument = getWord( in ,  false );

	    if( C_Instrument.length() < 1 )
	      throw std::logic_error(
	          std::string( "No Instrument for Peaks file" ) );

	    // Date: use the current date/time if not found
	    Kernel::DateAndTime C_experimentDate;
	    std::string date;
	    tag = getWord( in ,  false );
	    if(tag.empty())
	      date = Kernel::DateAndTime::getCurrentTime().toISO8601String();
	    else if(tag == "Date:")
	      date = getWord( in ,  false );
	    readToEndOfLine( in ,  true );
	    confidence = 95;
      }
      catch (std::exception & )
      {
      }
      return confidence;
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void LoadIsawPeaks::init()
  {
    std::vector<std::string> exts;
    exts.push_back(".peaks");
    exts.push_back(".integrate");

    declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
        "Path to an ISAW-style .peaks filename.");
    declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace","",Direction::Output), "Name of the output workspace.");
  }


  std::string  LoadIsawPeaks::ApplyCalibInfo(std::ifstream & in, std::string startChar,Geometry::Instrument_const_sptr instr_old, Geometry::Instrument_const_sptr instr,
      double &T0)
  {
    ParameterMap_sptr parMap1= instr_old->getParameterMap();

    ParameterMap_sptr parMap= instr->getParameterMap();


    while( in.good() && (startChar.size() <1 || startChar !="7") )
       {
         readToEndOfLine( in, true);
         startChar = getWord(in, false);
       }
     if( !(in.good()))
     {
       //g_log.error()<<"Peaks file has no time shift and L0 info"<<std::endl;
       throw std::invalid_argument("Peaks file has no time shift and L0 info");
     }
     std::string L1s= getWord(in,false);
     std::string T0s =getWord(in, false);
     if( L1s.length() < 1 || T0s.length() < 1)
     {
       g_log.error()<<"Missing L1 or Time offset"<<std::endl;
       throw std::invalid_argument("Missing L1 or Time offset");
     }
     double L1;
     try
     {
       std::istringstream iss( L1s+" "+T0s, std::istringstream::in);
       iss>>L1;
       iss>>T0;
       V3D sampPos=instr->getSample()->getPos();
       SCDCalibratePanels::FixUpSourceParameterMap(instr, L1/100, sampPos,parMap1);

     }catch(...)
     {
       g_log.error()<<"Invalid L1 or Time offset"<<std::endl;
       throw std::invalid_argument("Invalid L1 or Time offset");

     }

     readToEndOfLine( in, true);
     startChar = getWord(in , false);
     while( in.good() && (startChar.size() <1 || startChar !="5") )
            {
              readToEndOfLine( in, true);
              startChar = getWord(in, false);
            }

    if( !(in.good()))
    {
      g_log.error()<<"Peaks file has no detector panel info"<<std::endl;
      throw std::invalid_argument("Peaks file has no detector panel info");
    }


    while( startChar =="5")
    {

      std::string line;
      for( int i=0; i<16;i++)
      {
        std::string s= getWord(in, false);
        if( s.size() < 1)
        {
          g_log.error()<<"Not enough info to describe panel "<<std::endl;
          throw std::length_error("Not enough info to describe panel ");
        }
       line +=" "+s;;
      }

      readToEndOfLine(in, true);
      startChar = getWord( in, false);// blank lines ?? and # lines ignore

      std::istringstream iss( line, std::istringstream::in);
      int  bankNum,nrows,ncols;
      double width,height,depth,detD,Centx,Centy,Centz,Basex,Basey,Basez,
             Upx,Upy,Upz;
      try
      {
         iss>>bankNum>>nrows>>ncols>>width>>height>>depth>>detD
            >>Centx>>Centy>>Centz>>Basex>>Basey>>Basez
            >>Upx>>Upy>>Upz;
      }catch(...)
      {

        g_log.error()<<"incorrect type of data for panel "<<std::endl;
        throw std::length_error("incorrect type of data for panel ");
      }

      std::string SbankNum = boost::lexical_cast<std::string>(bankNum);

      std::string bankName = "bank"+SbankNum;
      boost::shared_ptr<const Geometry::IComponent> bank =instr_old->getComponentByName( bankName );

      if( !bank)
      {
        g_log.error()<<"There is no bank "<< bankName<<" in the instrument"<<std::endl;
        throw std::length_error("There is no bank "+ bankName+" in the instrument");
      }

      V3D dPos= V3D(Centx,Centy,Centz)/100.0- bank->getPos();
      V3D Base(Basex,Basey,Basez), Up(Upx,Upy,Upz);
      V3D ToSamp =Base.cross_prod(Up);
      Base.normalize();
      Up.normalize();
      ToSamp.normalize();
      Quat thisRot(Base,Up,ToSamp);
      Quat bankRot(bank->getRotation());
      bankRot.inverse();
      Quat dRot = thisRot*bankRot;

      boost::shared_ptr< const Geometry::RectangularDetector>bankR= boost::dynamic_pointer_cast
                         <const Geometry::RectangularDetector>( bank);

      double DetWScale = 1, DetHtScale = 1;
      if( bank)
      {
        DetWScale = width/bankR->xsize()/100;
        DetHtScale = height/bankR->ysize()/100;

      }
      std::vector<std::string> bankNames;
      bankNames.push_back(bankName);

      SCDCalibratePanels::FixUpBankParameterMap(bankNames,instr, dPos,
          dRot,DetWScale,DetHtScale , parMap1, false);

    }
    return startChar;
  }

  //-----------------------------------------------------------------------------------------------
  /** Reads the header of a .peaks file
   * @param outWS :: the workspace in which to place the information
   * @param in :: stream of the input file
   * @param T0 :: Time offset
   * @return the first word on the next line
   */
  std::string LoadIsawPeaks::readHeader( PeaksWorkspace_sptr outWS, std::ifstream& in, double &T0 )
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

    getWord( in ,  false ); //tag
    // cppcheck-suppress unreadVariable
    std::string C_Facility = getWord( in ,  false );

    getWord( in ,  false ); //tag
    std::string C_Instrument = getWord( in ,  false );

    if( C_Instrument.length() < 1 )
      throw std::logic_error(
          std::string( "No Instrument for Peaks file" ) );

    // Date: use the current date/time if not found
    Kernel::DateAndTime C_experimentDate;
    std::string date;
    tag = getWord( in ,  false );
    if(tag.empty())
      date = Kernel::DateAndTime::getCurrentTime().toISO8601String();
    else if(tag == "Date:")
      date = getWord( in ,  false );
    readToEndOfLine( in ,  true );

    // Now we load the instrument using the name and date
    MatrixWorkspace_sptr tempWS = WorkspaceFactory::Instance().create("Workspace2D", 1, 1, 1);
    tempWS->mutableRun().addProperty<std::string>("run_start", date);

    IAlgorithm_sptr loadInst= createChildAlgorithm("LoadInstrument");
    loadInst->setPropertyValue("InstrumentName", C_Instrument);
    loadInst->setProperty<MatrixWorkspace_sptr> ("Workspace", tempWS);
    loadInst->executeAsChildAlg();

    // Populate the instrument parameters in this workspace - this works around a bug
    tempWS->populateInstrumentParameters();
    Geometry::Instrument_const_sptr instr_old = tempWS->getInstrument() ;
    boost::shared_ptr< ParameterMap > map(new ParameterMap());
    Geometry::Instrument_const_sptr instr ( new Geometry::Instrument(instr_old->baseInstrument(), map ));

    //std::string s;
    std::string  s = ApplyCalibInfo(in, "", instr_old, instr, T0);
    outWS->setInstrument( instr);

    // Now skip all lines on L1, detector banks, etc. until we get to a block of peaks. They start with 0.
   // readToEndOfLine( in ,  true );
   // readToEndOfLine( in ,  true );
   // s = getWord(in, false);
    while (s != "0" && in.good())
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
   * @param lastStr [in,out] :: last word (the one at the start of the line)
   * @param in :: input stream
   * @param seqNum [out] :: the sequence number of the peak
   * @param bankName :: the bank number from the ISAW file.
   * @return the Peak the Peak object created
   */
  Mantid::DataObjects::Peak readPeak( PeaksWorkspace_sptr outWS, std::string & lastStr,  std::ifstream& in, int & seqNum, std::string bankName)
  {
    double h ; double k ;  double l ;  double col ;
    double row ; double wl ;
    double IPK ; double Inti ;
    double SigI ;

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
    strtod( getWord( in ,  false ).c_str() ,  0 ) ; //chan
    strtod( getWord( in ,  false ).c_str() ,  0 ) ; //L2
    strtod( getWord( in ,  false ).c_str() ,  0 ) ; //ScatAng

    strtod( getWord( in , false ).c_str() , 0 ) ; //Az
    wl = strtod( getWord( in , false ).c_str() , 0 ) ;
    strtod( getWord( in , false ).c_str() , 0 ) ; //D
    IPK = strtod( getWord( in , false ).c_str() , 0 ) ;

    Inti = strtod( getWord( in , false ).c_str() , 0 ) ;
    SigI = strtod( getWord( in , false ).c_str() , 0 ) ;
    atoi( getWord( in , false ).c_str() ) ; // iReflag

    // Finish the line and get the first word of next line
    readToEndOfLine( in ,  true );
    lastStr = getWord( in , false );

    // Find the detector ID from row/col
    Instrument_const_sptr inst = outWS->getInstrument();
    if (!inst) throw std::runtime_error("No instrument in PeaksWorkspace!");
    IComponent_const_sptr bank = inst->getComponentByName(bankName);
    if (!bank) throw std::runtime_error("Bank named " + bankName + " not found!");
    RectangularDetector_const_sptr rect = boost::dynamic_pointer_cast<const RectangularDetector>(bank);
    if (!rect) throw std::runtime_error("Bank named " + bankName + " is not a RectangularDetector!");
    IDetector_sptr det = rect->getAtXY(int(col), int(row));
    if (!det) throw std::runtime_error("Detector not found on " + bankName + "!");

    //Create the peak object
    Peak peak(outWS->getInstrument(), det->getID(), wl);
    // HKL's are flipped by -1 because of the internal Q convention
    peak.setHKL(-h,-k,-l);
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
   * @param outWS :: the workspace in which to place the information
   * @param filename :: path to the .peaks file
   */
  void LoadIsawPeaks::appendFile( PeaksWorkspace_sptr outWS, std::string filename )
  {

    // Open the file
    std::ifstream in( filename.c_str() );


    // Read the header, load the instrument
    double T0;
    std::string s = readHeader( outWS, in , T0);

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
    uniGonio.makeUniversalGoniometer();

    // TODO: Can we find the number of peaks to get better progress reporting?
    Progress prog(this, 0.0, 1.0, 100);

    while( in.good() )
    {
      // Read the header if necessary
      s = readPeakBlockHeader( s ,  in  , run , bankNum , chi , phi ,
          omega , monCount );
      // Build the Rotation matrix using phi,chi,omega
      uniGonio.setRotationAngle("phi", phi);
      uniGonio.setRotationAngle("chi", chi);
      uniGonio.setRotationAngle("omega", omega);
      //Put goniometer into peaks workspace
      outWS->mutableRun().setGoniometer(uniGonio, false);


      std::ostringstream oss;
      oss << "bank" << bankNum;
      std::string bankName = oss.str();

      int seqNum = -1;

      try
      {
        // Read the peak
        Peak peak = readPeak(outWS, s, in, seqNum, bankName);

        // Get the calculated goniometer matrix
        Matrix<double> gonMat = uniGonio.getR();

        peak.setGoniometerMatrix(gonMat);
        peak.setRunNumber(run);
        peak.setMonitorCount( monCount );

        double tof = peak.getTOF()+T0;
        Kernel::Units::Wavelength wl;

        wl.initialize(peak.getL1(), peak.getL2(), peak.getScattering(), 0,
                  peak.getInitialEnergy(), 0.0);

        peak.setWavelength(wl.singleFromTOF( tof));
        // Add the peak to workspace
        outWS->addPeak(peak);
      }
      catch (std::runtime_error & e)
      {
        g_log.warning() << "Error reading peak SEQN " << seqNum << " : " << e.what() << std::endl;
      }

      prog.report();
    }

  }



  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void LoadIsawPeaks::exec()
  {
    // Create the workspace
    PeaksWorkspace_sptr ws(new PeaksWorkspace());

    // This loads (appends) the peaks
    this->appendFile( ws, getPropertyValue("Filename") );

    // Save it in the output
    setProperty("OutputWorkspace", boost::dynamic_pointer_cast<Workspace>(ws));
  }



} // namespace Mantid
} // namespace Crystal

