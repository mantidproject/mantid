#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/RefAxis.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidAPI/LocatedDataRef.h"
#include "MantidAPI/WorkspaceIterator.h"
#include "MantidAPI/WorkspaceIteratorCode.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceFactory.h"

namespace Mantid
{
  namespace DataObjects
  {
    using std::size_t;

    DECLARE_WORKSPACE(Workspace2D)

    // Get a reference to the logger
    Kernel::Logger& Workspace2D::g_log = Kernel::Logger::get("Workspace2D");

    /// Constructor
    Workspace2D::Workspace2D()
    {}

    ///Destructor
    Workspace2D::~Workspace2D()
    {}

    /** Sets the size of the workspace and initializes arrays to zero
    *  @param NVectors :: The number of vectors/histograms/detectors in the workspace
    *  @param XLength :: The number of X data points/bin boundaries in each vector (must all be the same)
    *  @param YLength :: The number of data/error points in each vector (must all be the same)
    */
    void Workspace2D::init(const size_t &NVectors, const size_t &XLength, const size_t &YLength)
    {
      m_noVectors = NVectors;
      data.resize(m_noVectors);
      m_axes.resize(2);
      m_axes[0] = new API::RefAxis(XLength, this);
      // This axis is always a spectra one for now
      m_axes[1] = new API::SpectraAxis(m_noVectors);

      MantidVecPtr t1,t2;
      t1.access().resize(XLength); //this call initializes array to zero
      t2.access().resize(YLength);
      for (size_t i=0;i<m_noVectors;i++)
      {
        this->setX(i,t1,t1);
        // Y,E arrays populated
        this->setData(i,t2,t2);
      }
    }

    /**
    Set the x values
    @param histnumber :: Index to the histogram
    @param Vec :: Shared ptr base object
    */
    void Workspace2D::setX(const size_t histnumber, const MantidVecPtr::ptr_type& Vec)
    {
    
      if (histnumber>=m_noVectors)
        throw std::range_error("Workspace2D::setX, histogram number out of range");
     
      data[histnumber].setX(Vec);
      
      return;
    }

    /**
    Set the x values and errors
    @param histnumber :: Index to the histogram
    @param Vec :: Shared ptr base object
    */
    void Workspace2D::setX(const size_t histnumber, const MantidVecPtr::ptr_type& Vec, const MantidVecPtr::ptr_type& Err)
    {

      if (histnumber>=m_noVectors)
        throw std::range_error("Workspace2D::setX, histogram number out of range");

      data[histnumber].setX(Vec, Err);

      return;
    }

    /**
    Set the x values
    @param histnumber :: Index to the histogram
    @param PA :: Reference counted histogram
    */
    void Workspace2D::setX(const size_t histnumber, const MantidVecPtr& PA)
    {
      if (histnumber>=m_noVectors)
        throw std::range_error("Workspace2D::setX, histogram number out of range");
      
      data[histnumber].setX(PA);

      return;
    }

    /**
    Set the x values and errors
    @param histnumber :: Index to the histogram
    @param PA :: Reference counted histogram
    */
    void Workspace2D::setX(const size_t histnumber, const MantidVecPtr& PA, const MantidVecPtr& Err)
    {
      if (histnumber>=m_noVectors)
        throw std::range_error("Workspace2D::setX, histogram number out of range");

      data[histnumber].setX(PA, Err);

      return;
    }

    /**
    Sets the data in the workspace
    @param histnumber :: The histogram to be set
    @param PY :: A reference counted data range
    */
    void Workspace2D::setData(const size_t histnumber, const MantidVecPtr& PY)
    {
      if (histnumber>=m_noVectors)
        throw std::range_error("Workspace2D::setData, histogram number out of range");

      data[histnumber].setData(PY);
    }

    /**
    Sets the data in the workspace
    @param histnumber :: The histogram to be set
    @param PY :: A reference counted data range
    @param PE :: A reference containing the corresponding errors
    */
    void Workspace2D::setData(const size_t histnumber, const MantidVecPtr& PY,
      const MantidVecPtr& PE)
    {
      if (histnumber>=m_noVectors)
        throw std::range_error("Workspace2D::setData, histogram number out of range");

      data[histnumber].setData(PY,PE);
      return;
    }

    /**
    Sets the data in the workspace
    @param histnumber :: The histogram to be set
    @param PY :: A reference counted data range
    @param PE :: A reference containing the corresponding errors
    */
    void Workspace2D::setData(const size_t histnumber, const MantidVecPtr::ptr_type& PY,
      const MantidVecPtr::ptr_type& PE)
    {
      if (histnumber>=m_noVectors)
        throw std::range_error("Workspace2D::setData, histogram number out of range");

      data[histnumber].setData(PY,PE);
      return;
    }

    /** Gets the number of histograms
    @return Integer
    */
    size_t Workspace2D::getNumberHistograms() const
    {
      return getHistogramNumberHelper();
    }

    /**
    Get the x data of a specified histogram
    @param index :: The number of the histogram
    @return A vector of doubles containing the x data
    */
    const MantidVec& Workspace2D::dataX(const size_t index) const
    {
      if (index>=m_noVectors)
        throw std::range_error("Workspace2D::dataX, histogram number out of range");

      return data[index].dataX();
    }

