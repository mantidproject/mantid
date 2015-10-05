#ifndef TEXTFILEIO_H_
#define TEXTFILEIO_H_

#include <QString>
#include <QStringList>

/**
 * Defines a static class for simple text file
 * I/O.
 */
class TextFileIO
{
public:
  /// Construct the object with a list of file filters
  TextFileIO(QStringList fileFilters = QStringList());

  /// Save to a file
  bool save(const QString & txt, const QString & filename) const;

private:
  /// Open a save dialog
  QString askWhereToSave() const;

  const QStringList m_filters;
};


#endif /* TEXTFILEIO_H_ */
