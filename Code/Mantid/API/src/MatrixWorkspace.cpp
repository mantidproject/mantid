#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/IMDWorkspace.h" 
#include "MantidAPI/Axis.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/LocatedDataRef.h"
#include "MantidAPI/WorkspaceIterator.h"
#include "MantidAPI/WorkspaceIteratorCode.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidGeometry/Instrument/Instrument.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/Instrument/XMLlogfile.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidGeometry/MDGeometry/MDCell.h"
#include "MantidGeometry/MDGeometry/MDPoint.h"
#include "MantidGeometry/MDGeometry/MDDimension.h"
#include "MantidAPI/MatrixWSIndexCalculator.h"
#include "MantidKernel/PhysicalConstants.h"

#include <numeric>

namespace Mantid
{
  namespace API
  {
    using namespace Geometry;

    Kernel::Logger& MatrixWorkspace::g_log = Kernel::Logger::get("MatrixWorkspace");

    /// Default constructor
    MatrixWorkspace::MatrixWorkspace() : 
    IMDWorkspace(), m_axes(), m_isInitialized(false),
      sptr_instrument(new Instrument), m_spectramap(), m_sample(), m_run(),
      m_YUnit(), m_YUnitLabel(), m_isDistribution(false), m_parmap(new ParameterMap), m_masks(), m_indexCalculator()
    {}

    /// Destructor
    // RJT, 3/10/07: The Analysis Data Service needs to be able to delete workspaces, so I moved this from protected to public.
    MatrixWorkspace::~MatrixWorkspace()
    {
      for (unsigned int i = 0; i < m_axes.size(); ++i)
      {
        delete m_axes[i];
      }
    }

    /** Initialize the workspace. Calls the protected init() method, which is implemented in each type of
    *  workspace. Returns immediately if the workspace is already initialized.
    *  @param NVectors The number of spectra in the workspace (only relevant for a 2D workspace
    *  @param XLength The number of X data points/bin boundaries in each vector (must all be the same)
    *  @param YLength The number of data/error points in each vector (must all be the same)
    */
    void MatrixWorkspace::initialize(const int &NVectors, const int &XLength, const int &YLength)
    {
      // Check validity of arguments
      if (NVectors <= 0 || XLength <= 0 || YLength <= 0)
      {
        g_log.error("All arguments to init must be positive and non-zero");
        throw std::out_of_range("All arguments to init must be positive and non-zero");
      }

      // Bypass the initialization if the workspace has already been initialized.
      if (m_isInitialized) return;

      // Invoke init() method of the derived class inside a try/catch clause
      try
      {
        this->init(NVectors, XLength, YLength);
      }
      catch(std::runtime_error& ex)
      {
        g_log.error() << "Error initializing the workspace" << ex.what() << std::endl;
        throw;
      }
      m_indexCalculator =  MatrixWSIndexCalculator(this->blocksize());
      // Indicate that this Algorithm has been initialized to prevent duplicate attempts.
      m_isInitialized = true;
    }


    //---------------------------------------------------------------------------------------
    /** Set the instrument
    *
    * \param instr Shared pointer to an instrument.
    */
    void MatrixWorkspace::setInstrument(const IInstrument_sptr& instr)
    {
      boost::shared_ptr<Instrument> tmp = boost::dynamic_pointer_cast<Instrument>(instr);
      if (tmp->isParametrized())
      {
        sptr_instrument = tmp->baseInstrument();
        m_parmap = tmp->getParameterMap();
      }
      else
      {
        sptr_instrument=tmp;
      }
    }

    //---------------------------------------------------------------------------------------
    /** Get a const reference to the SpectraDetectorMap associated with this workspace.
    *  Can ONLY be taken as a const reference!
    *
    *  @return The SpectraDetectorMap
    */
    const SpectraDetectorMap& MatrixWorkspace::spectraMap() const
    {
      return *m_spectramap;
    }

    //---------------------------------------------------------------------------------------
    /** Get a reference to the SpectraDetectorMap associated with this workspace.
    *  This non-const method will copy the map if it is shared between more than one workspace,
    *  and the reference returned will be to the copy.
    *  Can ONLY be taken by reference!
    *
    *  @return The SpectraDetectorMap
    */
    SpectraDetectorMap& MatrixWorkspace::mutableSpectraMap()
    {
      return m_spectramap.access();
    }


    //---------------------------------------------------------------------------------------
    /** Return a map where:
    *    KEY is the Workspace Index
    *    VALUE is the Spectrum #
    */
    IndexToIndexMap * MatrixWorkspace::getWorkspaceIndexToSpectrumMap() const
    {
      SpectraAxis * ax = dynamic_cast<SpectraAxis * >( this->m_axes[1] );
      if (!ax)
        throw std::runtime_error("MatrixWorkspace::getWorkspaceIndexToSpectrumMap: axis[1] is not a SpectraAxis, so I cannot generate a map.");
      IndexToIndexMap * map = new IndexToIndexMap();
      try
      {
        ax->getIndexSpectraMap(*map);
      }
      catch (std::runtime_error &)
      {
        delete map;
        throw std::runtime_error("MatrixWorkspace::getWorkspaceIndexToSpectrumMap: no elements!");
      }
      return map;
    }

