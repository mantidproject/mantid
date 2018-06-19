#ifndef MANTID_MANTIDWIDGETS_HINT_H_
#define MANTID_MANTIDWIDGETS_HINT_H_
#include <string>
#include "DllOption.h"

namespace MantidQt {
namespace MantidWidgets {

class EXPORT_OPT_MANTIDQT_COMMON Hint {
public:
  Hint(std::string word, std::string description);
  std::string const &word() const;
  std::string const &description() const;

private:
  std::string m_word;
  std::string m_description;
};
}
}
#endif // MANTID_MANTIDWIDGETS_HINT_H_
