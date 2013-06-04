#ifndef MANTID_API_MATRIXWORKSPACE_H_
#define MANTID_API_MATRIXWORKSPACE_H_ 

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <boost/scoped_ptr.hpp>
#include "MantidAPI/DllConfig.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/ISpectrum.h"
#include "MantidAPI/MatrixWSIndexCalculator.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/SpectraDetectorTypes.h"
#include "MantidAPI/WorkspaceIterator.h"

namespace Mantid
{
  //----------------------------------------------------------------------------
  // Forward declarations
  //----------------------------------------------------------------------------
  namespace Geometry
  {
    class ParameterMap;
    class INearestNeighbours;
    class INearestNeighboursFactory;
  }
  namespace API
  {
    class SpectrumDetectorMapping;

    //----------------------------------------------------------------------
    /** Base MatrixWorkspace Abstract Class.

    @author Laurent C Chapon, ISIS, RAL
    @date 26/09/2007

    Copyright &copy; 2007-2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    class MANTID_API_DLL MatrixWorkspace : public IMDWorkspace, public ExperimentInfo
    {
    public:

      // The Workspace Factory create-from-parent method needs direct access to the axes.
      friend class WorkspaceFactoryImpl;

      /// Typedef for the workspace_iterator to use with a Workspace
      typedef workspace_iterator<LocatedDataRef, MatrixWorkspace> iterator;
      /// Typedef for the const workspace_iterator to use with a Workspace
      typedef workspace_iterator<const LocatedDataRef, const MatrixWorkspace> const_iterator;
      /// Initialize
      void initialize(const std::size_t &NVectors, const std::size_t &XLength, const std::size_t &YLength);
      /// Delete
      virtual ~MatrixWorkspace();
      
      /**@name Instrument queries */
      //@{
      Geometry::IDetector_const_sptr getDetector(const size_t workspaceIndex) const;
      double detectorTwoTheta(Geometry::IDetector_const_sptr det) const;
      double detectorSignedTwoTheta(Geometry::IDetector_const_sptr det) const;
      double gravitationalDrop(Geometry::IDetector_const_sptr det, const double waveLength) const;
      //@}

      virtual void populateInstrumentParameters();

      /** @name Nearest neighbours */
      /// Build and populate the NearestNeighbours object
      void buildNearestNeighbours(const bool ignoreMaskedDetectors=false) const;
      /// Causes the nearest neighbours map to be rebuilt
      void rebuildNearestNeighbours();
      /// Query the NearestNeighbours object for a detector
      std::map<specid_t, Mantid::Kernel::V3D> getNeighbours(const Geometry::IDetector *comp, const double radius = 0.0, const bool ignoreMaskedDetectors=false) const;
      /// Query the NearestNeighbours object for a given spectrum index using a search radius
      std::map<specid_t, Mantid::Kernel::V3D> getNeighbours(specid_t spec, const double radius, const bool ignoreMaskedDetectors=false) const;
      /// Query the NearestNeighbours object for a given spectrum index using the direct number of nearest neighbours
      std::map<specid_t, Mantid::Kernel::V3D> getNeighboursExact(specid_t spec, const int nNeighbours, const bool ignoreMaskedDetectors=false) const;
      //@}

      void updateSpectraUsing(const SpectrumDetectorMapping& map);
      /// Build the default spectra mapping, most likely wanted after an instrument update
      void rebuildSpectraMapping(const bool includeMonitors = true);

      // More mapping
      spec2index_map * getSpectrumToWorkspaceIndexMap() const;
      detid2index_map * getDetectorIDToWorkspaceIndexMap( bool throwIfMultipleDets=false ) const;
      void getDetectorIDToWorkspaceIndexVector( std::vector<size_t> & out, detid_t & offset, bool throwIfMultipleDets=false) const;
      void getSpectrumToWorkspaceIndexVector(std::vector<size_t> & out, specid_t & offset) const;
      void getIndicesFromSpectra(const std::vector<specid_t>& spectraList, std::vector<size_t>& indexList) const;
      size_t getIndexFromSpectrumNumber(const specid_t specNo) const;
      void getIndicesFromDetectorIDs(const std::vector<detid_t>& detIdList, std::vector<size_t>& indexList) const;
      void getSpectraFromDetectorIDs(const std::vector<detid_t>& detIdList, std::vector<specid_t>& spectraList) const;

