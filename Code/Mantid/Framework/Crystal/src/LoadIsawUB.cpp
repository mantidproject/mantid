#include "MantidAPI/FileProperty.h"
#include "MantidCrystal/LoadIsawUB.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/System.h"
#include <fstream>
#include <iosfwd>
#include <MantidGeometry/Crystal/OrientedLattice.h>
#include <MantidGeometry/Crystal/UnitCell.h>
#include "MantidAPI/IMDEventWorkspace.h"

using namespace Mantid::Kernel::Strings;
using Mantid::Kernel::DblMatrix;
using Mantid::Geometry::UnitCell;
using Mantid::Geometry::OrientedLattice;

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

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void LoadIsawUB::init()
  {
    declareProperty(new WorkspaceProperty<Workspace>("InputWorkspace","",Direction::InOut),
        "An input workspace to which to add the lattice information.");

    std::vector<std::string> exts;
    exts.push_back(".mat");
    exts.push_back(".ub");
    exts.push_back(".txt");

    declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
        "Path to an ISAW-style UB matrix text file.");

    declareProperty("CheckUMatrix", true,
          "If True (default) then a check is performed to ensure the U matrix is a proper rotation matrix");
  }

  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void LoadIsawUB::exec()
  {
    // In and Out workspace.
    Workspace_sptr ws1 = getProperty("InputWorkspace");

    ExperimentInfo_sptr ws;
    IMDEventWorkspace_sptr MDWS=boost::dynamic_pointer_cast<IMDEventWorkspace>(ws1);
    if (MDWS != NULL)
    {
        ws = MDWS->getExperimentInfo(0);
    }
    else
    {
        ws = boost::dynamic_pointer_cast<ExperimentInfo>(ws1);
    }
    if (!ws) throw
        std::invalid_argument("Must specify either a MatrixWorkspace or a PeaksWorkspace or a MDEventWorkspace.");

    std::string Filename = getProperty("Filename");

    // Open the file
    std::ifstream in( Filename.c_str() );

    Kernel::DblMatrix ub(3,3);
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

    readToEndOfLine(in, true);
    double latVals[6];
    for( size_t col=0 ; col < 6; col++)
    {
      s = getWord(in, true);
      if (!convert(s, val))
                throw std::runtime_error("The string '" + s + "' in the file was not understood as a number.");
      latVals[col] = val;
    }

    // Adjust the UB by transposing
    ub = ub.Transpose();
    
    /* The method in OrientedLattice gets both the lattice parameters and the U matrix from the UB matrix.
     * This is compatible (same results) with the ISAW lattice parameters */
    OrientedLattice * latt = new OrientedLattice();
    latt->setUB(ub);
    latt->setError( latVals[0],latVals[1],latVals[2],latVals[3],latVals[4],latVals[5]);
    DblMatrix U=latt->getU();

    // Swap rows around to accound for IPNS convention
    DblMatrix U2 = U;
    // Swap rows around
    for (size_t r=0; r<3; r++)
    {
      U2[2][r] = U[0][r];
      U2[1][r] = U[2][r];
      U2[0][r] = U[1][r];
    }
    U = U2;
    const bool checkU = getProperty("CheckUMatrix");
    latt->setU(U, !checkU);

    // Save it into the workspace
    ws->mutableSample().setOrientedLattice(latt);

    //Save it to every experiment info in MD workspaces
    if ((MDWS != NULL) && (MDWS->getNumExperimentInfo()>1))
    {
        for(uint16_t i=1;i<MDWS->getNumExperimentInfo();i++)
        {
           ws = MDWS->getExperimentInfo(i);
           ws->mutableSample().setOrientedLattice(latt);
        }
    }

    delete latt;
    this->setProperty("InputWorkspace", ws1);

  }



} // namespace Mantid
} // namespace Crystal
