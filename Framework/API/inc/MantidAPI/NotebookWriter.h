// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

/** @class NotebookWriter

    This class assists with writing ipython notebooks.
    */

#include "MantidAPI/DllConfig.h"

#include <json/writer.h>
#include <string>

namespace Mantid {
namespace API {

class MANTID_API_DLL NotebookWriter {

public:
  NotebookWriter();
  virtual ~NotebookWriter() = default;
  std::string markdownCell(const std::string &string_text);
  std::string codeCell(const std::string &string_code);
  std::string writeNotebook();

private:
  void headerComment();
  void headerCode();

  Json::Value buildNotebook();

  void markdownCell(Json::Value string_array);
  void codeCell(Json::Value array_code);

  Json::Value m_cell_buffer;
};
} // namespace API
} // namespace Mantid