    /**
    Get the y data of a specified histogram
    @param index :: The number of the histogram
    @return A vector of doubles containing the y data
    */
    const MantidVec& Workspace2D::dataY(const size_t index) const
    {
      if (index>=m_noVectors)
      {
        std::stringstream msg;
        msg << "Workspace2D::dataY, histogram number " << index << " out of range.";
        throw std::range_error(msg.str());
      }
      return data[index].dataY();
    }

    /**
    Get the error data for a specified histogram
    @param index :: The number of the histogram
    @return A vector of doubles containing the error data
    */
    const MantidVec& Workspace2D::dataE(const size_t index) const
    {
      if (index>=m_noVectors)
        throw std::range_error("Workspace2D::dataE, histogram number out of range");

      return data[index].dataE();
    }

    /**
    Get the error x data for a specified histogram
    @param index :: The number of the histogram
    @return A vector of doubles containing the error x data
    */
    const MantidVec& Workspace2D::dataDx(const size_t index) const
    {
      if (index>=m_noVectors)
        throw std::range_error("Workspace2D::dataDx, histogram number out of range");

      return data[index].dataDx();
    }

    /// get pseudo size
    size_t Workspace2D::size() const
    {
      return data.size() * blocksize();
    }

    ///get the size of each vector
    size_t Workspace2D::blocksize() const
    {
      return (data.size() > 0) ? data[0].size() : 0;
    }

    ///Returns the x data
    MantidVec& Workspace2D::dataX(const size_t index)
    {
      if (index>=m_noVectors)
        throw std::range_error("Workspace2D::dataX, histogram number out of range");

      return data[index].dataX();
    }

    ///Returns the y data
    MantidVec& Workspace2D::dataY(const size_t index)
    {
      if (index>=m_noVectors)
      {
        std::stringstream msg;
        msg << "Workspace2D::dataY, histogram number " << index << " out of range.";
        throw std::range_error(msg.str());
      }
      return data[index].dataY();
    }

    ///Returns the error data
    MantidVec& Workspace2D::dataE(const size_t index)
    {
      if (index>=m_noVectors)
        throw std::range_error("Workspace2D::dataE, histogram number out of range");

      return data[index].dataE();
    }
    
    ///Returns the error x data
    MantidVec& Workspace2D::dataDx(const size_t index)
    {
      if (index>=m_noVectors)
        throw std::range_error("Workspace2D::dataDx, histogram number out of range");

      return data[index].dataDx();
    }

    /// Returns a pointer to the x data
    Kernel::cow_ptr<MantidVec> Workspace2D::refX(const size_t index) const
    {
      if (index>=m_noVectors)
        throw std::range_error("Workspace2D::refX, histogram number out of range");

      return data[index].ptrX();      
    }

    /** Returns the number of histograms.
     *  For some reason Visual Studio couldn't deal with the main getHistogramNumber() method
     *  being virtual so it now just calls this private (and virtual) method which does the work.
     *  @return the number of histograms associated with the workspace
     */
    size_t Workspace2D::getHistogramNumberHelper() const
    {
      return data.size();
    }

  } // namespace DataObjects
} //NamespaceMantid






///\cond TEMPLATE
template DLLExport class Mantid::API::workspace_iterator<Mantid::API::LocatedDataRef, Mantid::DataObjects::Workspace2D>;
template DLLExport class Mantid::API::workspace_iterator<const Mantid::API::LocatedDataRef, const Mantid::DataObjects::Workspace2D>;

template DLLExport class Mantid::API::WorkspaceProperty<Mantid::DataObjects::Workspace2D>;

namespace Mantid
{
  namespace Kernel
  {
    template<> DLLExport
    Mantid::DataObjects::Workspace2D_sptr IPropertyManager::getValue<Mantid::DataObjects::Workspace2D_sptr>(const std::string &name) const
    {
      PropertyWithValue<Mantid::DataObjects::Workspace2D_sptr>* prop =
        dynamic_cast<PropertyWithValue<Mantid::DataObjects::Workspace2D_sptr>*>(getPointerToProperty(name));
      if (prop)
      {
        return *prop;
      }
      else
      {
        std::string message = "Attempt to assign property "+ name +" to incorrect type. Expected Workspace2D.";
        throw std::runtime_error(message);
      }
    }

    template<> DLLExport
    Mantid::DataObjects::Workspace2D_const_sptr IPropertyManager::getValue<Mantid::DataObjects::Workspace2D_const_sptr>(const std::string &name) const
    {
      PropertyWithValue<Mantid::DataObjects::Workspace2D_sptr>* prop =
        dynamic_cast<PropertyWithValue<Mantid::DataObjects::Workspace2D_sptr>*>(getPointerToProperty(name));
      if (prop)
      {
        return prop->operator()();
      }
      else
      {
        std::string message = "Attempt to assign property "+ name +" to incorrect type. Expected const Workspace2D.";
        throw std::runtime_error(message);
      }
    }
  } // namespace Kernel
} // namespace Mantid

///\endcond TEMPLATE