    //---------------------------------------------------------------------------------------
    /** Return a map where:
    *    KEY is the Spectrum #
    *    VALUE is the Workspace Index
    */
    IndexToIndexMap * MatrixWorkspace::getSpectrumToWorkspaceIndexMap() const
    {
      SpectraAxis * ax = dynamic_cast<SpectraAxis * >( this->m_axes[1] );
      if (!ax)
        throw std::runtime_error("MatrixWorkspace::getSpectrumToWorkspaceIndexMap: axis[1] is not a SpectraAxis, so I cannot generate a map.");
      IndexToIndexMap * map = new IndexToIndexMap();
      try
      {
        ax->getSpectraIndexMap(*map);
      }
      catch (std::runtime_error &)
      {
        delete map;
        throw std::runtime_error("MatrixWorkspace::getSpectrumToWorkspaceIndexMap: no elements!");
      }
      return map;
    }

    //---------------------------------------------------------------------------------------
    /** Return a map where:
    *    KEY is the DetectorID (pixel ID)
    *    VALUE is the Workspace Index
    *  @param throwIfMultipleDets set to true to make the algorithm throw an error
    *         if there is more than one detector for a specific workspace index.
    *  @throws runtime_error if there is more than one detector per spectrum (if throwIfMultipleDets is true)
    *  @return Index to Index Map object
    */
    IndexToIndexMap * MatrixWorkspace::getDetectorIDToWorkspaceIndexMap( bool throwIfMultipleDets ) const
    {
      SpectraAxis * ax = dynamic_cast<SpectraAxis * >( this->m_axes[1] );
      if (!ax)
        throw std::runtime_error("MatrixWorkspace::getDetectorIDToWorkspaceIndexMap(): axis[1] is not a SpectraAxis, so I cannot generate a map.");

      IndexToIndexMap * map = new IndexToIndexMap();
      //Loop through the workspace index
      for (int ws=0; ws < this->getNumberHistograms(); ws++)
      {
        //Get the spectrum # from the WS index
        int specNo = ax->spectraNo(ws);

        //Now the list of detectors
        std::vector<int> detList = this->m_spectramap->getDetectors(specNo);
        if (throwIfMultipleDets)
        {
          if (detList.size() > 1)
          {
            delete map;
            throw std::runtime_error("MatrixWorkspace::getDetectorIDToWorkspaceIndexMap(): more than 1 detector for one histogram! I cannot generate a map of detector ID to workspace index.");
          }

          //Set the KEY to the detector ID and the VALUE to the workspace index.
          if (detList.size() == 1)
            (*map)[ detList[0] ] = ws;
        }
        else
        {
          //Allow multiple detectors per workspace index
          for (std::vector<int>::iterator it = detList.begin(); it != detList.end(); it++)
            (*map)[ *it ] = ws;
        }

        //Ignore if the detector list is empty.
      }
      return map;
    }


    //---------------------------------------------------------------------------------------
    /** Return a map where:
    *    KEY is the Workspace Index
    *    VALUE is the DetectorID (pixel ID)
    *  @throws runtime_error if there is more than one detector per spectrum, or other incompatibilities.
    *  @return Map of workspace index to detector/pixel id.
    */
    IndexToIndexMap * MatrixWorkspace::getWorkspaceIndexToDetectorIDMap() const
    {
      SpectraAxis * ax = dynamic_cast<SpectraAxis * >( this->m_axes[1] );
      if (!ax)
        throw std::runtime_error("MatrixWorkspace::getWorkspaceIndexToDetectorIDMap: axis[1] is not a SpectraAxis, so I cannot generate a map.");

      IndexToIndexMap * map = new IndexToIndexMap();
      //Loop through the workspace index
      for (int ws=0; ws < this->getNumberHistograms(); ws++)
      {
        //Get the spectrum # from the WS index
        int specNo = ax->spectraNo(ws);

        //Now the list of detectors
        std::vector<int> detList = this->m_spectramap->getDetectors(specNo);
        if (detList.size() > 1)
        {
          delete map;
          throw std::runtime_error("MatrixWorkspace::getWorkspaceIndexToDetectorIDMap(): more than 1 detector for one histogram! I cannot generate a map of workspace index to detector ID.");
        }

        //Set the KEY to the detector ID and the VALUE to the workspace index.
        if (detList.size() == 1)
          (*map)[ws] = detList[0];

        //Ignore if the detector list is empty.
      }
      return map;
    }


    //---------------------------------------------------------------------------------------
    /** Converts a list of spectrum numbers to the corresponding workspace indices.
    *  Not a very efficient operation, but unfortunately it's sometimes required.
    *  @param spectraList The list of spectrum numbers required
    *  @param indexList   Returns a reference to the vector of indices (empty if not a Workspace2D)
    */
    void MatrixWorkspace::getIndicesFromSpectra(const std::vector<int>& spectraList, std::vector<int>& indexList) const
    {
      // Clear the output index list
      indexList.clear();
      indexList.reserve(this->getNumberHistograms());
      // get the spectra axis
      SpectraAxis *spectraAxis;
      if (this->axes() == 2)
      {
        spectraAxis = dynamic_cast<SpectraAxis*>(this->getAxis(1));
        if (!spectraAxis) return;
      }
      // Just return an empty list if this isn't a Workspace2D
      else return;

      std::vector<int>::const_iterator iter = spectraList.begin();
      while( iter != spectraList.end() )
      {
        for (int i = 0; i < this->getNumberHistograms(); ++i)
        {
          if ( spectraAxis->spectraNo(i) == *iter )
          {
            indexList.push_back(i);
          }
        }
        ++iter;
      }
    }




