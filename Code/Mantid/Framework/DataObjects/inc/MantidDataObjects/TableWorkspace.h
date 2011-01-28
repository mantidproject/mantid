#ifndef MANTID_DATAOBJECTS_TABLEWORKSPACE_H_
#define MANTID_DATAOBJECTS_TABLEWORKSPACE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

#include "MantidKernel/System.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidDataObjects/TableColumn.h"
#include "MantidDataObjects/TablePointerColumn.h"
#include <boost/shared_ptr.hpp>
#include <boost/tuple/tuple.hpp>

namespace Mantid
{

//----------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------
namespace Kernel
{
  class Logger;
}

namespace DataObjects
{
/** \class TableWorkspace

     TableWorkspace is an implementation of Workspace in which the data are organised in columns of same size.
     Elements of a column have the same data type. Columns can be added to the TableWorkspace with
     ctreateColumn(type,name). name is a name given to the column. type is a symbolic name for the data type of the column. Predefined types are:
     - "int"    for int
     - "float"  for float
     - "double" for double
     - "bool"   for bool
     - "str"    for std::string
     - "V3D"    for Mantid::Geometry::V3D

     User defined types can be used after declaring them with DECLARE_TABLECOLUMN macro: 
     DECLARE_TABLECOLUMN(typeName, UserDefinedType)

     Ways to access the data:
       - Using templated cell method. E.g. SomeType var = table.cell<SomeType>(i,j); where j is the column number, i is 
         the position of the element in the column or the row number. The type of var must match the column's type,
         otherwise a runtime_error exception will be thrown. The columns are kept in the order of their creation.
       - Using specialized access methods for the predefined types. E.g. int var = table.Int(i,j);. If j-th column is
         of the wrong type a runtime_error exception will be thrown. 
       - Getting the pointer to a column and working with it.
       - Creating a TableRow object and working with it.


    \author Roman Tolchenov
    \date 31/10/2008

    Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
#ifdef _WIN32
#ifdef IN_MANTID_DATA_OBJECTS
  #define TableWorkspace_DllExport __declspec( dllexport )
#else
  #define TableWorkspace_DllExport __declspec( dllimport )
#endif
#else
  #define TableWorkspace_DllExport
  #define TableWorkspace_DllImport
#endif

  class TableWorkspace_DllExport TableWorkspace: public API::ITableWorkspace
  {
  public:
    /// Constructor.
    TableWorkspace(int nrows=0);
    /// Virtual destructor.
    virtual ~TableWorkspace();
    /// Return the workspace typeID
    virtual const std::string id() const{return "TableWorkspace";}
    /// Get the footprint in memory in KB.
    virtual size_t getMemorySize() const;

    /// Creates a new column.
    bool addColumn(const std::string& type, const std::string& name);
    /// Removes a column.
    void removeColumn( const std::string& name);
    /// Number of columns in the workspace.
    int columnCount() const {return static_cast<int>(m_columns.size());}
    /// Gets the shared pointer to a column.
    API::Column_sptr getColumn(const std::string& name);
    /// Gets the shared pointer to a column.
    API::Column_sptr getColumn(int index);
    /// Returns a vector of all column names.
    std::vector<std::string> getColumnNames();
    /// Number of rows in the workspace.
    int rowCount() const {return m_rowCount;}
    /// Resizes the workspace.
    void setRowCount(int count);
    /// Inserts a row before row pointed to by index and fills it with default vales.
    int insertRow(int index);
    /// Delets a row if it exists.
    void removeRow(int index);
     
	 /** This method finds the row and column index of an integer cell value in a table workspace
	 * @param value :: -value to search
	 * @param  row  row number of the value  searched
	 * @param  col  column number of the value searched
	*/
	virtual void find(int value,int& row,const int & col)
	{		
		findValue(value,row,col);
	}
	/** This method finds the row and column index of an string cell value in a table workspace
	* @param value :: -value to search
	* @param  row  row number of the value  searched
	* @param  col  column number of the value searched
	*/
	virtual void find(std::string value,int& row,const int & col)
	{
		findValue(value,row,col);
	}
	/** This method finds the row and column index of an float value in a table workspace
	* @param value :: -value to search
	* @param  row  row number of the value  searched
	* @param  col  column number of the value searched
	*/
	virtual void find(float value,int& row,const int & col)
	{
		findValue(value,row,col);
	}
	/** This method finds the row and column index of an API::Bollean value in a table workspace
	* @param value :: -value to search
	* @param  row  row number of the value  searched
	* @param  col  column number of the value searched
	*/
	virtual void find(API::Boolean value,int& row,const int & col)
	{
		findValue(value,row,col);
	}
	/** This method finds the row and column index of an double cell value in a table workspace
	* @param value :: -value to search
	* @param  row  row number of the value  searched
	* @param  col  column number of the value searched
	*/
	virtual void find(double value,int& row,const int & col)
	{
		findValue(value,row,col);
	}
	/** This method finds the row and column index of an Mantid::Geometry::V3D cell value in a table workspace
	* @param value :: -value to search
	* @param  row  row number of the value  searched
	* @param  col  column number of the value searched
	*/
	void find(Mantid::Geometry::V3D value,int& row,const int & col)
	{
		findValue(value,row,col);
	}
			
private:
	
