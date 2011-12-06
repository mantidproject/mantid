//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCrystal/FindSXUBUsingLatticeParameters.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidAPI/Progress.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidAPI/Column.h"
#include "MantidAPI/IPeak.h"

namespace Mantid
{
namespace Crystal
{

// Register the class into the algorithm factory
DECLARE_ALGORITHM(FindSXUBUsingLatticeParameters)

using namespace Kernel;
using namespace API;

/// Set the documentation strings
void FindSXUBUsingLatticeParameters::initDocs()
{
  this->setWikiSummary("Takes a PeaksWorkspace and a B-Matrix and determines the hkl values corresponding to each peak and a UB matrix for the sample. Sets results on input workspace.");
  this->setOptionalMessage("Takes a PeaksWorkspace and a B-Matrix and determines the hkl values corresponding to each peak and a UB matrix for the sample. Sets results on input workspace.");
}

/** Initialisation method.
 *
 */
void FindSXUBUsingLatticeParameters::init()
{
  declareProperty(new WorkspaceProperty<API::ITableWorkspace>("PeaksTable","",Direction::Input),
      "The name of the TableWorkspace in which to store the list of peaks found" );
  declareProperty(new ArrayProperty<double> ("UnitCell"),
         "Lattice parameters");
  declareProperty(new ArrayProperty<int> ("PeakIndices"),
       "Index of the peaks in the table workspace to be used");
  declareProperty("dTolerance",0.01,"Tolerance for peak positions in d-spacing");

  //TODO
  //this->declareProperty(new WorkspaceProperty<PeaksWorkspace>(
  //        "PeaksWorkspace","",Direction::InOut), "Input Peaks Workspace");

}

/** Executes the algorithm
 *
 *  @throw runtime_error Thrown if algorithm cannot execute
 */
void FindSXUBUsingLatticeParameters::exec()
{
	const std::vector<int> peakindices = getProperty("PeakIndices");

	// Need a least two peaks
	if (peakindices.size() < 2)
	{
	    throw std::runtime_error("At least two peaks are required");
	}

	std::vector<double> cell = getProperty("UnitCell");

	// Check that number of Unit Cell parameters is =6
	if (cell.size() != 6)
	{
		throw std::runtime_error("Problem with lattice parameters");
	}
	// Create the Unit-Cell.
	Mantid::Geometry::UnitCell unitcell(cell[0],cell[1],cell[2],cell[3],cell[4],cell[5]);
	//Mantid::Geometry::UnitCell unitcell(cell);
	//


	//Mantid::Algorithms::CreatePeaksWorkspace pkwsp;
    Mantid::API::ITableWorkspace_sptr table=getProperty("PeaksTable");

	// Make sure the table has the right column names, qx,qy,qz,Intensity,and NPixels otherwise throw
    std::vector<std::string> colnames=table->getColumnNames();
	std::vector<std::string> correctnames(3);
	correctnames[0]="Qx";
	correctnames[1]="Qy";
	correctnames[2]="Qz";
	if (!std::equal(colnames.begin(),colnames.begin()+2,correctnames.begin()))
		throw std::runtime_error("The input table has invalid fields");
	//
	std::size_t npeaks=peakindices.size();
	std::vector<PeakCandidate> peaks;
	for (std::size_t i=0;i<npeaks;i++)
	{
		int row=peakindices[i]-1;
		double qx=table->Double(row,0);
		double qy=table->Double(row,1);
		double qz=table->Double(row,2);
		peaks.push_back(PeakCandidate(qx,qy,qz));
	}



 // Mantid::API::IPeaksWorkspace_sptr peaksWS = getProperty("PeaksWorkspace");
 


	// Find two non-colinear peaks
	bool all_collinear=true;

	for (std::size_t i=0;i<npeaks;i++)
	{
		for (std::size_t j=i;j<npeaks;j++)
		{
			double anglerad=peaks[i].angle(peaks[j]);
			if (anglerad>2.0*M_PI/180.0 && anglerad<178.0*M_PI/180.0)
			{
				all_collinear=false;
				break;
			}
		}
	}
	// Throw if all collinear
	if (all_collinear)
  {
		throw std::runtime_error("Angles between all pairs of peaks are too small");
  }
	//
	double dtol= getProperty("dTolerance");

	for (int h=-20;h<20;h++)
	{
		for (int k=-20;k<20;k++)
			{
			for (int l=-20;l<20;l++)
				{
					double dspacing=unitcell.d(h,k,l); //Create a fictional d spacing
					for (std::size_t p=0;p<npeaks;p++)
					{
            double dSpacingPeaks = peaks[p].getdSpacing();
						if (std::abs(dspacing-dSpacingPeaks)<dtol)
							peaks[p].addHKL(h,k,l); // If the peak position and the fictional d spacing are within tolerance, add it
					}
				}
			}
	}

	for (std::size_t p=0;p<npeaks;p++)
	{
		std::cout << peaks[p] << "\n";
	}


	for (std::size_t p=0;p<npeaks;p++)
	{
		for (std::size_t q=0;q<npeaks;q++)
		{
			if (p==q) //Don't do a self comparison
      {
				continue;
      }
			peaks[p].clean(peaks[q],unitcell,2*M_PI/180.0);// Half a degree tolerance
		}
	}

	peaks[0].setFirst(); //On the first peak, now only the first candidate hkl is considered, others are erased,
  //This means the design space of possible peak-hkl alignments has been reduced, will improve future refinements.
	for (std::size_t p=0;p<npeaks;p++)
	{
		for (std::size_t q=0;q<npeaks;q++)
		{
			if (p==q) //Don't do a self comparison
      {
				continue;
      }
			peaks[p].clean(peaks[q],unitcell,0.5*M_PI/180.0);// Half a degree tolerance should be configurable
		}
	}
	peaks[1].setFirst(); //What does this do?
		for (std::size_t p=0;p<npeaks;p++)
		{
			for (std::size_t q=0;q<npeaks;q++)
			{
				if (p==q) //Don't do a self comparison
        {
					continue;
        }
				peaks[p].clean(peaks[q],unitcell,0.5*M_PI/180.0);// Half a degree tolerance
			}
		}

    //Now we can index the input peaks workspace
    //Now we can create a ub matrix!

    
    //std::vector<Peak> &peaks = ws->getPeaks();
    //size_t n_peaks = ws->getNumberPeaks();

    //std::vector<V3D>  q_vectors;
    //for ( size_t i = 0; i < n_peaks; i++ )
    //  q_vectors.push_back( peaks[i].getQSampleFrame() );

    //IPeaksWorkspace* input;
    //Column_sptr col_h = input->getColumn("h");
    //Column_sptr col_k = input->getColumn("k");
    //Column_sptr col_l = input->getColumn("l");
    //for(double i = 0; i < peaks.size(); i++)
    //{
    //  try
    //  {
    //  const V3D hkl = peaks[i].getHKL();
    //  col_h->cell<double>(i) = hkl[0];
    //  col_k->cell<double>(i) = hkl[1];
    //  col_l->cell<double>(i) = hkl[2];
    //  }
    //  catch(std::logic_error&)
    //  {
    //    continue;
    //  }
    //}
    
	//
	std::cout << "New list \n";
	for (std::size_t p=0;p<npeaks;p++)
	{
		std::cout << peaks[p] << "\n";
	}

	Progress prog(this,0.0,1.0,1);
	prog.report();
}



} // namespace Algorithms
} // namespace Mantid