    //---------------------------------------------------------------------------------------
    /** Integrate all the spectra in the matrix workspace within the range given.
     * Default implementation, can be overridden by base classes if they know something smarter!
     *
     * @param out returns the vector where there is one entry per spectrum in the workspace. Same
     *            order as the workspace indices.
     * @param minX minimum X bin to use in integrating.
     * @param maxX maximum X bin to use in integrating.
     * @param entireRange set to true to use the entire range. minX and maxX are then ignored!
     */
    void MatrixWorkspace::getIntegratedSpectra(std::vector<double> & out, const double minX, const double maxX, const bool entireRange) const
    {
      out.resize(this->getNumberHistograms(), 0.0);

      //Run in parallel if the implementation is threadsafe
      PARALLEL_FOR_IF( this->threadSafe() )
      for (int wksp_index = 0; wksp_index < this->getNumberHistograms(); wksp_index++)
      {
        // Get Handle to data
        const Mantid::MantidVec& x=this->readX(wksp_index);
        const Mantid::MantidVec& y=this->readY(wksp_index);
        // If it is a 1D workspace, no need to integrate
        if (x.size()==2)
        {
          out[wksp_index] = y[0];
        }
        else
        {
          // Iterators for limits - whole range by default
          Mantid::MantidVec::const_iterator lowit, highit;
          lowit=x.begin();
          highit=x.end()-1;

          //But maybe we don't want the entire range?
          if (!entireRange)
          {
            // If the first element is lower that the xmin then search for new lowit
            if ((*lowit) < minX)
              lowit = std::lower_bound(x.begin(),x.end(),minX);
            // If the last element is higher that the xmax then search for new lowit
            if ((*highit) > maxX)
              highit = std::upper_bound(lowit,x.end(),maxX);
          }

          // Get the range for the y vector
          Mantid::MantidVec::difference_type distmin = std::distance(x.begin(), lowit);
          Mantid::MantidVec::difference_type distmax = std::distance(x.begin(), highit);
          double sum(0.0);
          if( distmin <= distmax )
          {
            // Integrate
            sum = std::accumulate(y.begin() + distmin,y.begin() + distmax,0.0);
          }
          //Save it in the vector
          out[wksp_index] = sum;
        }
      }
    }

    //---------------------------------------------------------------------------------------
    /** Get a constant reference to the Sample associated with this workspace.
    * @return const reference to Sample object
    */
    const  Sample& MatrixWorkspace::sample() const
    {
      return *m_sample;
    }

    /** Get a reference to the Sample associated with this workspace.
    *  This non-const method will copy the sample if it is shared between 
    *  more than one workspace, and the reference returned will be to the copy.
    *  Can ONLY be taken by reference!
    * @return reference to sample object
    */
    Sample& MatrixWorkspace::mutableSample()
    {
      return m_sample.access();
    }

    /** Get a constant reference to the Run object associated with this workspace.
    * @return const reference to run object
    */
    const Run& MatrixWorkspace::run() const
    {
      return *m_run;
    }

    /** Get a reference to the Run object associated with this workspace.
    *  This non-const method will copy the Run object if it is shared between 
    *  more than one workspace, and the reference returned will be to the copy.
    *  Can ONLY be taken by reference!
    * @return reference to Run object
    */
    Run& MatrixWorkspace::mutableRun()
    {
      return m_run.access();
    }

    /** Get the effective detector for the given spectrum
    *  @param  index The workspace index for which the detector is required
    *  @return A single detector object representing the detector(s) contributing
    *          to the given spectrum number. If more than one detector contributes then
    *          the returned object's concrete type will be DetectorGroup.
    *  @throw  std::runtime_error if the SpectraDetectorMap has not been filled
    *  @throw  Kernel::Exception::NotFoundError if the SpectraDetectorMap or the Instrument
    do not contain the requested spectrum number of detector ID
    */
    Geometry::IDetector_sptr MatrixWorkspace::getDetector(const int index) const
    {
      if ( ! m_spectramap->nElements() )
      {
        throw std::runtime_error("SpectraDetectorMap has not been populated.");
      }

      const int spectrum_number = getAxis(1)->spectraNo(index);
      const std::vector<int> dets = m_spectramap->getDetectors(spectrum_number);
      if ( dets.empty() )
      {
        throw Kernel::Exception::NotFoundError("Spectrum number not found", spectrum_number);
      }
      IInstrument_sptr localInstrument = getInstrument();
      if( !localInstrument )
      {
        g_log.debug() << "No instrument defined.\n";
        throw Kernel::Exception::NotFoundError("Instrument not found", "");
      }
      if ( dets.size() == 1 ) 
      {
        // If only 1 detector for the spectrum number, just return it
        return localInstrument->getDetector(dets[0]);
      }
      // Else need to construct a DetectorGroup and return that
      std::vector<Geometry::IDetector_sptr> dets_ptr;
      std::vector<int>::const_iterator it;
      for ( it = dets.begin(); it != dets.end(); ++it )
      {
        dets_ptr.push_back( localInstrument->getDetector(*it) );
      }

      return Geometry::IDetector_sptr( new Geometry::DetectorGroup(dets_ptr, false) );
    }

    /** Returns the 2Theta scattering angle for a detector
    *  @param det A pointer to the detector object (N.B. might be a DetectorGroup)
    *  @return The scattering angle (0 < theta < pi)
    */
    double MatrixWorkspace::detectorTwoTheta(Geometry::IDetector_const_sptr det) const
    {
      const Geometry::V3D samplePos = getInstrument()->getSample()->getPos();
      const Geometry::V3D beamLine = samplePos - getInstrument()->getSource()->getPos();

      return det->getTwoTheta(samplePos,beamLine);
    }

