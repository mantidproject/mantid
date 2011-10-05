#include "MDDataObjects/MDFitWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/Expression.h"
#include "MantidAPI/IMDIterator.h"

#include <boost/lexical_cast.hpp>
#include <boost/lambda/lambda.hpp>

using namespace boost::lambda;

template<class C, class F>
void for_all(C& c,F f)
{
  std::for_each(c.begin(),c.end(),f);
}

namespace Mantid{
  namespace MDDataObjects{
    using namespace Kernel;
    using namespace Geometry;
    using namespace API;


    // Register the workspace into the WorkspaceFactory
    DECLARE_WORKSPACE(MDFitWorkspace)


    // logger for MD workspaces  
    Kernel::Logger& MDFitWorkspace::g_log =Kernel::Logger::get("MDFitWorkspaces");

    /**
      * Creates the workspace
      * @param nDimensions The number of dimensions in the workspace.
      */
    MDFitWorkspace::MDFitWorkspace(unsigned int nDimensions, unsigned int nRecDims)
    {
      UNUSED_ARG(nRecDims);
      m_indexCalculator.reset(new MDWorkspaceIndexCalculator(nDimensions));
      m_dimensions.resize(nDimensions);
    }

    /**
      * Estimates the memory requirements for this workspace.
      * @return The memory footprint in bytes.
      */
    size_t MDFitWorkspace::getMemorySize(void) const
    {
      return m_cells.size()*sizeof(MDCell) + m_points.size()*(2*sizeof(void*)+sizeof(MDPoint));
    } 

    void  MDFitWorkspace::setInstrument(const Instrument_sptr& instr)
    {
      boost::shared_ptr<Instrument> tmp = boost::dynamic_pointer_cast<Instrument>(instr);
      if (tmp->isParametrized())
      {
        m_instrument = tmp->baseInstrument();
        m_parmap = tmp->getParameterMap();
      }
      else
      {
        m_instrument=tmp;
      }
    }


    uint64_t MDFitWorkspace::getNPoints() const
    {
      return uint64_t(m_points.size());
    }

    size_t MDFitWorkspace::getNumDims() const
    {
      return m_dimensions.size();
    }

    boost::shared_ptr<const Mantid::Geometry::IMDDimension> MDFitWorkspace::getXDimension() const
    { 
      return m_dimensions[0]; 
    }

    boost::shared_ptr< const Mantid::Geometry::IMDDimension> MDFitWorkspace::getYDimension() const
    { 
      if (m_dimensions.size() < 2)
      {
        throw std::runtime_error("MDFitWorkspace does not have the Y dimension");
      }
      return m_dimensions[1]; 
    }

    boost::shared_ptr<const Mantid::Geometry::IMDDimension> MDFitWorkspace::getZDimension() const
    { 
      if (m_dimensions.size() < 3)
      {
        throw std::runtime_error("MDFitWorkspace does not have the Z dimension");
      }
      return m_dimensions[2]; 
    }

    boost::shared_ptr<const Mantid::Geometry::IMDDimension> MDFitWorkspace::getTDimension() const
    { 
      if (m_dimensions.size() < 4)
      {
        throw std::runtime_error("MDFitWorkspace does not have the t dimension");
      }
      return m_dimensions[3]; 
    }

    boost::shared_ptr<const Mantid::Geometry::IMDDimension> MDFitWorkspace::getDimension(std::string id) const
    { 
      for(int i = 0; m_dimensions.size(); ++i)
      {
        boost::shared_ptr<IMDDimension> dim = m_dimensions[i];
        if (dim->getDimensionId() == id)
        {
          return dim;
        }
      }
      throw std::invalid_argument("MDFitWorkspace does not have dimension " + id); 
    }

    const Mantid::Geometry::SignalAggregate & MDFitWorkspace::getPoint(size_t index) const
    {
      if(index >= m_points.size())
      {
        throw std::range_error("Requested point is out of range.");
      }
      return *m_points[index];
    }

    const Mantid::Geometry::SignalAggregate& MDFitWorkspace::getCell(size_t dim1Increment) const
    {
      if (dim1Increment >= m_cells.size())
      {
        throw std::range_error("Cell index out of range");
      }
      return m_cells[dim1Increment];
    }