	  bool hasGroupedDetectors() const;

      /// Get the footprint in memory in bytes.
      virtual size_t getMemorySize() const;
      virtual size_t getMemorySizeForXAxes() const;

      // Section required for iteration
      /// Returns the number of single indexable items in the workspace
      virtual std::size_t size() const = 0;
      /// Returns the size of each block of data returned by the dataY accessors
      virtual std::size_t blocksize() const = 0;
      /// Returns the number of histograms in the workspace
      virtual std::size_t getNumberHistograms() const = 0;

      /// Sets MatrixWorkspace title
      virtual void setTitle(const std::string&);
      /// Gets MatrixWorkspace title (same as Run object run_title property)
      virtual const std::string getTitle() const;

      Kernel::DateAndTime getFirstPulseTime() const;
      Kernel::DateAndTime getLastPulseTime() const;

      /// Returns the bin index for a given X value of a given workspace index
      size_t binIndexOf(const double xValue, const std::size_t = 0) const;

      //----------------------------------------------------------------------
      // DATA ACCESSORS
      //----------------------------------------------------------------------

      /// Return the underlying ISpectrum ptr at the given workspace index.
      virtual ISpectrum * getSpectrum(const size_t index) = 0;

      /// Return the underlying ISpectrum ptr (const version) at the given workspace index.
      virtual const ISpectrum * getSpectrum(const size_t index) const = 0;

      // Methods for getting read-only access to the data.
      // Just passes through to the virtual dataX/Y/E function (const version)
      /// Returns a read-only (i.e. const) reference to the specified X array
      /// @param index :: workspace index to retrieve.
      const MantidVec& readX(std::size_t const index) const { return getSpectrum(index)->dataX(); }
      /// Returns a read-only (i.e. const) reference to the specified Y array
      /// @param index :: workspace index to retrieve.
      const MantidVec& readY(std::size_t const index) const { return getSpectrum(index)->dataY(); }
      /// Returns a read-only (i.e. const) reference to the specified E array
      /// @param index :: workspace index to retrieve.
      const MantidVec& readE(std::size_t const index) const { return getSpectrum(index)->dataE(); }
      /// Returns a read-only (i.e. const) reference to the specified X error array
      /// @param index :: workspace index to retrieve.
      const MantidVec& readDx(size_t const index) const { return getSpectrum(index)->dataDx(); }

      /// Returns the x data
      virtual MantidVec& dataX(const std::size_t index) { return getSpectrum(index)->dataX(); }
      /// Returns the y data
      virtual MantidVec& dataY(const std::size_t index) { return getSpectrum(index)->dataY(); }
      /// Returns the error data
      virtual MantidVec& dataE(const std::size_t index) { return getSpectrum(index)->dataE(); }
      /// Returns the x error data
      virtual MantidVec& dataDx(const std::size_t index) { return getSpectrum(index)->dataDx(); }

      /// Returns the x data const
      virtual const MantidVec& dataX(const std::size_t index) const { return getSpectrum(index)->dataX(); }
      /// Returns the y data const
      virtual const MantidVec& dataY(const std::size_t index) const { return getSpectrum(index)->dataY(); }
      /// Returns the error const
      virtual const MantidVec& dataE(const std::size_t index) const { return getSpectrum(index)->dataE(); }
      /// Returns the error const
      virtual const MantidVec& dataDx(const std::size_t index) const { return getSpectrum(index)->dataDx(); }