    /**Calculates the distance a neutron coming from the sample will have deviated from a
    *  straight tragetory before hitting a detector. If calling this function many times
    *  for the same detector you can call this function once, with waveLength=1, and use
    *  the fact drop is proportional to wave length squared .This function has no knowledge
    *  of which axis is vertical for a given instrument
    *  @param det the detector that the neutron entered
    *  @param waveLength the neutrons wave length in meters
    *  @return the deviation in meters
    */
    double MatrixWorkspace::gravitationalDrop(Geometry::IDetector_const_sptr det, const double waveLength) const
    {
      using namespace PhysicalConstants;
      /// Pre-factor in gravity calculation: gm^2/2h^2
      static const double gm2_OVER_2h2 = g*NeutronMass*NeutronMass/( 2.0*h*h );

      const V3D samplePos = getInstrument()->getSample()->getPos();
      const double pathLength = det->getPos().distance(samplePos);
      // Want L2 (sample-pixel distance) squared, times the prefactor g^2/h^2
      const double L2 = gm2_OVER_2h2*std::pow(pathLength,2);

      return waveLength*waveLength*L2;
    }

    /** Get a shared pointer to the instrument associated with this workspace
    *
    *  @return The instrument class
    */
    IInstrument_sptr MatrixWorkspace::getInstrument()const
    {
      return boost::shared_ptr<Instrument>(new Instrument(sptr_instrument,m_parmap));
    }

    /** Get a shared pointer to the instrument associated with this workspace
    *
    *  @return The instrument class
    */
    boost::shared_ptr<Instrument> MatrixWorkspace::getBaseInstrument()const
    {
      return sptr_instrument;
    }

    /**  Returns a new copy of the instrument parameters
    *    @return a (new) copy of the instruments parameter map
    */
    Geometry::ParameterMap& MatrixWorkspace::instrumentParameters()const
    {
      //TODO: Here duplicates cow_ptr. Figure out if there's a better way

      // Use a double-check for sharing so that we only
      // enter the critical region if absolutely necessary
      if (!m_parmap.unique())
      {
        PARALLEL_CRITICAL(cow_ptr_access)
        {
          // Check again because another thread may have taken copy
          // and dropped reference count since previous check
          if (!m_parmap.unique())
          {
            ParameterMap_sptr oldData=m_parmap;
            m_parmap.reset();
            m_parmap = ParameterMap_sptr(new ParameterMap(*oldData));
          }
        }
      }

      return *m_parmap;

      //return m_parmap.access(); //old cow_ptr thing
    }



    const Geometry::ParameterMap& MatrixWorkspace::constInstrumentParameters() const
    {
      return *m_parmap;
    }

    /// The number of axes which this workspace has
    int MatrixWorkspace::axes() const
    {
      return static_cast<int>(m_axes.size());
    }

    /** Get a pointer to a workspace axis
    *  @param axisIndex The index of the axis required
    *  @throw IndexError If the argument given is outside the range of axes held by this workspace
    *  @return Pointer to Axis object
    */
    Axis* MatrixWorkspace::getAxis(const int& axisIndex) const
    {
      if ( axisIndex < 0 || axisIndex >= static_cast<int>(m_axes.size()) )
      {
        g_log.error() << "Argument to getAxis (" << axisIndex << ") is invalid for this (" << m_axes.size() << " axis) workspace" << std::endl;
        throw Kernel::Exception::IndexError(axisIndex, m_axes.size(),"Argument to getAxis is invalid for this workspace");
      }

      return m_axes[axisIndex];
    }

    /** Replaces one of the workspace's axes with the new one provided.
    *  @param axisIndex The index of the axis to replace
    *  @param newAxis A pointer to the new axis. The class will take ownership.
    *  @throw IndexError If the axisIndex given is outside the range of axes held by this workspace
    *  @throw std::runtime_error If the new axis is not of the correct length (within one of the old one)
    */
    void MatrixWorkspace::replaceAxis(const int& axisIndex, Axis* const newAxis)
    {
      // First check that axisIndex is in range
      if ( axisIndex < 0 || axisIndex >= static_cast<int>(m_axes.size()) )
      {
        g_log.error() << "Value of axisIndex (" << axisIndex << ") is invalid for this (" << m_axes.size() << " axis) workspace" << std::endl;
        throw Kernel::Exception::IndexError(axisIndex, m_axes.size(),"Value of axisIndex is invalid for this workspace");
      }
      // Now check that the new axis is of the correct length
      // Later, may want to allow axis to be one longer than number of vectors, to allow bins.
      if ( std::abs(newAxis->length() - m_axes[axisIndex]->length()) > 1 )
      {
        std::stringstream msg;
        msg << "replaceAxis: The new axis is not a valid length (original axis: "
          << m_axes[axisIndex]->length() << "; new axis: " << newAxis->length() << ")." << std::endl;
        g_log.error(msg.str());
        throw std::runtime_error("replaceAxis: The new axis is not a valid length");
      }

      // If we're OK, then delete the old axis and set the pointer to the new one
      delete m_axes[axisIndex];
      m_axes[axisIndex] = newAxis;
    }

    /**
    *  Whether the workspace contains histogram data
    *  @return whether the worksapace contains histogram data
    */
    bool MatrixWorkspace::isHistogramData() const
    {
      return ( readX(0).size()==readY(0).size() ? false : true );
    }

    /// Returns the units of the data in the workspace
    std::string MatrixWorkspace::YUnit() const
    {
      return m_YUnit;
    }

    /// Sets a new unit for the data (Y axis) in the workspace
    void MatrixWorkspace::setYUnit(const std::string& newUnit)
    {
      m_YUnit = newUnit;
    }

