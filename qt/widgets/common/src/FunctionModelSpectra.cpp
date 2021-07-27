// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/FunctionModelSpectra.h"
#include "MantidKernel/Strings.h"

#include <numeric>

namespace {
using MantidQt::MantidWidgets::WorkspaceIndex;

std::vector<WorkspaceIndex> workspaceIndexVectorFromString(const std::string &listString) {
  auto const intVec = MantidQt::MantidWidgets::vectorFromString<std::size_t>(listString);
  std::vector<WorkspaceIndex> output;
  for (auto const i : intVec) {
    output.emplace_back(WorkspaceIndex{i});
  }
  return output;
}

} // namespace

namespace MantidQt {
namespace MantidWidgets {

FunctionModelSpectra::FunctionModelSpectra(const std::string &str)
    : m_vec(workspaceIndexVectorFromString(str)), m_isContinuous(true) {
  checkContinuous();
}

FunctionModelSpectra::FunctionModelSpectra(WorkspaceIndex minimum, WorkspaceIndex maximum) {
  if (maximum < minimum) {
    std::swap(minimum, maximum);
  }
  m_vec.resize(maximum.value - minimum.value + 1);
  std::iota(m_vec.begin(), m_vec.end(), minimum);
  m_isContinuous = true;
}

FunctionModelSpectra::FunctionModelSpectra(const FunctionModelSpectra &vec)
    : m_vec(vec.m_vec), m_isContinuous(vec.m_isContinuous) {}

FunctionModelSpectra::FunctionModelSpectra(FunctionModelSpectra &&vec)
    : m_vec(std::move(vec.m_vec)), m_isContinuous(std::move(vec.m_isContinuous)) {}

FunctionModelSpectra &FunctionModelSpectra::operator=(const FunctionModelSpectra &vec) {
  m_vec = vec.m_vec;
  m_isContinuous = vec.m_isContinuous;
  return *this;
}

FunctionModelSpectra &FunctionModelSpectra::operator=(FunctionModelSpectra &&vec) {
  m_vec = std::move(vec.m_vec);
  m_isContinuous = std::move(vec.m_isContinuous);
  return *this;
}

[[nodiscard]] bool FunctionModelSpectra::empty() const { return m_vec.empty(); }

FitDomainIndex FunctionModelSpectra::size() const { return FitDomainIndex{m_vec.size()}; }

std::string FunctionModelSpectra::getString() const {
  if (empty())
    return "";
  if (m_isContinuous)
    return m_vec.size() > 1 ? std::to_string(m_vec.front().value) + "-" + std::to_string(m_vec.back().value)
                            : std::to_string(m_vec.front().value);
  std::vector<size_t> out(m_vec.size());
  std::transform(m_vec.begin(), m_vec.end(), out.begin(), [](WorkspaceIndex i) { return i.value; });
  return Mantid::Kernel::Strings::toString(out);
}

std::pair<WorkspaceIndex, WorkspaceIndex> FunctionModelSpectra::getMinMax() const {
  if (empty())
    return std::make_pair(WorkspaceIndex{0}, WorkspaceIndex{0});
  return std::make_pair(m_vec.front(), m_vec.back());
}

bool FunctionModelSpectra::operator==(FunctionModelSpectra const &spec) const {
  return this->getString() == spec.getString();
}

bool FunctionModelSpectra::isContinuous() const { return m_isContinuous; }

FitDomainIndex FunctionModelSpectra::indexOf(WorkspaceIndex i) const {
  auto const it = std::find(begin(), end(), i);
  if (it == end()) {
    throw std::runtime_error("Spectrum index " + std::to_string(i.value) + " not found.");
  }
  return FitDomainIndex{static_cast<size_t>(std::distance(begin(), it))};
}

FunctionModelSpectra FunctionModelSpectra::combine(const FunctionModelSpectra &other) const {
  std::set<WorkspaceIndex> indices(begin(), end());
  indices.insert(other.begin(), other.end());
  return FunctionModelSpectra(indices);
}

FunctionModelSpectra::FunctionModelSpectra(const std::set<WorkspaceIndex> &indices)
    : m_vec(indices.begin(), indices.end()) {
  checkContinuous();
}

void FunctionModelSpectra::checkContinuous() {
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

void FunctionModelSpectra::erase(WorkspaceIndex workspaceIndex) {
  auto iteratorToErase = std::find(m_vec.begin(), m_vec.end(), workspaceIndex);
  if (iteratorToErase != m_vec.end()) {
    m_vec.erase(iteratorToErase);
    checkContinuous();
  }
}

} // namespace MantidWidgets
} // namespace MantidQt