      virtual double getXMin() const;
      virtual double getXMax() const;
      virtual void getXMinMax(double &xmin, double &xmax) const;

      /// Returns a pointer to the x data
      virtual Kernel::cow_ptr<MantidVec> refX(const std::size_t index) const { return getSpectrum(index)->ptrX(); }

      /// Set the specified X array to point to the given existing array
      virtual void setX(const std::size_t index, const MantidVec& X) { getSpectrum(index)->setX(X); }

      /// Set the specified X array to point to the given existing array
      virtual void setX(const std::size_t index, const MantidVecPtr& X) { getSpectrum(index)->setX(X); }

      /// Set the specified X array to point to the given existing array
      virtual void setX(const std::size_t index, const MantidVecPtr::ptr_type& X)  { getSpectrum(index)->setX(X); }

      /** Sets the data in the workspace
      @param index :: the workspace index to set.
      @param Y :: Y vector  */
      virtual void setData(const std::size_t index, const MantidVecPtr& Y)
      { getSpectrum(index)->setData(Y); }

      /** Sets the data in the workspace
      @param index :: the workspace index to set.
      @param Y :: Y vector
      @param E :: Error vector   */
      virtual void setData(const std::size_t index, const MantidVecPtr& Y, const MantidVecPtr& E)
      { getSpectrum(index)->setData(Y,E); }

      /** Sets the data in the workspace
      @param index :: the workspace index to set.
      @param Y :: Y vector
      @param E :: Error vector   */
      virtual void setData(const std::size_t index, const MantidVecPtr::ptr_type& Y, const MantidVecPtr::ptr_type& E)
      { getSpectrum(index)->setData(Y,E); }

      /// Generate the histogram or rebin the existing histogram.
      virtual void generateHistogram(const std::size_t index, const MantidVec& X, MantidVec& Y, MantidVec& E, bool skipError = false) const = 0;


      /// Return a vector with the integrated counts for all spectra withing the given range
      virtual void getIntegratedSpectra(std::vector<double> & out, const double minX, const double maxX, const bool entireRange) const;

      //----------------------------------------------------------------------

      int axes() const;
      Axis* getAxis(const std::size_t& axisIndex) const;
      void replaceAxis(const std::size_t& axisIndex, Axis* const newAxis);

      /// Returns true if the workspace contains data in histogram form (as opposed to point-like)
      virtual bool isHistogramData() const;

      std::string YUnit() const;
      void setYUnit(const std::string& newUnit);
      std::string YUnitLabel() const;
      void setYUnitLabel(const std::string& newLabel);

      /// Are the Y-values dimensioned?
      const bool& isDistribution() const;
      bool& isDistribution(bool newValue);

      /// Mask a given workspace index, setting the data and error values to zero
      void maskWorkspaceIndex(const std::size_t index);

      // Methods to set and access masked bins
      void maskBin(const size_t& workspaceIndex, const size_t& binIndex, const double& weight = 1.0);
      void flagMasked(const size_t& spectrumIndex, const size_t& binIndex, const double& weight = 1.0);
      bool hasMaskedBins(const size_t& spectrumIndex) const;
      /// Masked bins for each spectrum are stored as a set of pairs containing <bin index, weight>
      typedef std::map<size_t,double> MaskList;
      const MaskList& maskedBins(const size_t& spectrumIndex) const;

      void saveInstrumentNexus(::NeXus::File * file) const;
      void loadInstrumentNexus(::NeXus::File * file);
       void saveSpectraMapNexus(::NeXus::File * file, const std::vector<int>& spec,
          const ::NeXus::NXcompression compression = ::NeXus::LZW) const;

      //=====================================================================================
      // MD Geometry methods
      //=====================================================================================
      virtual size_t getNumDims() const;
      virtual boost::shared_ptr<const Mantid::Geometry::IMDDimension> getDimension(size_t index) const;
      virtual boost::shared_ptr<const Mantid::Geometry::IMDDimension> getDimensionWithId(std::string id) const;
      //=====================================================================================
      // End MD Geometry methods
      //=====================================================================================