    /// Returns a caption for the units of the data in the workspace
    std::string MatrixWorkspace::YUnitLabel() const
    {
      std::string retVal;
      if ( !m_YUnitLabel.empty() ) retVal = m_YUnitLabel;
      else
      {
        retVal = m_YUnit;
        // If this workspace a distribution & has at least one axis & this axis has its unit set
        // then append that unit to the string to be returned
        if ( !retVal.empty() && this->isDistribution() && this->axes() && this->getAxis(0)->unit() )
        {
          retVal = retVal + " per " + this->getAxis(0)->unit()->label();
        }
      }

      return retVal;
    }

    /// Sets a new caption for the data (Y axis) in the workspace
    void MatrixWorkspace::setYUnitLabel(const std::string& newLabel)
    {
      m_YUnitLabel = newLabel;
    }

    /** Are the Y-values in this workspace dimensioned?
    * TODO: For example: ????
    * @return whether workspace is a distribution or not
    */
    const bool& MatrixWorkspace::isDistribution() const
    {
      return m_isDistribution;
    }

    /** Set the flag for whether the Y-values are dimensioned
    *  @return whether workspace is now a distribution
    */
    bool& MatrixWorkspace::isDistribution(bool newValue)
    {
      m_isDistribution = newValue;
      return m_isDistribution;
    }

    /**
     * Mask a given workspace index, setting the data and error values to the given value
     * @param index The index within the workspace to mask
     * @param maskValue A value to assign to the data and error values of the spectra
     */
    void MatrixWorkspace::maskWorkspaceIndex(const int index, const double maskValue)
    {
      if(index < 0 || index >= this->getNumberHistograms() )
        throw Kernel::Exception::IndexError(index,this->getNumberHistograms(),
					    "MatrixWorkspace::maskWorkspaceIndex,index");

      // Assign the value to the data and error arrays
      MantidVec & yValues = this->dataY(index);
      std::fill(yValues.begin(), yValues.end(), maskValue);
      MantidVec & eValues = this->dataE(index);
      std::fill(eValues.begin(), eValues.end(), maskValue);

      IDetector_sptr det;
      try
      {
	det = this->getDetector(index);
      }
      catch(Kernel::Exception::NotFoundError &)
      {
	return;
      }
      
      PARALLEL_CRITICAL(MatrixWorkspace_maskWorkspaceIndex)
      {
	int spectrum_number = getAxis(1)->spectraNo(index);
	const std::vector<int> dets = m_spectramap->getDetectors(spectrum_number);
	for (std::vector<int>::const_iterator iter=dets.begin(); iter != dets.end(); ++iter)
	{
	  try
	  {
	    if ( Geometry::Detector* det = dynamic_cast<Geometry::Detector*>(sptr_instrument->getDetector(*iter).get()) )
	    {
	      m_parmap->addBool(det,"masked",true);
	    }
	  }
	  catch(Kernel::Exception::NotFoundError &)
	  {
	  }
	}
      }
    }

    /** Masks a single bin. It's value (and error) will be scaled by (1-weight).
    *  @param spectrumIndex The workspace spectrum index of the bin
    *  @param binIndex      The index of the bin in the spectrum
    *  @param weight        'How heavily' the bin is to be masked. =1 for full masking (the default).
    */
    void MatrixWorkspace::maskBin(const int& spectrumIndex, const int& binIndex, const double& weight)
    {
      // First check the spectrumIndex is valid
      if (spectrumIndex < 0 || spectrumIndex >= this->getNumberHistograms() )
        throw Kernel::Exception::IndexError(spectrumIndex,this->getNumberHistograms(),"MatrixWorkspace::maskBin,spectrumIndex");
      // Then check the bin index
      if (binIndex < 0 || binIndex>= this->blocksize() )
        throw Kernel::Exception::IndexError(binIndex,this->blocksize(),"MatrixWorkspace::maskBin,binIndex");

      // Writing to m_masks is not thread-safe, so put in some protection
      PARALLEL_CRITICAL(maskBin)
      {
        // If a mask for this bin already exists, it would be replaced. But I think that is OK.
        // First get a reference to the list for this spectrum (or create a new list)
        MatrixWorkspace::MaskList& specList = m_masks[spectrumIndex];
        // Add the new value. Will automatically be put in the right place (ordered by binIndex)
        specList.insert( std::make_pair(binIndex,weight) );
      }

      this->dataY(spectrumIndex)[binIndex] *= (1-weight);
      // Do we want to scale the error?
      this->dataE(spectrumIndex)[binIndex] *= (1-weight);
    }

    /** Does this spectrum contain any masked bins 
    *  @param spectrumIndex The workspace spectrum index to test
    *  @return True if there are masked bins for this spectrum
    */
    bool MatrixWorkspace::hasMaskedBins(const int& spectrumIndex) const
    {
      // First check the spectrumIndex is valid. Return false if it isn't (decided against throwing here).
      if (spectrumIndex < 0 || spectrumIndex >= this->getNumberHistograms() ) return false;
      return (m_masks.find(spectrumIndex)==m_masks.end()) ? false : true;
    }

    /** Returns the list of masked bins for a spectrum. 
    *  @param  spectrumIndex
    *  @return A const reference to the list of masked bins
    *  @throw  Kernel::Exception::IndexError if there are no bins masked for this spectrum (so call hasMaskedBins first!)
    */
    const MatrixWorkspace::MaskList& MatrixWorkspace::maskedBins(const int& spectrumIndex) const
    {
      std::map<int,MaskList>::const_iterator it = m_masks.find(spectrumIndex);
      // Throw if there are no masked bins for this spectrum. The caller should check first using hasMaskedBins!
      if (it==m_masks.end())
      {
        g_log.error() << "There are no masked bins for spectrum index " << spectrumIndex << std::endl;
        throw Kernel::Exception::IndexError(spectrumIndex,0,"MatrixWorkspace::maskedBins");
      }

      return it->second;
    }

