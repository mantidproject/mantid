#include "MantidAPI/NotebookWriter.h"
#include <fstream>


Json::Value NotebookWriter::codeCell(std::string string_code) {

  Json::Value cell_data;

  cell_data["cell_type"] = "code";
  cell_data["collapsed"] = false;
  cell_data["input"] = string_code;
  cell_data["language"] = "python";
  cell_data["metadata"] = {};

  return cell_data;
}

Json::Value NotebookWriter::markdownCell(std::string string_text) {

  Json::Value cell_data;

  cell_data["cell_type"] = "markdown";
  cell_data["metadata"] = {};
  cell_data["source"] = string_text;

  return cell_data;
}

Json::Value NotebookWriter::headerCell() {

  return markdownCell("Mantid Version: [VERSION NUMBER]");
}

Json::Value NotebookWriter::buildNotebook() {

  Json::Value output;

  Json::Value cells(Json::arrayValue);
  cells.append(headerCell());
  cells.append(codeCell("print \"Hello, IPython\""));

  Json::Value meta_name;
  meta_name["name"] = "example";
  output["metadata"] = meta_name;
  output["nbformat"] = 3;
  output["nbformat_minor"] = 0;
  output["worksheets"] = cells;

  return output;
}

void NotebookWriter::writeNotebook() {

  std::string filename = "test_notebook.json";
  std::ofstream out_stream;
  out_stream.open(filename);
  out_stream << buildNotebook();
  out_stream.close();

}