      //=====================================================================================
      // IMDWorkspace methods
      //=====================================================================================

      /// Gets the number of points available on the workspace.
      virtual uint64_t getNPoints() const;
      /// Get the number of points available on the workspace.
      virtual uint64_t getNEvents() const{return this->getNPoints();}
      /// Dimension id for x-dimension.
      static const std::string xDimensionId;
      /// Dimensin id for y-dimension.
      static const std::string yDimensionId;
      /// Generate a line plot through the matrix workspace.
      virtual void getLinePlot(const Mantid::Kernel::VMD & start, const Mantid::Kernel::VMD & end,
          Mantid::API::MDNormalization normalize, std::vector<coord_t> & x, std::vector<signal_t> & y, std::vector<signal_t> & e) const;
      /// Get the signal at a coordinate in the workspace.
      virtual signal_t getSignalAtCoord(const coord_t * coords, const Mantid::API::MDNormalization & normalization) const;
      /// Create iterators. Partitions the iterators according to the number of cores.
      virtual std::vector<IMDIterator*> createIterators(size_t suggestedNumCores = 1,
          Mantid::Geometry::MDImplicitFunction * function = NULL) const;

       /// Apply masking.
       void setMDMasking(Mantid::Geometry::MDImplicitFunction* maskingRegion);

       /// Clear exsting masking.
       void clearMDMasking();

       /// @return the special coordinate system used if any.
       virtual Mantid::API::SpecialCoordinateSystem getSpecialCoordinateSystem() const;

      //=====================================================================================
      // End IMDWorkspace methods
      //=====================================================================================

    protected:
      MatrixWorkspace(Mantid::Geometry::INearestNeighboursFactory* factory = NULL);

      /// Initialises the workspace. Sets the size and lengths of the arrays. Must be overloaded.
      virtual void init(const std::size_t &NVectors, const std::size_t &XLength, const std::size_t &YLength) = 0;

      /// A vector of pointers to the axes for this workspace
      std::vector<Axis*> m_axes;

    private:
      /// Private copy constructor. NO COPY ALLOWED
      MatrixWorkspace(const MatrixWorkspace&);
      /// Private copy assignment operator. NO ASSIGNMENT ALLOWED
      MatrixWorkspace& operator=(const MatrixWorkspace&);

      /// Has this workspace been initialised?
      bool m_isInitialized;

      /// The unit for the data values (e.g. Counts)
      std::string m_YUnit;
      /// A text label for use when plotting spectra
      std::string m_YUnitLabel;
      /// Flag indicating whether the Y-values are dimensioned. False by default
      bool m_isDistribution;
      /// The set of masked bins in a map keyed on spectrum index
      std::map< int64_t, MaskList > m_masks;

    protected:
      /// Assists conversions to and from 2D histogram indexing to 1D indexing.
      MatrixWSIndexCalculator m_indexCalculator;

      /// Scoped pointer to NearestNeighbours factory
      boost::scoped_ptr<Mantid::Geometry::INearestNeighboursFactory> m_nearestNeighboursFactory;

      /// Shared pointer to NearestNeighbours object
      mutable boost::shared_ptr<Mantid::Geometry::INearestNeighbours> m_nearestNeighbours;

      /// Static reference to the logger class
      static Kernel::Logger& g_log;

      /// Getter for the dimension id based on the axis.
      std::string getDimensionIdFromAxis(const int& axisIndex) const;

    };

    ///shared pointer to the matrix workspace base class
    typedef boost::shared_ptr<MatrixWorkspace> MatrixWorkspace_sptr;
    ///shared pointer to the matrix workspace base class (const version)
    typedef boost::shared_ptr<const MatrixWorkspace> MatrixWorkspace_const_sptr;

  } // namespace API
} // namespace Mantid



#endif /*MANTID_API_MATRIXWORKSPACE_H_*/
