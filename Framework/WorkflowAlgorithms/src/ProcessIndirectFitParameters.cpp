// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidWorkflowAlgorithms/ProcessIndirectFitParameters.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/TextAxis.h"

#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/UnitFactory.h"

#include <utility>

namespace {
using namespace Mantid::API;

template <typename T, typename... Ts> std::vector<T, Ts...> repeat(std::vector<T, Ts...> const &vec, std::size_t n) {
  std::vector<T, Ts...> result;
  result.reserve(vec.size() * n);
  for (; n > 0; --n)
    result.insert(result.end(), vec.begin(), vec.end());
  return result;
}

template <typename T> std::vector<T> getIncrementingSequence(const T &from, std::size_t length) {
  std::vector<T> sequence(length);
  std::iota(sequence.begin(), sequence.end(), from);
  return sequence;
}

std::vector<std::string> appendSuffix(std::vector<std::string> const &vec, std::string const &suffix) {
  std::vector<std::string> appended;
  appended.reserve(vec.size());
  std::transform(vec.cbegin(), vec.cend(), std::back_inserter(appended),
                 [&suffix](auto &&str) { return str + suffix; });
  return appended;
}

MatrixWorkspace_sptr createWorkspace(std::vector<double> const &x, std::vector<double> const &y,
                                     std::vector<double> const &e, int numberOfSpectra,
                                     std::vector<std::string> const &verticalAxisNames, std::string const &unitX) {
  auto createWorkspaceAlgorithm = AlgorithmManager::Instance().createUnmanaged("CreateWorkspace");
  createWorkspaceAlgorithm->initialize();
  createWorkspaceAlgorithm->setChild(true);
  createWorkspaceAlgorithm->setLogging(false);
  createWorkspaceAlgorithm->setProperty("DataX", x);
  createWorkspaceAlgorithm->setProperty("DataY", y);
  createWorkspaceAlgorithm->setProperty("DataE", e);
  createWorkspaceAlgorithm->setProperty("NSpec", numberOfSpectra);
  createWorkspaceAlgorithm->setProperty("VerticalAxisUnit", "Text");
  createWorkspaceAlgorithm->setProperty("VerticalAxisValues", verticalAxisNames);
  createWorkspaceAlgorithm->setProperty("UnitX", unitX);
  createWorkspaceAlgorithm->setProperty("OutputWorkspace", "__created");
  createWorkspaceAlgorithm->execute();
  return createWorkspaceAlgorithm->getProperty("OutputWorkspace");
}

template <typename T, typename OutputIterator>
void extractColumnValues(Column const &column, std::size_t startRow, std::size_t endRow, OutputIterator outputIt) {
  for (auto i = startRow; i <= endRow; ++i)
    *outputIt++ = column.cell<T>(i);
}

template <typename T, typename OutputIterator>
void extractValuesFromColumns(std::size_t startRow, std::size_t endRow, const std::vector<Column_const_sptr> &columns,
                              OutputIterator outputIt) {
  for (auto &&column : columns)
    extractColumnValues<T>(*column, startRow, endRow, outputIt);
}

template <typename T> std::vector<T> getColumnValues(Column const &column, std::size_t startRow, std::size_t endRow) {
  std::vector<T> values;
  values.reserve(1 + (endRow - startRow));
  extractColumnValues<T>(column, startRow, endRow, std::back_inserter(values));
  return values;
}

std::vector<double> getNumericColumnValuesOrIndices(Column const &column, std::size_t startRow, std::size_t endRow) {
  auto const length = startRow > endRow ? 0 : 1 + endRow - startRow;
  if (column.isNumber())
    return getColumnValues<double>(column, startRow, endRow);
  return getIncrementingSequence(0.0, length);
}

std::string getColumnName(const Column_const_sptr &column) { return column->name(); }

std::vector<std::string> extractColumnNames(std::vector<Column_const_sptr> const &columns) {
  std::vector<std::string> names;
  names.reserve(columns.size());
  std::transform(columns.begin(), columns.end(), std::back_inserter(names), getColumnName);
  return names;
}

template <typename ColumnFilter>
std::vector<Column_const_sptr> extractColumns(ITableWorkspace const *table, ColumnFilter const &filter) {
  std::vector<Column_const_sptr> columns;
  for (auto i = 0u; i < table->columnCount(); ++i) {
    auto const column = table->getColumn(i);
    if (filter(*column))
      columns.emplace_back(column);
  }
  return columns;
}

struct TableToMatrixWorkspaceConverter {
  template <typename YFilter, typename EFilter>
  TableToMatrixWorkspaceConverter(ITableWorkspace const *table, std::vector<double> x, YFilter const &yFilter,
                                  EFilter const &eFilter)
      : m_x(std::move(x)), m_yColumns(extractColumns(table, yFilter)), m_eColumns(extractColumns(table, eFilter)),
        m_yAxis(extractColumnNames(m_yColumns)) {}