    long int MatrixWorkspace::getMemorySize() const
    {
      //3 doubles per histogram bin.
      return 3*size()*sizeof(double)/1024;
    }

    /** Add parameters to the instrument parameter map that are defined in instrument
    *   definition file and for which logfile data are available. Logs must be loaded 
    *   before running this method.
    */
    void MatrixWorkspace::populateInstrumentParameters()
    {
      // Get instrument and sample

      boost::shared_ptr<const Instrument> instrument = getBaseInstrument();
      Instrument* inst = const_cast<Instrument*>(instrument.get());

      // Get the data in the logfiles associated with the raw data

      const std::vector<Kernel::Property*>& logfileProp = run().getLogData();


      // Get pointer to parameter map that we may add parameters to and information about
      // the parameters that my be specified in the instrument definition file (IDF)

      Geometry::ParameterMap& paramMap = instrumentParameters();
      std::multimap<std::string, boost::shared_ptr<API::XMLlogfile> >& paramInfoFromIDF = inst->getLogfileCache();


      // iterator to browse through the multimap: paramInfoFromIDF

      std::multimap<std::string, boost::shared_ptr<API::XMLlogfile> > :: const_iterator it;
      std::pair<std::multimap<std::string, boost::shared_ptr<API::XMLlogfile> >::iterator,
        std::multimap<std::string, boost::shared_ptr<API::XMLlogfile> >::iterator> ret;

      // In order to allow positions to be set with r-position, t-position and p-position parameters
      // The idea is here to simply first check if parameters with names "r-position", "t-position"
      // and "p-position" are encounted then at the end of this method act on this
      std::set<const IComponent*> rtp_positionComp;
      std::multimap<const IComponent*, m_PositionEntry > rtp_positionEntry;

      // loop over all logfiles and see if any of these are associated with parameters in the
      // IDF

      unsigned int N = logfileProp.size();
      for (unsigned int i = 0; i < N; i++)
      {
        // Get the name of the timeseries property

        std::string logName = logfileProp[i]->name();

        // See if filenamePart matches any logfile-IDs in IDF. If this add parameter to parameter map

        ret = paramInfoFromIDF.equal_range(logName);
        for (it=ret.first; it!=ret.second; ++it)
        {
          double value = ((*it).second)->createParamValue(static_cast<Kernel::TimeSeriesProperty<double>*>(logfileProp[i]));

          // special cases of parameter names

          std::string paramN = ((*it).second)->m_paramName;
          if ( paramN.compare("x")==0 || paramN.compare("y")==0 || paramN.compare("z")==0 )
            paramMap.addPositionCoordinate(((*it).second)->m_component, paramN, value);
          else if ( paramN.compare("rot")==0 || paramN.compare("rotx")==0 || paramN.compare("roty")==0 || paramN.compare("rotz")==0 )
          {
            paramMap.addRotationParam(((*it).second)->m_component, paramN, value);
          }
          else if ( paramN.compare("r-position")==0 || paramN.compare("t-position")==0 || paramN.compare("p-position")==0 )
          {
            rtp_positionComp.insert(((*it).second)->m_component);
            rtp_positionEntry.insert( 
              std::pair<const IComponent*, m_PositionEntry >(
                ((*it).second)->m_component, m_PositionEntry(paramN, value)));
          }
          else
            paramMap.addDouble(((*it).second)->m_component, paramN, value);
        }
      }

      // Check if parameters have been specified using the 'value' attribute rather than the 'logfile-id' attribute
      // All such parameters have been stored using the key = "".
      ret = paramInfoFromIDF.equal_range("");
      Kernel::TimeSeriesProperty<double>* dummy = NULL;
      for (it = ret.first; it != ret.second; ++it)
      {
        std::string paramN = ((*it).second)->m_paramName;
        std::string category = ((*it).second)->m_type;  

        // if category is sting no point in trying to generate a double from parameter
        double value = 0.0;
        if ( category.compare("string") != 0 )
          value = ((*it).second)->createParamValue(dummy);

        if ( category.compare("fitting") == 0 )
        {
          std::ostringstream str;
          str << value << " , " << ((*it).second)->m_fittingFunction << " , " << paramN << " , " << ((*it).second)->m_constraint[0] << " , " 
            << ((*it).second)->m_constraint[1] << " , " << ((*it).second)->m_penaltyFactor << " , " 
            << ((*it).second)->m_tie << " , " << ((*it).second)->m_formula << " , " 
            << ((*it).second)->m_formulaUnit << " , " << ((*it).second)->m_resultUnit << " , " << (*(((*it).second)->m_interpolation));
          paramMap.add("fitting",((*it).second)->m_component, paramN, str.str());
        }
        else if ( category.compare("string") == 0 )
        {
          paramMap.addString(((*it).second)->m_component, paramN, ((*it).second)->m_value);
        }
        else
        {
          if (paramN.compare("x") == 0 || paramN.compare("y") == 0 || paramN.compare("z") == 0)
            paramMap.addPositionCoordinate(((*it).second)->m_component, paramN, value);
          else if ( paramN.compare("rot")==0 || paramN.compare("rotx")==0 || paramN.compare("roty")==0 || paramN.compare("rotz")==0 )        
            paramMap.addRotationParam(((*it).second)->m_component, paramN, value);
          else if ( paramN.compare("r-position")==0 || paramN.compare("t-position")==0 || paramN.compare("p-position")==0 )
          {
            rtp_positionComp.insert(((*it).second)->m_component);
            rtp_positionEntry.insert( 
              std::pair<const IComponent*, m_PositionEntry >(
                ((*it).second)->m_component, m_PositionEntry(paramN, value)));
          }
          else
            paramMap.addDouble(((*it).second)->m_component, paramN, value);
        }
      }

      // check if parameters with names "r-position", "t-position"
      // and "p-position" were encounted
      std::pair<std::multimap<const IComponent*, m_PositionEntry >::iterator,
        std::multimap<const IComponent*, m_PositionEntry >::iterator> retComp;
      double deg2rad = (M_PI/180.0);
      std::set<const IComponent*>::iterator itComp;
      std::multimap<const IComponent*, m_PositionEntry > :: const_iterator itRTP;
      for (itComp=rtp_positionComp.begin(); itComp!=rtp_positionComp.end(); itComp++)
      {
        retComp = rtp_positionEntry.equal_range(*itComp);
        bool rSet = false;
        double rVal=0.0;
        double tVal=0.0;
        double pVal=0.0;
        for (itRTP = retComp.first; itRTP!=retComp.second; ++itRTP)
        {
          std::string paramN = ((*itRTP).second).paramName;  
          if ( paramN.compare("r-position")==0 )
          {
            rSet = true;
            rVal = ((*itRTP).second).value;
          }
          if ( paramN.compare("t-position")==0 )
          {
            tVal = deg2rad*((*itRTP).second).value;
          }
          if ( paramN.compare("p-position")==0 )
          {
            pVal = deg2rad*((*itRTP).second).value;
          }
        }
        if ( rSet )
        {
          // convert spherical coordinates to cartesian coordinate values
          double x = rVal*sin(tVal)*cos(pVal);
          double y = rVal*sin(tVal)*sin(pVal);
          double z = rVal*cos(tVal);
          
          paramMap.addPositionCoordinate(*itComp, "x", x);
          paramMap.addPositionCoordinate(*itComp, "y", y);
          paramMap.addPositionCoordinate(*itComp, "z", z);
        }
      }
    }

