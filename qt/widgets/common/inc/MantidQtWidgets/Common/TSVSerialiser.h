// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_TSVSERIALISER_H_
#define MANTID_TSVSERIALISER_H_

#include "DllOption.h"
#include "MantidKernel/CaseInsensitiveMap.h"

#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <QColor>
#include <QPoint>
#include <QRect>
#include <QString>

/** Parses the formatting used in MantidPlot project files

  @author Harry Jeffery, ISIS, RAL
  @date 23/07/2014
*/

namespace MantidQt {
namespace API {

class EXPORT_OPT_MANTIDQT_COMMON TSVSerialiser {
public:
  TSVSerialiser();

  explicit TSVSerialiser(const std::string &lines);

  void parseLines(const std::string &lines);
  std::string outputLines() const;
  void clear();

  bool hasLine(const std::string &name) const;
  bool hasSection(const std::string &name) const;

  std::vector<std::string> values(const std::string &name,
                                  const size_t i = 0) const;

  template <typename T> TSVSerialiser &operator>>(std::vector<T> &val) {
    val.reserve(m_curValues.size() - m_curIndex);

    for (size_t i = m_curIndex; i < m_curValues.size(); ++i) {
      auto valStr = m_curValues.at(i);
      std::stringstream valSS(valStr);
      T ret;
      valSS >> ret;
      val.push_back(ret);
    }
    return *this;
  }

  /**
   * Parse all lines matching a name and extract the values to a vector
   *
   * This is an overloaded version of the function below that uses a default
   * extractor function. This expects that the type of the container matches
   * one of the parsable types implemented in this class.
   *
   * @param name :: the name of the line to match with
   * @param container :: the output vector to store values in
   */
  template <typename T>
  void parseLines(const std::string &name, std::vector<T> &container) {

    auto extractor = [](TSVSerialiser &tsv) {
      T value;
      tsv >> value;
      return value;
    };

    parseLines(name, container, extractor);
  }

  /**
   * Parse all lines matching a name and extract the values to a vector
   *
   * The third argument should be a function that accepts a TSVSerialiser
   * instance and returns the parsed value matching the type of the container.
   *
   * @param name :: the name of the line to match with
   * @param container :: the output vector to store values in
   * @param extractor :: function to use to extract values from each line
   */
  template <typename T, typename Extractor>
  void parseLines(const std::string &name, std::vector<T> &container,
                  Extractor &&extractor) {
    size_t index = 0;
    while (selectLine(name, index)) {
      auto value = std::forward<Extractor>(extractor)(*this);
      container.push_back(value);
      ++index;
    }
  }

  std::vector<std::string> sections(const std::string &name) const;

  std::string lineAsString(const std::string &name, const size_t i = 0) const;
  QString lineAsQString(const std::string &name, const size_t i = 0) const;

  bool selectLine(const std::string &name, const size_t i = 0);
  bool selectSection(const std::string &name, const size_t i = 0);

  void storeDouble(const double val);
  void storeInt(const int val);
  void storeString(const std::string val);
  void storeBool(const bool val);

  double readDouble();
  int readInt();
  std::string readString();
  bool readBool();

  int asInt(const size_t i) const;
  size_t asSize_t(const size_t i) const;
  double asDouble(const size_t i) const;
  float asFloat(const size_t i) const;
  std::string asString(const size_t i) const;
  QString asQString(const size_t i) const;
  bool asBool(const size_t i) const;
  QRect asQRect(const size_t i);
  QColor asQColor(const size_t i);
  QPoint asQPoint(const size_t i);
  QPointF asQPointF(const size_t i);

  TSVSerialiser &operator>>(int &val);
  TSVSerialiser &operator>>(size_t &val);
  TSVSerialiser &operator>>(double &val);
  TSVSerialiser &operator>>(float &val);
  TSVSerialiser &operator>>(std::string &val);
  TSVSerialiser &operator>>(QString &val);
  TSVSerialiser &operator>>(bool &val);
  TSVSerialiser &operator>>(QRect &val);
  TSVSerialiser &operator>>(QColor &val);
  TSVSerialiser &operator>>(QPoint &val);
  TSVSerialiser &operator>>(QPointF &val);

  TSVSerialiser &writeLine(const std::string &name);

  TSVSerialiser &operator<<(const std::string &val);
  TSVSerialiser &operator<<(const char *val);
  TSVSerialiser &operator<<(const QString &val);
  TSVSerialiser &operator<<(const double &val);
  TSVSerialiser &operator<<(const int &val);
  TSVSerialiser &operator<<(const size_t &val);
  TSVSerialiser &operator<<(const bool &val);
  TSVSerialiser &operator<<(const QRect &val);
  TSVSerialiser &operator<<(const QColor &val);
  TSVSerialiser &operator<<(const QPoint &val);
  TSVSerialiser &operator<<(const QPointF &val);

  void writeRaw(const std::string &raw);
  void writeSection(const std::string &name, const std::string &body);
  void writeInlineSection(const std::string &name, const std::string &body);

private:
  Mantid::Kernel::CaseInsensitiveMap<std::vector<std::string>> m_sections;
  Mantid::Kernel::CaseInsensitiveMap<std::vector<std::string>> m_lines;

  std::vector<std::string> m_curValues;
  int m_curIndex;

  std::stringstream m_output;
  bool m_midLine;
};
} // namespace API
} // namespace MantidQt

#endif
