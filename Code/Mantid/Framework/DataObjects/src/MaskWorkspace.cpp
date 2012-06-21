#include "MantidDataObjects/MaskWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidAPI/WorkspaceFactory.h"

namespace Mantid
{
namespace DataObjects
{

    //Register the workspace
    DECLARE_WORKSPACE(MaskWorkspace)

    namespace { // keep these constants only within this file.
    /// The only value allowed for a pixel to be kept.
    const double LIVE_VALUE = 0.;

    /**
     * The default value for marking a pixel to be masked. For checks
     * anything that isn't live is dead.
     */
    const double DEAD_VALUE = 1.;

    /// The value for uncertainty.
    const double ERROR_VALUE = 0.;
    }

    //--------------------------------------------------------------------------

    /**
     * Constructor - Default.
     * @return MaskWorkspace
     */
    MaskWorkspace::MaskWorkspace(): m_hasInstrument(false)
    {
    }

    /**
     * Constructor - with a given dimension.
     * @param[in] numvectors Number of vectors/histograms for this workspace.
     * @return MaskWorkspace
     */
    MaskWorkspace::MaskWorkspace(std::size_t numvectors): m_hasInstrument(false)
    {
      this->init(numvectors, 1, 1);
      this->clearMask();
    }

    /**
     * Constructor - using an instrument.
     * @param[in] instrument Instrument that is the base for this workspace.
     * @return MaskWorkspace
     */
    MaskWorkspace::MaskWorkspace(Mantid::Geometry::Instrument_const_sptr instrument, const bool includeMonitors)
      : SpecialWorkspace2D(instrument, includeMonitors), m_hasInstrument(true)
    {
      this->clearMask();
    }

    //--------------------------------------------------------------------------

    /**
     * Destructor
     */
    MaskWorkspace::~MaskWorkspace()
    {

    }

    //--------------------------------------------------------------------------

    void MaskWorkspace::clearMask()
    {
      std::size_t nHist = this->getNumberHistograms();
      for (std::size_t i = 0; i < nHist; ++i)
      {
        this->dataY(i)[0] = LIVE_VALUE;
        this->dataE(i)[0] = ERROR_VALUE;
      }

      // Clear the mask flags
      Geometry::ParameterMap & pmap = this->instrumentParameters();
      pmap.clearParametersByName("masked");
    }

    /**
     * @return The total number of masked spectra.
     */
    std::size_t MaskWorkspace::getNumberMasked() const
    {
      std::size_t numMasked(0);
      const std::size_t numWksp(this->getNumberHistograms());
      for (std::size_t i = 0; i < numWksp; i++)
      {
        if (this->dataY(i)[0] != 0.) // quick check the value
        {
          numMasked++;
        }
        else if (m_hasInstrument)
        {
          if (this->isMasked(this->getDetectorID(i))) // slow and correct check with the real method
              numMasked++;
        }
      }
      return numMasked;
    }

    /**
     * @return True if the data should be deleted.
     */
    bool MaskWorkspace::isMasked(const detid_t detectorID) const
    {
      if (!m_hasInstrument)
        throw std::runtime_error("There is no instrument associated with the workspace");

      // return true if the value isn't zero
      if (this->getValue(detectorID, 0.) != LIVE_VALUE)
      {
        return true;
      }

      // the mask bit on the workspace can be set
      return this->getInstrument()->isDetectorMasked(detectorID);
    }

    /**
     * @return True if the data should be deleted.
     */
    bool MaskWorkspace::isMasked(const std::set<detid_t> &detectorIDs) const
    {
      if (detectorIDs.empty())
      {
        return false;
      }

      bool masked(true);
      for (std::set<detid_t>::const_iterator it = detectorIDs.begin(); it != detectorIDs.end(); ++it)
      {
        if (!this->isMasked(*it))
        {
          masked = false;
          break; // allows space for a debug print statement
        }
      }
      return masked;
    }

    /**
     * Use this method with MaskWorkspace that doesn't have an instrument.
     */
    bool MaskWorkspace::isMaskedIndex(const std::size_t wkspIndex) const
    {
      return (this->dataY(wkspIndex)[0] != LIVE_VALUE); // if is not live it should masked
    }

    /**
     * Mask an individual pixel.
     *
     * @param detectorID to mask.
     * @param mask True means to delete the data.
     */
    void MaskWorkspace::setMasked(const detid_t detectorID, const bool mask)
    {
      double value(LIVE_VALUE);
      if (mask)
        value = DEAD_VALUE;

      this->setValue(detectorID, value, ERROR_VALUE);
    }

    /**
     * Mask a set of pixels. This is a convenience function to
     * call @link MaskWorkspace::setMasked(const detid_t, const bool).
     */
    void MaskWorkspace::setMasked(const std::set<detid_t> &detectorIDs, const bool mask)
    {
      for (auto detId = detectorIDs.begin(); detId != detectorIDs.end(); ++detId)
      {
        this->setMasked(*detId, mask);
      }
    }

    void MaskWorkspace::setMaskedIndex(const std::size_t wkspIndex, const bool mask)
    {
      double value(LIVE_VALUE);
      if (mask)
        value = DEAD_VALUE;

      this->dataY(wkspIndex)[0] = value;
    }

    /**
     * Gets the name of the workspace type.
     * @return Standard string name
     */
    const std::string MaskWorkspace::id() const
    {
        return "MaskWorkspace";
    }

} //namespace DataObjects
} //namespace Mantid


///\cond TEMPLATE

namespace Mantid
{
  namespace Kernel
  {

    template<> DLLExport
    Mantid::DataObjects::MaskWorkspace_sptr IPropertyManager::getValue<Mantid::DataObjects::MaskWorkspace_sptr>(const std::string &name) const
    {
      PropertyWithValue<Mantid::DataObjects::MaskWorkspace_sptr>* prop =
        dynamic_cast<PropertyWithValue<Mantid::DataObjects::MaskWorkspace_sptr>*>(getPointerToProperty(name));
      if (prop)
      {
        return *prop;
      }
      else
      {
        std::string message = "Attempt to assign property "+ name +" to incorrect type. Expected MaskWorkspace.";
        throw std::runtime_error(message);
      }
    }

    template<> DLLExport
    Mantid::DataObjects::MaskWorkspace_const_sptr IPropertyManager::getValue<Mantid::DataObjects::MaskWorkspace_const_sptr>(const std::string &name) const
    {
      PropertyWithValue<Mantid::DataObjects::MaskWorkspace_sptr>* prop =
        dynamic_cast<PropertyWithValue<Mantid::DataObjects::MaskWorkspace_sptr>*>(getPointerToProperty(name));
      if (prop)
      {
        return prop->operator()();
      }
      else
      {
        std::string message = "Attempt to assign property "+ name +" to incorrect type. Expected const MaskWorkspace.";
        throw std::runtime_error(message);
      }
    }

  } // namespace Kernel
} // namespace Mantid

///\endcond TEMPLATE