    /**
    * Returns the bin index of the given X value
    * @param xValue The X value to search for
    * @param index The index within the workspace to search within (default = 0)
    * @returns An index that 
    */
    size_t MatrixWorkspace::binIndexOf(const double xValue, const int index) const
    {
      if( index < 0 || index >= getNumberHistograms() )
      {
        throw std::out_of_range("MatrixWorkspace::binIndexOf - Index out of range.");
      }
      const MantidVec & xValues = this->dataX(index);
      // Lower bound will test if the value is greater than the last but we need to see if X is valid at the start
      if( xValue < xValues.front() )
      {
        throw std::out_of_range("MatrixWorkspace::binIndexOf - X value lower than lowest in current range.");
      }
      MantidVec::const_iterator lowit = std::lower_bound(xValues.begin(), xValues.end(), xValue);
      if( lowit == xValues.end() )
      {
        throw std::out_of_range("MatrixWorkspace::binIndexOf - X value greater than highest in current range.");
      }
      // If we are pointing at the first value then that means we still want to be in the first bin
      if( lowit == xValues.begin() )
      {
        ++lowit;
      }
      size_t hops = std::distance(xValues.begin(), lowit);
      // The bin index is offset by one from the number of hops between iterators as they start at zero
      return hops - 1;
    }

    int MatrixWorkspace::getNPoints() const
    {
      return this->size();
    }

    const Mantid::Geometry::SignalAggregate& MatrixWorkspace::getPoint(int index) const
    {
      HistogramIndex histInd = m_indexCalculator.getHistogramIndex(index);
      BinIndex binInd = m_indexCalculator.getBinIndex(index, histInd);
      MatrixMDPointMap::const_iterator iter = m_mdPointMap.find(index);
      //Create the MDPoint if it is not already present.
      if(m_mdPointMap.end() ==  iter)
      {
        m_mdPointMap[index] = createPoint(histInd, binInd);
      }
      return m_mdPointMap[index];
    }

    const Mantid::Geometry::SignalAggregate& MatrixWorkspace::getPointImp(int histogram, int bin) const
    {
      Index oneDimIndex = m_indexCalculator.getOneDimIndex(histogram, bin);
      MatrixMDPointMap::const_iterator iter = m_mdPointMap.find(oneDimIndex);
      if(m_mdPointMap.end() ==  iter)
      {
        m_mdPointMap[oneDimIndex] = createPoint(histogram, bin);
      }
      return m_mdPointMap[oneDimIndex];
    }

    Mantid::Geometry::MDPoint  MatrixWorkspace::createPoint(int histogram, int bin) const
    {
      std::vector<Mantid::Geometry::coordinate> verts;

      double x = this->dataX(histogram)[bin];
      double signal = this->dataY(histogram)[bin];
      double error = this->dataE(histogram)[bin];

      coordinate vert1, vert2, vert3, vert4;

      if(isHistogramData()) //TODO. complete vertex generating cases.
      {
        vert1.y = histogram;
        vert2.y = histogram;
        vert3.y = histogram+1;
        vert4.y = histogram+1;
        vert1.x = x;
        vert2.x = this->dataX(histogram)[bin+1];
        vert3.x = x;
        vert4.x = this->dataX(histogram)[bin+1];
      }
      verts.resize(4);
      verts[0] = vert1;
      verts[1] = vert2;
      verts[2] = vert3;
      verts[3] = vert4;

      IDetector_sptr detector;
      try
      {
        detector = this->getDetector(histogram);
      }
      catch(std::exception&)
      {
        //Swallow exception and continue processing.
      }
      return Mantid::Geometry::MDPoint(signal, error, verts, detector, this->sptr_instrument);
    }


