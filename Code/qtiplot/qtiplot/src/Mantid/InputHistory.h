#ifndef INPUTHISTORY_H
#define INPUTHISTORY_H

#include <MantidKernel/SingletonHolder.h>

#include <QString>
#include <QMap>
#include <QList>

using namespace Mantid::Kernel;

namespace Mantid
{
    namespace API
    {
        class Algorithm;
    }
}

struct PropertyData
{
    PropertyData(const QString& nm,const QString& vl):name(nm),value(vl){}
    QString name;
    QString value;
};

/*  

*/
class InputHistoryImpl
{
public:
    void updateAlgorithm(Mantid::API::Algorithm *alg);
    /// The name:value map of non-default properties with which algorithm algName was called last time.
    QMap< QString, QString > algorithmProperties(const QString& algName);
    /// Returns the value of property propNameif it has been recorded for algorithm algName.
    QString algorithmProperty(const QString& algName,const QString& propName);
    /// Replaces the value of a recorded property.
    void updateAlgorithmProperty(const QString& algName,const QString& propName, const QString& propValue);

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
    void printAll();

    /// Keeps algorithm parameters.
    QMap< QString,QList< PropertyData > > m_history;

};

typedef Mantid::Kernel::SingletonHolder<InputHistoryImpl> InputHistory;


#endif /* INPUTHISTORY_H */