  MatrixWorkspace_sptr operator()(std::size_t startRow, std::size_t endRow, std::string const &unitX,
                                  bool includeChiSquared) const {
    auto const x = repeat(m_x, m_yColumns.size());

    std::vector<double> y;
    std::vector<double> e;
    y.reserve(x.size());
    e.reserve(x.size());
    extractValuesFromColumns<double>(startRow, endRow, m_yColumns, std::back_inserter(y));
    extractValuesFromColumns<double>(startRow, endRow, m_eColumns, std::back_inserter(e));

    if (includeChiSquared)
      std::fill_n(std::back_inserter(e), y.size() - e.size(), 0.0);

    return createWorkspace(x, y, e, static_cast<int>(m_yColumns.size()), m_yAxis, unitX);
  }

private:
  std::vector<double> const m_x;
  std::vector<Column_const_sptr> const m_yColumns;
  std::vector<Column_const_sptr> const m_eColumns;
  std::vector<std::string> const m_yAxis;
};

struct EndsWithOneOf {
  explicit EndsWithOneOf(std::vector<std::string> &&strings) : m_strings(std::move(strings)) {}

  bool operator()(std::string const &value) const {
    for (auto &&str : m_strings) {
      if (value.ends_with(str))
        return true;
    }
    return false;
  }

private:
  std::vector<std::string> const m_strings;
};

template <typename StringFilter> struct ColumnNameFilter {
public:
  explicit ColumnNameFilter(StringFilter &&filter) : m_filter(std::forward<StringFilter>(filter)) {}

  bool operator()(Column const &column) const { return m_filter(column.name()); }

private:
  StringFilter const m_filter;
};

template <typename StringFilter> ColumnNameFilter<StringFilter> makeColumnNameFilter(StringFilter &&filter) {
  return ColumnNameFilter<StringFilter>(std::forward<StringFilter>(filter));
}
} // namespace

namespace Mantid::Algorithms {

using namespace API;
using namespace Kernel;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ProcessIndirectFitParameters)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string ProcessIndirectFitParameters::name() const { return "ProcessIndirectFitParameters"; }

/// Algorithm's version for identification. @see Algorithm::version
int ProcessIndirectFitParameters::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ProcessIndirectFitParameters::category() const { return "Workflow\\MIDAS"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string ProcessIndirectFitParameters::summary() const {
  return "Convert a parameter table output by PlotPeakByLogValue to a "
         "MatrixWorkspace.";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void ProcessIndirectFitParameters::init() {

  std::vector<std::string> unitOptions = UnitFactory::Instance().getKeys();
  unitOptions.emplace_back("");

  declareProperty(std::make_unique<WorkspaceProperty<ITableWorkspace>>("InputWorkspace", "", Direction::Input),
                  "The table workspace to convert to a MatrixWorkspace.");

  declareProperty("ColumnX", "", std::make_shared<MandatoryValidator<std::string>>(),
                  "The column in the table to use for the x values.", Direction::Input);

  declareProperty(std::make_unique<ArrayProperty<std::string>>(
                      "ParameterNames", std::make_shared<MandatoryValidator<std::vector<std::string>>>()),
                  "List of the parameter names to add to the workspace.");

  declareProperty("IncludeChiSquared", false, "Add Chi-squared to the output workspace.");

  declareProperty("XAxisUnit", "", std::make_shared<StringListValidator>(unitOptions),
                  "The unit to assign to the X Axis");

  auto positiveInt = std::make_shared<Kernel::BoundedValidator<int>>();
  positiveInt->setLower(0);
  declareProperty("StartRowIndex", EMPTY_INT(), positiveInt,
                  "The start row index to include in the output matrix workspace.");
  declareProperty("EndRowIndex", EMPTY_INT(), positiveInt,
                  "The end row index to include in the output matrix workspace.");

  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "The name to give the output workspace");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void ProcessIndirectFitParameters::exec() {
  ITableWorkspace_sptr const inputWs = getProperty("InputWorkspace");
  std::string xColumn = getProperty("ColumnX");
  std::string const xUnit = getProperty("XAxisUnit");
  bool const includeChiSquared = getProperty("IncludeChiSquared");
  std::vector<std::string> parameterNames = getProperty("ParameterNames");
  std::vector<std::string> errorNames = appendSuffix(parameterNames, "_Err");
  auto const startRow = getStartRow();
  auto const endRow = getEndRow(inputWs->rowCount() - 1);

  if (includeChiSquared)
    parameterNames.emplace_back("Chi_squared");

  auto const x = getNumericColumnValuesOrIndices(*inputWs->getColumn(xColumn), startRow, endRow);
  auto const yFilter = makeColumnNameFilter(EndsWithOneOf(std::move(parameterNames)));
  auto const eFilter = makeColumnNameFilter(EndsWithOneOf(std::move(errorNames)));

  TableToMatrixWorkspaceConverter converter(inputWs.get(), x, yFilter, eFilter);
  auto const output = converter(startRow, endRow, xUnit, includeChiSquared);
  setProperty("OutputWorkspace", output);
}

std::size_t ProcessIndirectFitParameters::getStartRow() const {
  int startRow = getProperty("StartRowIndex");
  return startRow == EMPTY_INT() ? 0 : static_cast<std::size_t>(startRow);
}

std::size_t ProcessIndirectFitParameters::getEndRow(std::size_t maximum) const {
  int endRow = getProperty("EndRowIndex");
  return endRow == EMPTY_INT() ? maximum : static_cast<std::size_t>(endRow);
}

} // namespace Mantid::Algorithms
