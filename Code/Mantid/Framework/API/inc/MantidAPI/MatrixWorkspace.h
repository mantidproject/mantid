#ifndef MANTID_API_MATRIXWORKSPACE_H_
#define MANTID_API_MATRIXWORKSPACE_H_ 

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidAPI/MatrixWSIndexCalculator.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceIterator.h"
#include "MantidAPI/Axis.h"
#include "MantidGeometry/Instrument/Instrument.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/cow_ptr.h"
#include <boost/shared_ptr.hpp>
#include <set>

namespace Mantid
{
  //Forward Decs
  namespace Geometry
  {
    class MDCell;
    class MDPoint;
    class ParameterMap;
  }

  namespace API
  {

    /** Map from one type of index (e.g. workspace index) to another type (e.g. spectrum # or detector id #).
    * Used by MatrixWorkspace to return maps.
    */
    typedef std::map<int64_t, int64_t> IndexToIndexMap;

    // Map for associating indexes to generated MDPoints.
    typedef std::map<int64_t, Mantid::Geometry::MDPoint> MatrixMDPointMap;

    //----------------------------------------------------------------------
    // Forward Declaration
    //----------------------------------------------------------------------
    class SpectraDetectorMap;
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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
    */
    
    class DLLExport MatrixWorkspace : public IMDWorkspace //, public Workspace
    {
    public:

      // The Workspace Factory create-from-parent method needs direct access to the axes.
      friend class WorkspaceFactoryImpl;

      /// Typedef for the workspace_iterator to use with a Workspace
      typedef workspace_iterator<LocatedDataRef, MatrixWorkspace> iterator;
      /// Typedef for the const workspace_iterator to use with a Workspace
      typedef workspace_iterator<const LocatedDataRef, const MatrixWorkspace> const_iterator;

      void initialize(const std::size_t &NVectors, const std::size_t &XLength, const std::size_t &YLength);
      virtual ~MatrixWorkspace();

      void setInstrument(const Geometry::IInstrument_sptr&);
      Geometry::IInstrument_sptr getInstrument() const;
      boost::shared_ptr<Geometry::Instrument> getBaseInstrument()const;

      // SpectraDetectorMap accessors
      const SpectraDetectorMap& spectraMap() const;
      virtual SpectraDetectorMap& mutableSpectraMap();

      // More mapping
      IndexToIndexMap * getWorkspaceIndexToSpectrumMap() const;
      IndexToIndexMap * getSpectrumToWorkspaceIndexMap() const;
      IndexToIndexMap * getWorkspaceIndexToDetectorIDMap() const;
      IndexToIndexMap * getDetectorIDToWorkspaceIndexMap( bool throwIfMultipleDets ) const;
      void getIndicesFromSpectra(const std::vector<int64_t>& spectraList, std::vector<int64_t>& indexList) const;

      /// Sample accessors
      const  Sample& sample() const;
      Sample& mutableSample();

      /// Run details object access
      const Run & run() const;
      /// Writable version of the run object
      Run& mutableRun();

      /// Get a detector object (Detector or DetectorGroup) for the given spectrum index
      Geometry::IDetector_sptr getDetector(const int64_t index) const;
      double detectorTwoTheta(Geometry::IDetector_const_sptr det) const;
      /// Calculates the drop of a neutron coming from the sample, there isn't currently a Mantid convention for which axis is vertical
      double gravitationalDrop(Geometry::IDetector_const_sptr det, const double waveLength) const;

      /// Get the footprint in memory in bytes.
      virtual size_t getMemorySize() const;
      virtual size_t getMemorySizeForXAxes() const;

      /// Returns the set of parameters modifying the base instrument
      const Geometry::ParameterMap& instrumentParameters() const;
      Geometry::ParameterMap& instrumentParameters();
      /// Const version
      const Geometry::ParameterMap& constInstrumentParameters() const;
      // Add parameters to the instrument parameter map
      void populateInstrumentParameters();

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

      /// Returns the bin index for a given X value of a given workspace index
      size_t binIndexOf(const double xValue, const std::size_t = 0) const;

      //----------------------------------------------------------------------
      // DATA ACCESSORS
      //----------------------------------------------------------------------
      // Methods for getting read-only access to the data.
      // Just passes through to the virtual dataX/Y/E function (const version)

