// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/Spectra.h"
#include "MantidKernel/Strings.h"

#include <numeric>

namespace {

std::vector<MantidQt::MantidWidgets::WorkspaceIndex>
workspaceIndexVectorFromString(const std::string &listString) {
  auto const intVec =
      MantidQt::MantidWidgets::vectorFromString<std::size_t>(listString);
  std::vector<MantidQt::MantidWidgets::WorkspaceIndex> output;
  for (auto const i : intVec) {
    output.emplace_back(MantidQt::MantidWidgets::WorkspaceIndex{i});
  }
  return output;
}

} // namespace

namespace MantidQt {
namespace MantidWidgets {

Spectra::Spectra(const std::string &str)
    : m_vec(workspaceIndexVectorFromString(str)), m_isContinuous(true) {
  checkContinuous();
}

Spectra::Spectra(WorkspaceIndex minimum, WorkspaceIndex maximum) {
  if (maximum < minimum) {
    std::swap(minimum, maximum);
  }
  m_vec.resize(maximum.value - minimum.value + 1);
  std::iota(m_vec.begin(), m_vec.end(), minimum);
  m_isContinuous = true;
}

Spectra::Spectra(const Spectra &vec)
    : m_vec(vec.m_vec), m_isContinuous(vec.m_isContinuous) {}

Spectra::Spectra(Spectra &&vec)
    : m_vec(std::move(vec.m_vec)),
      m_isContinuous(std::move(vec.m_isContinuous)) {}

Spectra &Spectra::operator=(const Spectra &vec) {
  m_vec = vec.m_vec;
  m_isContinuous = vec.m_isContinuous;
  return *this;
}

Spectra &Spectra::operator=(Spectra &&vec) {
  m_vec = std::move(vec.m_vec);
  m_isContinuous = std::move(vec.m_isContinuous);
  return *this;
}

[[nodiscard]] bool Spectra::empty() const { return m_vec.empty(); }

FitDomainIndex Spectra::size() const {
  return FitDomainIndex{m_vec.size()};
}

std::string Spectra::getString() const {
  if (empty())
    return "";
  if (m_isContinuous)
    return m_vec.size() > 1 ? std::to_string(m_vec.front().value) + "-" +
                                  std::to_string(m_vec.back().value)
                            : std::to_string(m_vec.front().value);
  std::vector<size_t> out(m_vec.size());
  std::transform(m_vec.begin(), m_vec.end(), out.begin(),
                 [](WorkspaceIndex i) { return i.value; });
  return Mantid::Kernel::Strings::toString(out);
}

std::pair<WorkspaceIndex, WorkspaceIndex> Spectra::getMinMax() const {
  if (empty())
    return std::make_pair(WorkspaceIndex{0}, WorkspaceIndex{0});
  return std::make_pair(m_vec.front(), m_vec.back());
}

bool Spectra::operator==(Spectra const &spec) const {
  return this->getString() == spec.getString();
}

bool Spectra::isContinuous() const { return m_isContinuous; }

FitDomainIndex Spectra::indexOf(WorkspaceIndex i) const {
  auto const it = std::find(begin(), end(), i);
  if (it == end()) {
    throw std::runtime_error("Spectrum index " + std::to_string(i.value) +
                             " not found.");
  }
  return FitDomainIndex{static_cast<size_t>(std::distance(begin(), it))};
}

Spectra Spectra::combine(const Spectra &other) const {
  std::set<WorkspaceIndex> indices(begin(), end());
  indices.insert(other.begin(), other.end());
  return Spectra(indices);
}

Spectra::Spectra(const std::set<WorkspaceIndex> &indices)
    : m_vec(indices.begin(), indices.end()) {
  checkContinuous();
}

void Spectra::checkContinuous() {
  m_isContinuous = true;
  if (m_vec.size() > 1) {
    for (size_t i = 1; i < m_vec.size(); ++i) {
      if (m_vec[i].value - m_vec[i - 1].value != 1) {
        m_isContinuous = false;
        break;
      }
    }
  }
}

void Spectra::erase(WorkspaceIndex workspaceIndex) {
  auto iteratorToErase = std::find(m_vec.begin(), m_vec.end(), workspaceIndex);
  if (iteratorToErase != m_vec.end()) {
    m_vec.erase(iteratorToErase);
    checkContinuous();
  }
}

} // namespace MantidWidgets
} // namespace MantidQt
