// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/Hint.h"
namespace MantidQt {
namespace MantidWidgets {

Hint::Hint(std::string word, std::string description)
    : m_word(std::move(word)), m_description(std::move(description)) {}

std::string const &Hint::word() const { return m_word; }

std::string const &Hint::description() const { return m_description; }

bool operator==(Hint const &lhs, Hint const &rhs) {
  return lhs.word() == rhs.word() && lhs.description() == rhs.description();
}

bool operator!=(Hint const &lhs, Hint const &rhs) { return !(lhs == rhs); }
} // namespace MantidWidgets
} // namespace MantidQt
