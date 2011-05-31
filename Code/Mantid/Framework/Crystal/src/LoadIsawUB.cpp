#include "MantidAPI/FileProperty.h"
#include "MantidCrystal/LoadIsawUB.h"
#include "MantidGeometry/Math/Matrix.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/System.h"
#include <fstream>
#include <iosfwd>
#include <MantidGeometry/Crystal/OrientedLattice.h>
#include <MantidGeometry/Crystal/UnitCell.h>

using namespace Mantid::Kernel::Strings;
using Mantid::Geometry::MantidMat;
using Mantid::Geometry::UnitCell;

namespace Mantid
{
namespace Crystal
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(LoadIsawUB)
  
  using namespace Mantid::Kernel;
  using namespace Mantid::API;


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  LoadIsawUB::LoadIsawUB()
  {
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  LoadIsawUB::~LoadIsawUB()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void LoadIsawUB::initDocs()
  {
    this->setWikiSummary("Load an ISAW-style ASCII UB matrix and lattice parameters file, and place its information into a workspace.");
    this->setOptionalMessage("Load an ISAW-style ASCII UB matrix and lattice parameters file, and place its information into a workspace.");
    this->setWikiDescription(
        "The ISAW UB Matrix file format is: TODO"
        );
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void LoadIsawUB::init()
  {
    declareProperty(new WorkspaceProperty<>("InputWorkspace","",Direction::InOut),
        "An input workspace to which to add the lattice information.");

    std::vector<std::string> exts;
    exts.push_back(".mat");
    exts.push_back(".ub");
    exts.push_back(".txt");

    declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
        "Path to an ISAW-style UB matrix text file.");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void LoadIsawUB::exec()
  {
    // In and Out workspace.
    MatrixWorkspace_sptr ws = getProperty("InputWorkspace");

    std::string Filename = getProperty("Filename");

    // Open the file
    std::ifstream in( Filename.c_str() );

    Geometry::Matrix<double> ub(3,3);
    std::string s;
    double val;

    // Read the ISAW UB matrix
    for (size_t row=0; row<3; row++)
    {
      for (size_t col=0; col<3; col++)
      {
        s = getWord(in, true);
        if (!convert(s, val))
          throw std::runtime_error("The string '" + s + "' in the file was not understood as a number.");
        ub[row][col] = val;
      }
      readToEndOfLine(in, true);
    }

    // Adjust the UB by transposing
    ub = ub.Transpose();

    UnitCell uc;
    MantidMat Gstar = ub.Tprime() * ub ;
    uc.recalculateFromGstar( Gstar );
    std::cout << uc.a() << " "  << uc.b() << " "  << uc.c() << " "
        << uc.alpha() << " "  << uc.beta() << " "  << uc.gamma() << std::endl;

//    // At this point, the UB uses the IPNS convention meaning that
//    // for us, the output Q vector from UB.hkl = Qz, Qx, Qy
//    Geometry::Matrix<double> adjust(3,3);
//    adjust[0][1] = 1.0;
//    adjust[1][2] = 1.0;
//    adjust[2][0] = 1.0;
//    // This matrix will swap the axes around to get our convention.
//    U = adjust * U;

    double lattPar[6];
    for (size_t c=0; c<6; c++)
    {
      s = getWord(in, true);
      if (!convert(s, val))
        throw std::runtime_error("The string '" + s + "' in the file was not understood as a number.");
      lattPar[c] = val;
    }

    // Create the lattice from the file's parameter
    OrientedLattice * latt = new OrientedLattice(lattPar[0], lattPar[1], lattPar[2], lattPar[3], lattPar[4], lattPar[5]);
    // Set the UB in there.
    Geometry::Matrix<double> Binv = latt->getBinv(); // B^-1
    Geometry::Matrix<double> U = ub * Binv; // U = UB * B^-1

    // Swap rows around to accound for IPNS convention
    MantidMat U2 = U;
    for (size_t r=0; r<3; r++)
    {
      U2[2][r] = U[0][r];
      U2[1][r] = U[2][r];
      U2[0][r] = U[1][r];
    }
    U = U2;
    //U *= -1.0;

    latt->setU( U );

    // Save it into the workspace
    ws->mutableSample().setOrientedLattice(latt);


    this->setProperty("InputWorkspace", ws);

  }



} // namespace Mantid
} // namespace Crystal

