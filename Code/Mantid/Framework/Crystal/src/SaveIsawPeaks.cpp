#include "MantidAPI/FileProperty.h"
#include "MantidCrystal/SaveIsawPeaks.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/V3D.h"
#include "MantidGeometry/IInstrument.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Strings.h"
#include <fstream>
#include "MantidDataObjects/Peak.h"

using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using Mantid::Geometry::IInstrument_sptr;
using Mantid::Geometry::IInstrument_sptr;

namespace Mantid
{
namespace Crystal
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(SaveIsawPeaks)


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  SaveIsawPeaks::SaveIsawPeaks()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  SaveIsawPeaks::~SaveIsawPeaks()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void SaveIsawPeaks::initDocs()
  {
    this->setWikiSummary("Save a PeaksWorkspace to a ISAW-style ASCII .peaks file.");
    this->setOptionalMessage("Save a PeaksWorkspace to a ISAW-style ASCII .peaks file.");
    this->setWikiDescription("Save a PeaksWorkspace to a ISAW-style ASCII .peaks file.");
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void SaveIsawPeaks::init()
  {
    declareProperty(new WorkspaceProperty<PeaksWorkspace>("InputWorkspace","",Direction::Input), "An input workspace.");

    std::vector<std::string> exts;
    exts.push_back(".peaks");
    exts.push_back(".integrate");

    declareProperty(new FileProperty("Filename", "", FileProperty::Save, exts),
        "Path to an ISAW-style peaks or integrate file to save.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void SaveIsawPeaks::exec()
  {
    // Section header
    std::string header = "2   SEQN    H    K    L     COL     ROW    CHAN       L2  2_THETA       AZ        WL        D   IPK      INTI   SIGI RFLG";

    std::string filename = getPropertyValue("Filename");
    PeaksWorkspace_sptr ws = getProperty("InputWorkspace");
    std::vector<Peak> peaks = ws->getPeaks();

    std::ofstream out( filename.c_str() );

    IInstrument_sptr inst = ws->getInstrument();
    double l1; V3D beamline; double beamline_norm; V3D samplePos;
    inst->getInstrumentParameters(l1, beamline, beamline_norm, samplePos);

    out << "Version: 2.0  Facility: SNS " ;
    out <<  " Instrument: " <<  inst->getName() <<  " Date: " ;
    out <<  " TODO: Instrument date " << std::endl;

    out << "0     1     2    3    4      5       6       7        8       9       10        11        12     13      14      15    16" << std::endl;
    out << "-" << header.substr(1, header.size()-1) << std::endl;

    out << "6        L1    T0_SHIFT" <<  std::endl;
    out << "7 "<< std::setw( 11 )  ;
    out <<   std::setprecision( 4 ) <<  std::fixed <<  ( l1*100 ) ;
    out << std::setw( 12 ) <<  std::setprecision( 3 ) <<  std::fixed  ;
    // Time offset of 0.00 for now
    out << "0.000" <<  std::endl;

    if (false)
    {
      //TODO: Do we need to save the .detcal header
      out <<  "4 DETNUM  NROWS  NCOLS   WIDTH   HEIGHT     DEPTH   DETD  "
          <<  " CenterX   CenterY   CenterZ   BaseX    BaseY    BaseZ    "
          <<  "  UpX      UpY      UpZ" <<  std::endl;
      // Here would save each detector...
    }

    // We must sort the peaks first by run, then bank #, and save the list of workspace indices of it
    typedef std::map<int, std::vector<size_t> > bankMap_t;
    typedef std::map<int, bankMap_t> runMap_t;
    runMap_t runMap;

    for (size_t i=0; i < peaks.size(); ++i)
    {
      Peak & p = peaks[i];
      int run = p.getRunNumber();
      int bank = 0;
      std::string bankName = p.getBankName();
      if (bankName.size() <= 4)
      {
        g_log.information() << "Could not interpret bank number of peak " << i << "(" << bankName << ")\n";
        continue;
      }
      // Take out the "bank" part of the bank name and convert to an int
      bankName = bankName.substr(4, bankName.size()-4);
      Strings::convert(bankName, bank);

      // Save in the map
      runMap[run][bank].push_back(i);
    }

    // Sequence number
    int seqNum = 1;

    // Go in order of run numbers
    runMap_t::iterator runMap_it;
    for (runMap_it = runMap.begin(); runMap_it != runMap.end(); runMap_it++)
    {
      // Start of a new run
      int run = runMap_it->first;
      bankMap_t & bankMap = runMap_it->second;

      bankMap_t::iterator bankMap_it;
      for (bankMap_it = bankMap.begin(); bankMap_it != bankMap.end(); bankMap_it++)
      {
        // Start of a new bank.
        int bank = bankMap_it->first;
        std::vector<size_t> & ids = bankMap_it->second;

        if (ids.size() > 0)
        {
          // Write the bank header
          out << "0 NRUN DETNUM    CHI    PHI  OMEGA MONCNT" << std::endl;
          out <<  "1" <<  std::setw( 5 ) <<  run <<  std::setw( 7 ) <<
              std::right <<  bank;

          // TODO: Determine goniometer angles!
          double chi = 0.0;
          double phi = 0.0;
          double omega = 0.0;
          out  <<  std::setw( 7 ) <<  std::fixed <<  std::setprecision( 2 )  <<  chi;
          out  <<  std::setw( 7 ) <<  std::fixed <<  std::setprecision( 2 )  <<  phi;
          out  <<  std::setw( 7 ) <<  std::fixed <<  std::setprecision( 2 )  <<  omega;
          out  <<  std::setw( 7 ) <<  (int)( 0 ) <<  std::endl;

          out << header << std::endl;

          // Go through each peak at this run / bank
          for (size_t i=0; i < ids.size(); i++)
          {
            size_t wi = ids[i];
            Peak & p = peaks[wi];

            out <<  "3" <<  std::setw( 7 ) <<  seqNum <<  std::setw( 5 ) << static_cast<int>(p.getH())
                <<  std::setw( 5 ) <<  static_cast<int>(p.getK()) <<  std::setw( 5 ) <<  static_cast<int>(p.getL());
            out <<  std::setw( 8 ) <<  std::fixed << std::setprecision( 2 )
            << static_cast<double>(p.getCol());

            out << std::setw( 8 ) << std::fixed << std::setprecision( 2 )
            << static_cast<double>(p.getRow());

            out << std::setw( 8 ) << std::fixed << std::setprecision( 0 )
            << p.getTOF();


            out << std::setw( 9 ) << std::fixed << std::setprecision( 3 )
            << (p.getL2()*100.0);

            // This is the scattered beam direction
            V3D dir = p.getDetPos() - inst->getSample()->getPos();
            double scattering, azimuth;

            // Two-theta = polar angle = scattering angle = between +Z vector and the scattered beam
            scattering = dir.angle( V3D(0.0, 0.0, 1.0) );

            // "Azimuthal" angle: project the beam onto the XY plane, and measure the angle between that and the +X axis (right-handed)
            azimuth = atan2( dir.Y(), dir.X() );

            out << std::setw( 9 ) << std::fixed << std::setprecision( 5 )
            << scattering; //two-theta scattering

            out << std::setw( 9 ) << std::fixed << std::setprecision( 5 )
            << azimuth;

            out << std::setw( 10 ) << std::fixed << std::setprecision( 6 )
            << p.getWavelength();

            out << std::setw( 9 ) << std::fixed << std::setprecision( 4 )
            << p.getDSpacing();

            out << std::setw( 6 ) << static_cast<int>(p.getBinCount()) << std::setw( 10 )
            << std::fixed << std::setprecision( 2 ) << p.getIntensity();

            out << std::setw( 7 ) << std::fixed << std::setprecision( 2 )
            << p.getSigmaIntensity();

            int thisReflag = 0;
            out << std::setw( 5 ) << thisReflag;

            out << std::endl;

            // Count the sequence
            seqNum++;
          }
        }
      }
    }

    out.flush();
    out.close();

//    //REMOVE:
//    std::string line;
//    std::ifstream myfile (filename.c_str());
//    if (myfile.is_open())
//    {
//      while ( myfile.good() )
//      {
//        getline (myfile,line);
//        std::cout << line << std::endl;
//      }
//      myfile.close();
//    }


  }





} // namespace Mantid
} // namespace Crystal