    /** 
      * Implementation of IMDDimension to work with MDFitWorkspace
      */
    class MDFitWorkspaceDimension: public IMDDimension
    {
    public:
      /**
        * Creates a dimension object.
        */
      MDFitWorkspaceDimension(const std::string& id,const std::string& name,const std::vector<double>& x):
          m_name(name),m_id(id)
      {
        if (x.size() < 2)
        {
          throw std::invalid_argument("A dimension must have at least one bin");
        }
        m_binBoundaries.assign(x.begin(),x.end());
      }
      virtual std::string getName() const {return m_name;}
      virtual std::string getUnits() const {return "None";}
      virtual std::string getDimensionId() const {return m_id;}
      virtual bool getIsIntegrated() const {return m_binBoundaries.size() == 2;}
      virtual double getMaximum() const {return m_binBoundaries.front();}
      virtual double getMinimum() const {return m_binBoundaries.back();}
      virtual size_t getNBins() const {return int(m_binBoundaries.size()) - 1;}
      virtual void setRange(size_t /*nBins*/, double /*min*/, double /*max*/){throw std::runtime_error("MDFitWorkspaceDimension::setRange(): Not implemented");}
      ///  Get coordinate for index; Good question: is it a bin boundary or the centre?
      virtual double getX(size_t ind)const
      {
        if (ind >= getNBins())
        {
          throw std::out_of_range("Dimension's x index is out of range");
        }
        return (m_binBoundaries[ind+1]+m_binBoundaries[ind])/2;
      }
      virtual std::string toXMLString() const {return "";}
    private:
      std::string m_name;
      std::string m_id;
      std::vector<double> m_binBoundaries;
    };

    class MDFitWorkspaceIterator: public IMDIterator
    {
    public:
      MDFitWorkspaceIterator(const MDWorkspaceIndexCalculator* indexCalculator,
        const std::vector<boost::shared_ptr<Mantid::Geometry::IMDDimension> >& dimensions):
      m_indexCalculator(indexCalculator),m_cur_pointer(),m_end_pointer(indexCalculator->getIndexUpperBounds())
      {
        m_index.resize(indexCalculator->getNDimensions());
        indexCalculator->calculateDimensionIndexes(m_cur_pointer,m_index);
        m_dimensions.assign(dimensions.begin(),dimensions.end());
      }
      /// Get the size of the data
      virtual size_t getDataSize()const
      {
        return m_indexCalculator->getIndexUpperBounds() + 1;
      }
      /// Get the i-th coordinate of the current cell
      virtual double getCoordinate(size_t i)const
      {
        return m_dimensions[i]->getX(m_index[i]);
      }
      /// Advance to the next cell. If the current cell is the last one in the workspace
      /// do nothing and return false.
      virtual bool next()
      {
        if (m_cur_pointer < m_end_pointer)
        {
          ++m_cur_pointer;
          m_indexCalculator->calculateDimensionIndexes(m_cur_pointer,m_index);
          return true;
        }
        return false;
      }
      ///< return the current data pointer (index)
      virtual size_t getPointer()const
      {
        return m_cur_pointer;
      }
    private:
      const MDWorkspaceIndexCalculator* m_indexCalculator;
      size_t m_cur_pointer;
      size_t m_end_pointer;
      std::vector<size_t> m_index;
      std::vector<boost::shared_ptr<Mantid::Geometry::IMDDimension> > m_dimensions;
    };

