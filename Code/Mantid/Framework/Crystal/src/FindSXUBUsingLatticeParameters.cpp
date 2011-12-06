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
#include "MantidDataObjects/PeaksWorkspace.h"

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
      BoundedValidator<double> *mustBePositive = new BoundedValidator<double>();
      mustBePositive->setLower(0.0);

      BoundedValidator<double> *reasonable_angle = new BoundedValidator<double>();
      reasonable_angle->setLower(5.0);
      reasonable_angle->setUpper(175.0);

      this->declareProperty(new PropertyWithValue<double>( "a",-1.0,
        mustBePositive->clone(),Direction::Input),"Lattice parameter a");

      this->declareProperty(new PropertyWithValue<double>( "b",-1.0,
        mustBePositive->clone(),Direction::Input),"Lattice parameter b");

      this->declareProperty(new PropertyWithValue<double>( "c",-1.0,
        mustBePositive->clone(),Direction::Input),"Lattice parameter c");

      this->declareProperty(new PropertyWithValue<double>( "alpha",-1.0,
        reasonable_angle->clone(),Direction::Input),"Lattice parameter alpha");

      this->declareProperty(new PropertyWithValue<double>("beta",-1.0,
        reasonable_angle->clone(),Direction::Input),"Lattice parameter beta");

      this->declareProperty(new PropertyWithValue<double>("gamma",-1.0,
        reasonable_angle->clone(),Direction::Input),"Lattice parameter gamma");

      declareProperty(new ArrayProperty<int> ("PeakIndices"),
        "Index of the peaks in the table workspace to be used");

      declareProperty("dTolerance",0.01,"Tolerance for peak positions in d-spacing");

      this->declareProperty(new WorkspaceProperty<Mantid::DataObjects::PeaksWorkspace>(
        "PeaksWorkspace","",Direction::InOut), "Input Peaks Workspace");
    }

    /**
    Culling method to direct the removal of hkl values off peaks where they cannot sit.
    */
    void FindSXUBUsingLatticeParameters::cullHKLs(int npeaks, std::vector<PeakCandidate>& peaksCandidates, Mantid::Geometry::UnitCell& unitcell)
    {
      for (std::size_t p=0;p<npeaks;p++)
      {
        for (std::size_t q=0;q<npeaks;q++)
        {
          if (p==q) //Don't do a self comparison
          {
            continue;
          }
          peaksCandidates[p].clean(peaksCandidates[q],unitcell,0.5*M_PI/180.0);// Half a degree tolerance
        }
      }
    }

    /** Executes the algorithm
    *
    *  @throw runtime_error Thrown if algorithm cannot execute
    */
    void FindSXUBUsingLatticeParameters::exec()
    {
      using namespace Mantid::DataObjects;
      const std::vector<int> peakindices = getProperty("PeakIndices");

      // Need a least two peaks
      if (peakindices.size() < 2)
      {
        throw std::runtime_error("At least two peaks are required");
      }

      double a = getProperty("a");
      double b = getProperty("b");
      double c = getProperty("c");
      double alpha = getProperty("alpha");
      double beta = getProperty("beta");
      double gamma = getProperty("gamma");

      // Create the Unit-Cell.
      Mantid::Geometry::UnitCell unitcell(a, b, c, alpha, beta, gamma);


      PeaksWorkspace_sptr ws = boost::dynamic_pointer_cast<PeaksWorkspace>(
         AnalysisDataService::Instance().retrieve(this->getProperty("PeaksWorkspace")) );


      std::size_t npeaks=peakindices.size();
      std::vector<PeakCandidate> peaks;
      for (std::size_t i=0;i<npeaks;i++)
      { 
        int row=peakindices[i]-1;
        IPeak& peak = ws->getPeak(row);
        V3D Qs = peak.getQSampleFrame();
        peaks.push_back(PeakCandidate(Qs[0], Qs[1], Qs[2]));
      }

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
      
      // Throw if all collinear
      if (all_collinear)
      {
        throw std::runtime_error("Angles between all pairs of peaks are too small");
      }
      double dtol= getProperty("dTolerance");
      Progress prog(this,0.0,1.0,4);
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
      prog.report(); //1st Progress report.

      for (std::size_t p=0;p<npeaks;p++)
      {
        std::cout << peaks[p] << "\n";
      }


      cullHKLs(npeaks, peaks, unitcell);
      prog.report(); //2nd progress report.

      peaks[0].setFirst(); //On the first peak, now only the first candidate hkl is considered, others are erased,
      //This means the design space of possible peak-hkl alignments has been reduced, will improve future refinements.
      cullHKLs(npeaks, peaks, unitcell);

      prog.report(); //3rd progress report.

      peaks[1].setFirst(); 

      cullHKLs(npeaks, peaks, unitcell);
      prog.report(); //4th progress report.

      //Now we can index the input peaks workspace
      for(int i = 0; i < npeaks; i++)
      {
        IPeak& peak = ws->getPeak(i);
        try
        {
          const V3D hkl = peaks[i].getHKL();
          peak.setHKL(hkl); 
        }
        catch(std::logic_error&)
        {
          continue;
        }
      }
    }



} // namespace Algorithms
} // namespace Mantid
