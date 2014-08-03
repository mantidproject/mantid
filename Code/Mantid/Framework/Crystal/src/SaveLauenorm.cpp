#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidCrystal/SaveLauenorm.h"
#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Utils.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ListValidator.h"
#include "MantidCrystal/AnvredCorrection.h"
#include <boost/math/special_functions/fpclassify.hpp>
#include <fstream>
#include <Poco/File.h>
#include <Poco/Path.h>
#include "boost/assign.hpp"

using namespace Mantid::Geometry;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::PhysicalConstants;
using namespace boost::assign;

namespace Mantid
{
namespace Crystal
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(SaveLauenorm)


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  SaveLauenorm::SaveLauenorm()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  SaveLauenorm::~SaveLauenorm()
  {
  }
  

  //----------------------------------------------------------------------------------------------

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void SaveLauenorm::init()
  {
    declareProperty(new WorkspaceProperty<PeaksWorkspace>("InputWorkspace","",Direction::Input),
        "An input PeaksWorkspace.");
    declareProperty(new API::FileProperty("Filename", "", API::FileProperty::Save),
        "The filename to use for the saved data");
    auto mustBePositive = boost::make_shared<BoundedValidator<double> >();
    mustBePositive->setLower(0.0);
    declareProperty("ScalePeaks", 1.0, mustBePositive,
      "Multiply FSQ and sig(FSQ) by scaleFactor");
    declareProperty("MinDSpacing", 0.0, "Minimum d-spacing (Angstroms)");
    declareProperty("MinWavelength", 0.0, "Minimum wavelength (Angstroms)");
    declareProperty("MaxWavelength", EMPTY_DBL(), "Maximum wavelength (Angstroms)");
    std::vector<std::string> histoTypes;
    histoTypes.push_back("Bank" );
    histoTypes.push_back("RunNumber" );
    histoTypes.push_back("Both Bank and RunNumber");
    declareProperty("SortFilesBy", histoTypes[0],boost::make_shared<StringListValidator>(histoTypes),
      "Sort into files by bank(default), run number or both.");
    declareProperty("MinIsigI", EMPTY_DBL(), mustBePositive,
       "The minimum I/sig(I) ratio");
    declareProperty("WidthBorder", EMPTY_INT(), "Width of border of detectors");
    declareProperty("MinIntensity", EMPTY_DBL(), mustBePositive,
       "The minimum Intensity");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void SaveLauenorm::exec()
  {

	std::string filename = getProperty("Filename");
    ws = getProperty("InputWorkspace");
    double scaleFactor = getProperty("ScalePeaks"); 
    double dMin = getProperty("MinDSpacing");
    double wlMin = getProperty("MinWavelength");
    double wlMax = getProperty("MaxWavelength");
    std::string type = getProperty("SortFilesBy");
    double minIsigI = getProperty("MinIsigI");
    double minIntensity = getProperty("MinIntensity");
    int widthBorder = getProperty("WidthBorder");

    // sequenceNo and run number
    int sequenceNo = 0;
    int oldSequence = -1;

    std::fstream out;
    std::ostringstream ss;


    // We must sort the peaks
    std::vector< std::pair<std::string, bool> > criteria;
    if(type.compare(0,2,"Ba")==0) criteria.push_back( std::pair<std::string, bool>("BankName", true) );
    else if(type.compare(0,2,"Ru")==0) criteria.push_back( std::pair<std::string, bool>("RunNumber", true) );
    else
    {
    	criteria.push_back( std::pair<std::string, bool>("RunNumber", true) );
    	criteria.push_back( std::pair<std::string, bool>("BankName", true) );
    }
    criteria.push_back( std::pair<std::string, bool>("h", true) );
    criteria.push_back( std::pair<std::string, bool>("k", true) );
    criteria.push_back( std::pair<std::string, bool>("l", true) );
    ws->sort(criteria);

    std::vector<Peak> peaks= ws->getPeaks();

    // ============================== Save all Peaks =========================================

    // Go through each peak at this run / bank
    for (int wi=0; wi < ws->getNumberPeaks(); wi++)
    {

      Peak & p = peaks[wi];
      if (p.getIntensity() == 0.0 || boost::math::isnan(p.getIntensity()) || 
        boost::math::isnan(p.getSigmaIntensity())) continue;
      if (minIsigI != EMPTY_DBL() && p.getIntensity() < std::abs(minIsigI * p.getSigmaIntensity())) continue;
      if (minIntensity != EMPTY_DBL() && p.getIntensity() < minIntensity) continue;
      int sequence = p.getRunNumber();
	  std::string bankName = p.getBankName();
	  int nCols, nRows;
	  sizeBanks(bankName, nCols, nRows);
	  if (widthBorder != EMPTY_INT() && (p.getCol() < widthBorder || p.getRow() < widthBorder || p.getCol() > (nCols - widthBorder) ||
			  p.getRow() > (nRows -widthBorder))) continue;
      if(type.compare(0,2,"Ru")!=0)
      {
		  // Take out the "bank" part of the bank name and convert to an int
		  bankName.erase(remove_if(bankName.begin(), bankName.end(), not1(std::ptr_fun (::isdigit))), bankName.end());
		  Strings::convert(bankName, sequence);
      }


      // Two-theta = polar angle = scattering angle = between +Z vector and the scattered beam
      double scattering = p.getScattering();
      double lambda =  p.getWavelength();
      double dsp = p.getDSpacing();
      if(dsp < dMin || lambda < wlMin || (minIsigI != EMPTY_DBL() && lambda > wlMax)) continue;
      // This can be bank number of run number depending on
      if (sequence != oldSequence)
      {
    	  oldSequence = sequence;
    	  out.flush();
    	  out.close();
    	  sequenceNo++;
    	  ss.str("");
    	  ss.clear();
    	  ss << std::setw(3) << std::setfill('0') << sequenceNo;

          Poco::Path path(filename);
          std::string basename = path.getBaseName(); // Filename minus extension
          // Chop off filename
          path.makeParent();
          path.append(basename + ss.str() );
          Poco::File fileobj(path);
          out.open(path.toString().c_str(), std::ios::out);
      }
      // h k l lambda theta intensity and  sig(intensity)  in format (3I5,2F10.5,2I10)
      // HKL is flipped by -1 due to different q convention in ISAW vs mantid.
      if (p.getH() == 0 && p.getK() == 0 && p.getL() == 0) continue;
      out <<  std::setw( 5 ) << Utils::round(-p.getH())
          <<  std::setw( 5 ) << Utils::round(-p.getK())
          <<  std::setw( 5 ) << Utils::round(-p.getL());
      out << std::setw( 10 ) << std::fixed << std::setprecision( 5 ) << lambda;
      // Assume that want theta not two-theta
      out << std::setw( 10 ) << std::fixed << std::setprecision( 5 ) << 0.5 * scattering;

      // SHELX can read data without the space between the l and intensity
      if(p.getDetectorID() != -1)
      {
		  double ckIntensity = scaleFactor*p.getIntensity();
		  if (ckIntensity > 999999999.985) g_log.warning() << "Scaled intensity, " << ckIntensity << " is too large for format.  Decrease ScalePeaks.\n";
		  out << std::setw( 10 ) << Utils::round(ckIntensity);

		  out << std::setw( 10 ) << Utils::round(scaleFactor*p.getSigmaIntensity());
      }
      else
      {
    	  // This is data from LoadLauenorm which is already corrected
          out << std::setw( 10 ) << Utils::round(p.getIntensity());

          out << std::setw( 10 ) << Utils::round(p.getSigmaIntensity());
      }

      out << std::endl;
    }

    out.flush();
    out.close();


  }
  void SaveLauenorm::sizeBanks(std::string bankName, int& nCols, int& nRows)
  {
         if (bankName.compare("None") == 0) return;
         boost::shared_ptr<const IComponent> parent = ws->getInstrument()->getComponentByName(bankName);
         if(!parent) return;
         if (parent->type().compare("RectangularDetector") == 0)
         {
                        boost::shared_ptr<const RectangularDetector> RDet = boost::dynamic_pointer_cast<
                                                                const RectangularDetector>(parent);

                        nCols = RDet->xpixels();
                        nRows = RDet->ypixels();
         }
         else
         {
                        std::vector<Geometry::IComponent_const_sptr> children;
                        boost::shared_ptr<const Geometry::ICompAssembly> asmb = boost::dynamic_pointer_cast<const Geometry::ICompAssembly>(parent);
                        asmb->getChildren(children, false);
                        boost::shared_ptr<const Geometry::ICompAssembly> asmb2 = boost::dynamic_pointer_cast<const Geometry::ICompAssembly>(children[0]);
                        std::vector<Geometry::IComponent_const_sptr> grandchildren;
                        asmb2->getChildren(grandchildren,false);
                        nRows = static_cast<int>(grandchildren.size());
                        nCols = static_cast<int>(children.size());
         }
  }


} // namespace Mantid
} // namespace Crystal