    /**
      * Creates a IMDDimension object for dimension idim of the workspace.
      * @param idim :: Index of the dimension.
      * @param paramString :: A string describing the dimension. Format:
      *    "id=<string>[,name=<string>],xmin=<double>,xmax=<double>,n=<int>, dx=<double>"
      * If name is not give name is the same as id. xmin is the lower bound of the first bin.
      * xmax is the upper bound of the last bin.
      * n is the number of bins, dx is the bin width. One of xmin, xmax, n, or dx should be omitted.
      * If all four are give dx is ignored.
      */
    void MDFitWorkspace::setDimension(size_t idim,const std::string& paramString)
    {
      if (idim >= getNumDims())
      {
        throw std::out_of_range("Dimension index is out of range");
      }
      Expression expr;
      expr.parse(paramString); // throws if fails
      if (expr.size() < 2 || expr.name() != ",")
      {
        throw std::invalid_argument("Dimension parameter string must be a comma separated list of key=value pairs");
      }
      double xmin = DBL_MAX;
      double xmax = DBL_MAX;
      double dx = DBL_MAX;
      int n = -1;
      std::string id,name;
      std::vector<double> x;
      // parse the input expression
      for(size_t i = 0; i < expr.size(); ++i)
      {
        const Expression& key_value = expr[i];
        std::string key = key_value[0].name();
        std::string value = key_value[1].str();
        if (key == "id")
        {
          id = value;
        }
        else if (key == "name")
        {
          name = value;
        }
        else if (key == "xmin")
        {
          xmin = boost::lexical_cast<double>(value);
        }
        else if (key == "xmax")
        {
          xmax = boost::lexical_cast<double>(value);
        }
        else if (key == "dx")
        {
          dx = boost::lexical_cast<double>(value);
        }
        else if (key == "n")
        {
          n = boost::lexical_cast<int>(value);
        }
      }

      if (xmin == DBL_MAX) // xmin undefined
      {
        if (xmax == DBL_MAX || dx == DBL_MAX || n < 1)
        {
          throw std::invalid_argument("Not enough input to create a dimension");
        }
        xmin = xmax - n*dx;
      }
      else if (xmax == DBL_MAX) // xmax undefined
      {
        if (xmin == DBL_MAX || dx == DBL_MAX || n < 1)
        {
          throw std::invalid_argument("Not enough input to create a dimension");
        }
        xmax = xmin + n*dx;
      }
      else if (dx == DBL_MAX) // dx undefined
      {
        if (xmax == DBL_MAX || xmin == DBL_MAX || n < 1)
        {
          throw std::invalid_argument("Not enough input to create a dimension");
        }
        dx = (xmax - xmin)/n;
      }
      else if (n < 1) // n undefined
      {
        if (xmax == DBL_MAX || xmin == DBL_MAX || dx == DBL_MAX)
        {
          throw std::invalid_argument("Not enough input to create a dimension");
        }
        n = int((xmax - xmin)/dx);
        if (n < 1) n = 1;
        dx = (xmax - xmin)/n; // adjust dx
      }
      else // all params are defined
      {
        dx = (xmax - xmin)/n; // ignore input dx
      }

      x.resize(n+1);
      { int i = 0; std::for_each(x.begin(),x.end(), _1 = xmin + (var(i)++)*dx); }
      if (name.empty())
      {
        name = id;
      }
      MDFitWorkspaceDimension* dim = new MDFitWorkspaceDimension(id,name,x);
      m_dimensions[idim].reset(dim);
	  // idim can not be larger then 2^32, so cast to unsigned int
      m_indexCalculator->setDimensionSize((unsigned int)idim,n);
      if (m_indexCalculator->isValid())
      {
        m_cells.resize(m_indexCalculator->getIndexUpperBounds()+1);
      }
    }

    /// Creates a new iterator pointing to the first cell in the workspace
    IMDIterator* MDFitWorkspace::createIterator() const
    {
      return new MDFitWorkspaceIterator(m_indexCalculator.get(),m_dimensions);
    }

    void MDFitWorkspace::setCell(size_t index,const std::vector<boost::shared_ptr<Mantid::Geometry::MDPoint> >& points)
    {
      if (index >= m_cells.size())
      {
        std::out_of_range("Cell index is out of range");
      }
      MDCell& cell = m_cells[index];
      cell = MDCell(points,std::vector<Coordinate>());
    }


  /*
  Get non-collapsed dimensions
  @return vector of collapsed dimensions in the workspace geometry.
  */
  Mantid::Geometry::VecIMDDimension_const_sptr MDFitWorkspace::getNonIntegratedDimensions() const
  {
    using namespace Mantid::Geometry;
    VecIMDDimension_const_sptr vecCollapsedDimensions;
    VecIMDDimension_sptr::const_iterator it = this->m_dimensions.begin();
    for(; it != this->m_dimensions.end(); ++it)
    {
      IMDDimension_sptr current = (*it);
      if(!current->getIsIntegrated())
      {
        vecCollapsedDimensions.push_back(current);
      }
    }
    return vecCollapsedDimensions;
  }


  } // namespace
}
//*********************************************************************************************************************************************************************************
namespace Mantid
{
  namespace Kernel
  {
    template<> DLLExport
      Mantid::MDDataObjects::MDFitWorkspace_sptr IPropertyManager::getValue<Mantid::MDDataObjects::MDFitWorkspace_sptr>(const std::string &name) const
    {
      PropertyWithValue<Mantid::MDDataObjects::MDFitWorkspace_sptr>* prop =
        dynamic_cast<PropertyWithValue<Mantid::MDDataObjects::MDFitWorkspace_sptr>*>(getPointerToProperty(name));
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
      Mantid::MDDataObjects::MDFitWorkspace_const_sptr IPropertyManager::getValue<Mantid::MDDataObjects::MDFitWorkspace_const_sptr>(const std::string &name) const
    {
      PropertyWithValue<Mantid::MDDataObjects::MDFitWorkspace_const_sptr>* prop =
        dynamic_cast<PropertyWithValue<Mantid::MDDataObjects::MDFitWorkspace_const_sptr>*>(getPointerToProperty(name));
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
} // name