	/// template method to find a given value in a table.
	template<typename  Type>
	void findValue(const Type value,int& row,const int & colIndex)
	{
		
		try
		{
			TableColumn_ptr<Type> tc_sptr= getColumn(colIndex);
			std::vector<Type> dataVec=tc_sptr->data();
			typename std::vector<Type>::iterator itr;
			itr=std::find(dataVec.begin(),dataVec.end(),value);
			if(itr!=dataVec.end())
			{
				std::vector<int>::difference_type pos;
				pos=std::distance(dataVec.begin(),itr);
				//int pos=static_cast<int>itr-dataVec.begin();
				row=static_cast<int>(pos);
				
			}
			else
			{
				throw std::out_of_range("Search object not found in table workspace");
			 }
		}
		catch(std::range_error&)
		{
			throw;
		}
		catch(std::runtime_error&)
		{
			throw;
		}
	}
	
    /** This method finds the row and column index of an integer cell value in a table workspace
    * @param value :: -value to search
    * @param  row  row number of the value  searched
    * @param  col  column number of the value searched
    */
    virtual void find(int value,int& row,int & col)
    {		
      findValue(value,row,col);
    }
    /** This method finds the row and column index of an string cell value in a table workspace
    * @param value :: -value to search
    * @param  row  row number of the value  searched
    * @param  col  column number of the value searched
    */
    virtual void find(std::string value,int& row,int & col)
    {
      findValue(value,row,col);
    }
    /** This method finds the row and column index of an float value in a table workspace
    * @param value :: -value to search
    * @param  row  row number of the value  searched
    * @param  col  column number of the value searched
    */
    virtual void find(float value,int& row,int & col)
    {
      findValue(value,row,col);
    }
    /** This method finds the row and column index of an API::Bollean value in a table workspace
    * @param value :: -value to search
    * @param  row  row number of the value  searched
    * @param  col  column number of the value searched
    */
    virtual void find(API::Boolean value,int& row,int & col)
    {
      findValue(value,row,col);
    }
    /** This method finds the row and column index of an double cell value in a table workspace
    * @param value :: -value to search
    * @param  row  row number of the value  searched
    * @param  col  column number of the value searched
    */
    virtual void find(double value,int& row,int & col)
    {
      findValue(value,row,col);
    }
    /** This method finds the row and column index of an Mantid::Geometry::V3D cell value in a table workspace
    * @param value :: -value to search
    * @param  row  row number of the value  searched
    * @param  col  column number of the value searched
    */
    void find(Mantid::Geometry::V3D value,int& row,int & col)
    {
      findValue(value,row,col);
    }


private:
    /// Used in std::find_if algorithm to find a Column with name \a name.
    class FindName
    {
        std::string m_name;///< Name to find
    public:
        /// Constructor
        FindName(const std::string& name):m_name(name){}
        /// Comparison operator
        bool operator()(boost::shared_ptr<API::Column>& cp)
        {
            return cp->name() == m_name;
        }
    };

    typedef std::vector< boost::shared_ptr<API::Column> >::iterator column_it;///< Column iterator
    typedef std::vector< boost::shared_ptr<API::Column> >::const_iterator column_const_it;///< Column const iterator
    /// Shared pointers to the columns.
    std::vector< boost::shared_ptr<API::Column> > m_columns;
    /// row count
    int m_rowCount;
    /// Logger
    static Kernel::Logger& g_log;

};

/// Typedef for a shared pointer to \c TableWorkspace
typedef boost::shared_ptr<TableWorkspace> TableWorkspace_sptr;
/// Typedef for a shared pointer to \c const \c TableWorkspace
typedef boost::shared_ptr<const TableWorkspace> TableWorkspace_const_sptr;


} // namespace DataObjects
} // Namespace Mantid
#endif /*MANTID_DATAOBJECTS_TABLEWORKSPACE_H_*/