      /// Returns a read-only (i.e. const) reference to the specified X array
      /// @param index :: workspace index to retrieve.
      const MantidVec& readX(std::size_t const index) const { return dataX(index); }
      /// Returns a read-only (i.e. const) reference to the specified Y array
      /// @param index :: workspace index to retrieve.
      const MantidVec& readY(std::size_t const index) const { return dataY(index); }
      /// Returns a read-only (i.e. const) reference to the specified E array
      /// @param index :: workspace index to retrieve.
      const MantidVec& readE(std::size_t const index) const { return dataE(index); }
      /// Returns a read-only (i.e. const) reference to the specified X error array
      /// @param index :: workspace index to retrieve.
      const MantidVec& readDx(int const index) const { return dataDx(index); }

      /** Returns a read-only (i.e. const) reference to both the Y
       * and E arrays
       * @param index :: workspace index to retrieve.
       * @param[out] Y :: reference to the pointer to the const data vector
       * @param[out] E :: reference to the pointer to the const error vector
       */
      virtual void readYE(std::size_t const index, MantidVec const*& Y, MantidVec const*& E) const
      {
        Y = &dataY(index);
        E = &dataE(index);
      }

      /// Returns the x data
      virtual MantidVec& dataX(const std::size_t index) = 0;
      /// Returns the y data
      virtual MantidVec& dataY(const std::size_t index) = 0;
      /// Returns the error data
      virtual MantidVec& dataE(const std::size_t index) = 0;
      /// Returns the x error data
      virtual MantidVec& dataDx(const std::size_t index) = 0;
      /// Returns the x data const
      virtual const MantidVec& dataX(const std::size_t index) const = 0;
      /// Returns the y data const
      virtual const MantidVec& dataY(const std::size_t index) const = 0;
      /// Returns the error const
      virtual const MantidVec& dataE(const std::size_t index) const = 0;
      /// Returns the error const
      virtual const MantidVec& dataDx(const std::size_t index) const = 0;

      /// Returns a pointer to the x data
      virtual Kernel::cow_ptr<MantidVec> refX(const std::size_t index) const = 0;
      /// Set the specified X array to point to the given existing array
      virtual void setX(const std::size_t index, const Kernel::cow_ptr<MantidVec>& X) = 0;

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

      /// Mask a given workspace index, setting the data and error values to the given value
      void maskWorkspaceIndex(const std::size_t index, const double maskValue = 0.0);

      // Methods to set and access masked bins
      void maskBin(const int64_t& spectrumIndex, const int64_t& binIndex, const double& weight = 1.0);
      bool hasMaskedBins(const int64_t& spectrumIndex) const;
      /// Masked bins for each spectrum are stored as a set of pairs containing <bin index, weight>
      typedef std::set< std::pair<int64_t,double> > MaskList;
      const MaskList& maskedBins(const int64_t& spectrumIndex) const;

      /// Gets the number of points available on the workspace.
      virtual uint64_t getNPoints() const;

      /// Get the number of dimensions
      virtual size_t getNumDims() const;

      /// Get the x-dimension mapping.
      virtual boost::shared_ptr<const Mantid::Geometry::IMDDimension> getXDimension() const;

      /// Get the y-dimension mapping.
      virtual boost::shared_ptr<const Mantid::Geometry::IMDDimension> getYDimension() const;

      /// Get the z-dimension mapping.
      virtual boost::shared_ptr<const Mantid::Geometry::IMDDimension> getZDimension() const;

      /// Get the t-dimension mapping.
      virtual boost::shared_ptr<const Mantid::Geometry::IMDDimension> getTDimension() const;

      /// Get the dimension with the specified id.
      virtual boost::shared_ptr<const Mantid::Geometry::IMDDimension> getDimension(std::string id) const;

      /// Get the dimension ids in their order
      virtual const std::vector<std::string> getDimensionIDs() const;

      /// Get the point at the specified index.
      virtual const Mantid::Geometry::SignalAggregate& getPoint(size_t index) const;

      /// Get the cell at the specified index/increment.
      virtual const Mantid::Geometry::SignalAggregate& getCell(size_t dim1Increment) const;

