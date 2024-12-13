#pragma once
#include <string>

class PythonHelpBridge {
public:
  PythonHelpBridge();
  void showHelpPage(const std::string &relative_url);
};
