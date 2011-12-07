/*WIKI* 

Given a PeaksWorkspace and a set of lattice parameters, attempts to tag each peak with a HKL value
by comparing d-spacings between potential hkl matches and the peaks as well as angles between Q vectors.

==Usage Notes==
This algorithm does not generate a UB Matrix, it will only index peaks. Run CalculateUBMatrix algorithm after executing this algorithm in order
to attach a UB Matrix onto the sample. The CopySample algorithm will allow this UB Matrix to be transfered between workspaces.

*WIKI*/

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCrystal/IndexSXPeaks.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidAPI/Progress.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidAPI/Column.h"
#include "MantidAPI/IPeak.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include <sstream>

namespace Mantid
{
  namespace Crystal
  {

    // Register the class into the algorithm factory
    DECLARE_ALGORITHM(IndexSXPeaks)

    using namespace Kernel;
    using namespace API;

    /// Set the documentation strings
    void IndexSXPeaks::initDocs()
    {
      this->setWikiSummary("Takes a PeaksWorkspace and a B-Matrix and determines the hkl values corresponding to each peak and a UB matrix for the sample. Sets results on input workspace.");
      this->setOptionalMessage("Takes a PeaksWorkspace and a B-Matrix and determines the hkl values corresponding to each peak and a UB matrix for the sample. Sets results on input workspace.");
    }

    /** Initialisation method.
    *
    */
    void IndexSXPeaks::init()
    {
      BoundedValidator<double> *mustBePositive = new BoundedValidator<double>();
      mustBePositive->setLower(0.0);

      BoundedValidator<double> *reasonable_angle = new BoundedValidator<double>();
      reasonable_angle->setLower(5.0);
      reasonable_angle->setUpper(175.0);

      declareProperty(new WorkspaceProperty<Mantid::DataObjects::PeaksWorkspace>(
        "PeaksWorkspace","",Direction::InOut), "Input Peaks Workspace");

      declareProperty(new PropertyWithValue<double>( "a",-1.0,
        mustBePositive->clone(),Direction::Input),"Lattice parameter a");

      declareProperty(new PropertyWithValue<double>( "b",-1.0,
        mustBePositive->clone(),Direction::Input),"Lattice parameter b");

      declareProperty(new PropertyWithValue<double>( "c",-1.0,
        mustBePositive->clone(),Direction::Input),"Lattice parameter c");

      declareProperty(new PropertyWithValue<double>( "alpha",-1.0,
        reasonable_angle->clone(),Direction::Input),"Lattice parameter alpha");

      declareProperty(new PropertyWithValue<double>("beta",-1.0,
        reasonable_angle->clone(),Direction::Input),"Lattice parameter beta");

      declareProperty(new PropertyWithValue<double>("gamma",-1.0,
        reasonable_angle->clone(),Direction::Input),"Lattice parameter gamma");

      declareProperty(new ArrayProperty<int> ("PeakIndices"),
        "Index of the peaks in the table workspace to be used. If no index are provided, all will be used.");

      declareProperty("dTolerance",0.01,"Tolerance for peak positions in d-spacing");

      std::vector<int> extents(6,0);
      const int range = 20;
    extents[0]=-range;extents[1]=range;extents[2]=-range;extents[3]=range;extents[4]=-range;extents[5]=range;
    declareProperty(
      new ArrayProperty<int>("SearchExtents", extents),
      "A comma separated list of min, max for each of H, K and L,\n"
      "Specifies the search extents applied for H K L values associated with the peaks.");
    }

    /**
    Culling method to direct the removal of hkl values off peaks where they cannot sit.
    @peakCandidates : Potential peaks containing sets of possible hkl values.
    */
    void IndexSXPeaks::cullHKLs(std::vector<PeakCandidate>& peakCandidates, Mantid::Geometry::UnitCell& unitcell)
    {
      size_t npeaks = peakCandidates.size();
      for (std::size_t p=0;p<npeaks;p++)
      {
        for (std::size_t q=0;q<npeaks;q++)
        {
          if (p==q) //Don't do a self comparison
          {
            continue;
          }
          peakCandidates[p].clean(peakCandidates[q],unitcell,0.5*M_PI/180.0);// Half a degree tolerance
        }
      }
    }

    /**
    Check that not all peaks are colinear and throw if they are not.
    @param PeakCandidates : Potential peaks
    @throws runtime_error if all colinear peaks have been provided
    */
    void IndexSXPeaks::validateNotColinear(std::vector<PeakCandidate>& peakCandidates) const
    {
      // Find two non-colinear peaks
      bool all_collinear=true;
      size_t npeaks = peakCandidates.size();
      for (std::size_t i=0;i<npeaks;i++)
      {
        for (std::size_t j=i;j<npeaks;j++)
        {
          double anglerad=peakCandidates[i].angle(peakCandidates[j]);
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
    }

    /** Executes the algorithm
    *
    *  @throw runtime_error Thrown if algorithm cannot execute
    */
    void IndexSXPeaks::exec()
    {
      using namespace Mantid::DataObjects;
      const std::vector<int> peakindices = getProperty("PeakIndices");

      // Need a least two peaks
      if (peakindices.size() < 2)
      {
        throw std::runtime_error("At least two peaks are required");
      }

      //Get the effective unit cell
      double a = getProperty("a");
      double b = getProperty("b");
      double c = getProperty("c");
      double alpha = getProperty("alpha");
      double beta = getProperty("beta");
      double gamma = getProperty("gamma");

      std::vector<int> extents = getProperty("SearchExtents");
      if(extents.size() != 6)
      {
        std::stringstream stream;
        stream << "Expected 6 elements for the extents. Got: " << extents.size();
        throw std::runtime_error(stream.str());      
      }

      // Create the Unit-Cell.
      Mantid::Geometry::UnitCell unitcell(a, b, c, alpha, beta, gamma);

      PeaksWorkspace_sptr ws = boost::dynamic_pointer_cast<PeaksWorkspace>(
         AnalysisDataService::Instance().retrieve(this->getProperty("PeaksWorkspace")) );

      //Explode each peak object to generate a CandidatePeak, which is internal to this algorithm.
      std::size_t npeaks=peakindices.size();

      //If the user provides no peaks we default to use all the available peaks.
      if(npeaks == 0)
      {
        npeaks = ws->getNumberPeaks();
      }

      std::vector<PeakCandidate> peaks;
      for (std::size_t i=0;i<npeaks;i++)
      { 
        int row=peakindices[i]-1;
        IPeak& peak = ws->getPeak(row);
        V3D Qs = peak.getQSampleFrame();
        peaks.push_back(PeakCandidate(Qs[0], Qs[1], Qs[2]));
      }

      //Sanity check the generated peaks.
      validateNotColinear(peaks);

      //Generate HKL possibilities for each peak.
      double dtol= getProperty("dTolerance");
      Progress prog(this,0.0,1.0,4);
      for (int h=extents[0]; h<extents[1]; h++)
      {
        for (int k=extents[2]; k<extents[3]; k++)
        {
          for (int l=extents[4]; l<extents[5]; l++)
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

      cullHKLs(peaks, unitcell);

      prog.report(); //2nd progress report.
      peaks[0].setFirst(); //On the first peak, now only the first candidate hkl is considered, others are erased,
      //This means the design space of possible peak-hkl alignments has been reduced, will improve future refinements.
      
      cullHKLs(peaks, unitcell);
      prog.report(); //3rd progress report.

      peaks[1].setFirst(); 

      cullHKLs(peaks, unitcell);
      prog.report(); //4th progress report.

      //Now we can index the input/output peaks workspace
      for(int i = 0; i < int(npeaks); i++)
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
