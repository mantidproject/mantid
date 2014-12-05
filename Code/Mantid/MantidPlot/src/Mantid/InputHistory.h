#ifndef INPUTHISTORY_H
#define INPUTHISTORY_H

#include <MantidKernel/SingletonHolder.h>
#include <boost/shared_ptr.hpp>

#include <QString>
#include <QMap>
#include <QList>

namespace Mantid
{
    namespace API
    {
        class IAlgorithm;
        typedef boost::shared_ptr<IAlgorithm> IAlgorithm_sptr;
    }
}

struct PropertyData
{
    PropertyData(const QString& nm,const QString& vl):name(nm),value(vl){}
    QString name;
    QString value;
};

/** @class InputHistory

 Keeps history of Mantid-related user input, such as algorithm parameters, etc.

 @author Roman Tolchenov, Tessella Support Services plc
 @date 15/10/2008

 Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

class InputHistoryImpl
{
public:
    void updateAlgorithm(Mantid::API::IAlgorithm_sptr alg);
    /// The name:value map of non-default properties with which algorithm algName was called last time.
    QMap< QString, QString > algorithmProperties(const QString& algName);
    /// Returns the value of property propNameif it has been recorded for algorithm algName.
    QString algorithmProperty(const QString& algName,const QString& propName);
    /// Replaces the value of a recorded property.
    void updateAlgorithmProperty(const QString& algName,const QString& propName, const QString& propValue);
    /// Saves the properties.
    void save();

    /// Returns the directory name from a full file path.
    static QString getDirectoryFromFilePath(const QString& filePath);
    /// Returns the short file name (without extension) from a full file path.
    static QString getNameOnlyFromFilePath(const QString& filePath);

private:
	friend struct Mantid::Kernel::CreateUsingNew<InputHistoryImpl>;

	///Private Constructor
	InputHistoryImpl();
	/// Private copy constructor - NO COPY ALLOWED
	InputHistoryImpl(const InputHistoryImpl&);
	/// Private assignment operator - NO ASSIGNMENT ALLOWED
	InputHistoryImpl& operator = (const InputHistoryImpl&);
	///Private Destructor
	virtual ~InputHistoryImpl();

  /// For debugging
  // cppcheck-suppress unusedPrivateFunction
  void printAll();

  /// Keeps algorithm parameters.
  QMap< QString,QList< PropertyData > > m_history;

};

typedef Mantid::Kernel::SingletonHolder<InputHistoryImpl> InputHistory;


#endif /* INPUTHISTORY_H */