      /// Get the cell at the specified index/increment.
      virtual const Mantid::Geometry::SignalAggregate& getCell(size_t dim1Increment, size_t dim2Increment) const;

      /// Get the cell at the specified index/increment.
      virtual const Mantid::Geometry::SignalAggregate& getCell(size_t dim1Increment, size_t dim2Increment, size_t dim3Increment) const;

      /// Get the cell at the specified index/increment.
      virtual const Mantid::Geometry::SignalAggregate& getCell(size_t dim1Increment, size_t dim2Increment, size_t dim3Increment, size_t dim4Increment) const;

      /// Get the cell at the specified index/increment.
      virtual const Mantid::Geometry::SignalAggregate& getCell(...) const;

      /// Provide the location of the underlying file. 
      virtual std::string getWSLocation() const;

      /// Provide the underlying xml for 
      virtual std::string getGeometryXML() const;

      /// Dimension id for x-dimension.
      static const std::string xDimensionId;

      /// Dimensin id for y-dimension.
      static const std::string yDimensionId;

    protected:
      MatrixWorkspace();

      /// Initialises the workspace. Sets the size and lengths of the arrays. Must be overloaded.
      virtual void init(const std::size_t &NVectors, const std::size_t &XLength, const std::size_t &YLength) = 0;

      /// A vector of pointers to the axes for this workspace
      std::vector<Axis*> m_axes;

    private:

      /// Implementation of getMDPointImp taking two arguments for histogram and bin.
      const Mantid::Geometry::SignalAggregate& getPointImp(size_t histogram, size_t bin) const;

      /// Creates a point for a given histogram/bin.
      Mantid::Geometry::MDPoint createPoint(HistogramIndex histogram, BinIndex bin) const;
     
      /// Private copy constructor. NO COPY ALLOWED
      MatrixWorkspace(const MatrixWorkspace&);
      /// Private copy assignment operator. NO ASSIGNMENT ALLOWED
      MatrixWorkspace& operator=(const MatrixWorkspace&);

      /// Has this workspace been initialised?
      bool m_isInitialized;

      /// The instrument used for this experiment
      boost::shared_ptr<Geometry::Instrument> sptr_instrument;

    protected:
      /// The SpectraDetector table used for this experiment. Inside a copy-on-write pointer.
      Kernel::cow_ptr<SpectraDetectorMap> m_spectramap;

      /// The information on the sample environment
      Kernel::cow_ptr<Sample> m_sample;

      /// The run information
      Kernel::cow_ptr<Run> m_run;

    private:
      /// The unit for the data values (e.g. Counts)
      std::string m_YUnit;
      /// A text label for use when plotting spectra
      std::string m_YUnitLabel;
      /// Flag indicating whether the Y-values are dimensioned. False by default
      bool m_isDistribution;

      /// Parameters modifying the base instrument
      boost::shared_ptr<Geometry::ParameterMap> m_parmap;

      /// The set of masked bins in a map keyed on spectrum index
      std::map< int64_t, MaskList > m_masks;

      /// Associates indexes to MDPoints. Dynamic cache.
      mutable MatrixMDPointMap m_mdPointMap;

      /// Assists conversions to and from 2D histogram indexing to 1D indexing.
      MatrixWSIndexCalculator m_indexCalculator;

      /// Used for storing info about "r-position", "t-position" and "p-position" parameters
      /// as all parameters are processed  
      struct m_PositionEntry 
      { m_PositionEntry(std::string& name, double val) : paramName(name), value(val) {} 
        std::string paramName; 
        double value; };

      /// Static reference to the logger class
      static Kernel::Logger& g_log;

      std::string getDimensionIdFromAxis(const int& axisIndex) const;

    };

    ///shared pointer to the matrix workspace base class
    typedef boost::shared_ptr<MatrixWorkspace> MatrixWorkspace_sptr;
    ///shared pointer to the matrix workspace base class (const version)
    typedef boost::shared_ptr<const MatrixWorkspace> MatrixWorkspace_const_sptr;

  } // namespace API
} // namespace Mantid



#endif /*MANTID_API_MATRIXWORKSPACE_H_*/