    std::string MatrixWorkspace::getDimensionIdFromAxis(Axis const * const axis) const
    {
      return axis->title(); //Seam. single point where we configure how an axis maps to a dimension id.
    }

    boost::shared_ptr<const Mantid::Geometry::IMDDimension> MatrixWorkspace::getXDimension() const
    { 
      Axis* xAxis = this->getAxis(0);
      MDDimension* dimension = new Mantid::Geometry::MDDimension(xAxis->title());
      return boost::shared_ptr<const Mantid::Geometry::IMDDimension>(dimension);
    }

    boost::shared_ptr< const Mantid::Geometry::IMDDimension> MatrixWorkspace::getYDimension() const
    { 
      Axis* yAxis = this->getAxis(1);
      MDDimension* dimension = new Mantid::Geometry::MDDimension(yAxis->title());
      return boost::shared_ptr<const Mantid::Geometry::IMDDimension>(dimension);
    }

    boost::shared_ptr<const Mantid::Geometry::IMDDimension> MatrixWorkspace::getZDimension() const
    { 
      throw std::logic_error("MatrixWorkspaces do not have a z-spacial dimension mapping.");
    }

    boost::shared_ptr<const Mantid::Geometry::IMDDimension> MatrixWorkspace::gettDimension() const
    { 
      throw std::logic_error("MatrixWorkspaces do not have a z-spacial dimension mapping.");
    }

    boost::shared_ptr<const Mantid::Geometry::IMDDimension> MatrixWorkspace::getDimension(std::string id) const
    { 
      int nAxes = this->axes();
      IMDDimension* dim = NULL;
      for(int i = 0; i < nAxes; i++)
      {
        Axis* xAxis = this->getAxis(i);
        const std::string& title = getDimensionIdFromAxis(xAxis);
        if(title == id)
        {
          dim = new Mantid::Geometry::MDDimension(id);
          break;
        }
      }
      if(NULL == dim)
      {
        std::string message = "Cannot find id : " + id;
        throw std::overflow_error(message);
      }
      return boost::shared_ptr<const Mantid::Geometry::IMDDimension>(dim);
    }

    const Mantid::Geometry::SignalAggregate& MatrixWorkspace::getCell(int dim1Increment) const
    { 
      if (dim1Increment<0 || dim1Increment >= static_cast<int>(this->dataX(0).size()))
      {
        throw std::range_error("MatrixWorkspace::getCell, increment out of range");
      }

      return this->getPoint(dim1Increment);
    }

    const Mantid::Geometry::SignalAggregate& MatrixWorkspace::getCell(int dim1Increment, int dim2Increment) const
    { 
      if (dim1Increment<0 || dim1Increment >= static_cast<int>(this->dataX(0).size()))
      {
        throw std::range_error("MatrixWorkspace::getCell, increment out of range");
      }
      if (dim2Increment<0 || dim2Increment >= static_cast<int>(this->dataX(0).size()))
      {
        throw std::range_error("MatrixWorkspace::getCell, increment out of range");
      }

      return getPointImp(dim1Increment, dim2Increment);
    }

    const Mantid::Geometry::SignalAggregate& MatrixWorkspace::getCell(int, int, int) const
    { 
      throw std::logic_error("Cannot access higher dimensions");
    }

    const Mantid::Geometry::SignalAggregate& MatrixWorkspace::getCell(int, int, int, int) const
    { 
      throw std::logic_error("Cannot access higher dimensions");
    }

    const Mantid::Geometry::SignalAggregate& MatrixWorkspace::getCell(...) const
    { 
      throw std::logic_error("Cannot access higher dimensions");
    }


  } // namespace API
} // Namespace Mantid


///\cond TEMPLATE
template DLLExport class Mantid::API::workspace_iterator<Mantid::API::LocatedDataRef,Mantid::API::MatrixWorkspace>;
template DLLExport class Mantid::API::workspace_iterator<const Mantid::API::LocatedDataRef, const Mantid::API::MatrixWorkspace>;

namespace Mantid
{
  namespace Kernel
  {

    template<> DLLExport
      Mantid::API::MatrixWorkspace_sptr IPropertyManager::getValue<Mantid::API::MatrixWorkspace_sptr>(const std::string &name) const
    {
      PropertyWithValue<Mantid::API::MatrixWorkspace_sptr>* prop =
        dynamic_cast<PropertyWithValue<Mantid::API::MatrixWorkspace_sptr>*>(getPointerToProperty(name));
      if (prop)
      {
        return *prop;
      }
      else
      {
        std::string message = "Attempt to assign property "+ name +" to incorrect type";
        throw std::runtime_error(message);
      }
    }

    template<> DLLExport
      Mantid::API::MatrixWorkspace_const_sptr IPropertyManager::getValue<Mantid::API::MatrixWorkspace_const_sptr>(const std::string &name) const
    {
      PropertyWithValue<Mantid::API::MatrixWorkspace_sptr>* prop =
        dynamic_cast<PropertyWithValue<Mantid::API::MatrixWorkspace_sptr>*>(getPointerToProperty(name));
      if (prop)
      {
        return prop->operator()();
      }
      else
      {
        std::string message = "Attempt to assign property "+ name +" to incorrect type";
        throw std::runtime_error(message);
      }
    }

  } // namespace Kernel
} // namespace Mantid

///\endcond TEMPLATE
