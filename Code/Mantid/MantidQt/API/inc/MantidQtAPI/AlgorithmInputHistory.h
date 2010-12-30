#ifndef MANTIDQT_API_ALGORITHMINPUTHISTORY_H_
#define MANTIDQT_API_ALGORITHMINPUTHISTORY_H_

//----------------------------------
// Includes
//----------------------------------
#include "DllOption.h"
#include "MantidKernel/SingletonHolder.h"

#include <QVector>
#include <QHash>
#include <QString>

//----------------------------------
// Mantid forward declarations
//----------------------------------
namespace Mantid
{
  namespace Kernel
  {
    class Property;
  }
}


// Top level namespace for this library
namespace MantidQt
{

namespace API
{

//----------------------------------
// Forward declarations
//----------------------------------

/** 
    This class is responsible for creating the correct dialog for an algorithm. If 
    no specialized version is registered for that algorithm then the default is created
    
    @author Martyn Gigg, Tessella Support Services plc
    @date 27/02/2009

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>    
*/
class EXPORT_OPT_MANTIDQT_API AlgorithmInputHistoryImpl
{
  
public:

  /// Update the old values that are stored here. Only valid
  /// values are stored here
  void storeNewValue(const QString & algName, const QPair<QString, QString> & property);
  
  /// Clear values for a particular algorithm
  void clearAlgorithmInput(const QString & algName);

  /// Retrieve an old parameter value 
  QString previousInput(const QString & algName, const QString & propName) const;
  
  /// Set the directory that was accessed when the previous open file dialog was used
  void setPreviousDirectory(const QString & lastdir);
  
  /// Get the directory that was accessed when the previous open file dialog was used
  const QString & getPreviousDirectory() const;

  /// Save the values stored here to persistent storage
  void save() const;
  
private:
  friend struct Mantid::Kernel::CreateUsingNew<AlgorithmInputHistoryImpl>;
  
  ///Private Constructor
  AlgorithmInputHistoryImpl();
  /// Private copy constructor - NO COPY ALLOWED
  AlgorithmInputHistoryImpl(const AlgorithmInputHistoryImpl&);
  /// Private assignment operator - NO ASSIGNMENT ALLOWED
  AlgorithmInputHistoryImpl& operator = (const AlgorithmInputHistoryImpl&);
  ///Private Destructor
  virtual ~AlgorithmInputHistoryImpl();
  
  /// Load any values that are available from persistent storage
  void load();
  
  /// A map indexing the algorithm name and a list of property name:value pairs
  QHash<QString, QHash<QString, QString> > m_lastInput;
  
  /// The directory that last used by an open file dialog
  QString m_previousDirectory;
  
  /// The string denoting the group where the algorithm properties are stored
  QString m_algorithmsGroup;
  
  /// The string denoting the key for the previous dir storage
  QString m_dirKey;
};

#ifdef _WIN32
// this breaks new namespace declaraion rules; need to find a better fix
	template class EXPORT_OPT_MANTIDQT_API Mantid::Kernel::SingletonHolder<AlgorithmInputHistoryImpl>;
#endif /* _WIN32 */
  /// The specific instantiation of the templated type
  typedef EXPORT_OPT_MANTIDQT_API Mantid::Kernel::SingletonHolder<AlgorithmInputHistoryImpl> AlgorithmInputHistory;

}
}

#endif //ALGORITHMINPUTHISTORY_H_
