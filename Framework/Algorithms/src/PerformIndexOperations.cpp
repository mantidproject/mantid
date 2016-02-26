#include "MantidAlgorithms/PerformIndexOperations.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/Strings.h"
#include <boost/regex.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/algorithm/string.hpp>

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace {
/**
 * Command class for executing algorithms on workspaces and appending resulting
 * workspace together.
 */
class Command {
public:
  virtual bool isValid() const { return true; }

  virtual MatrixWorkspace_sptr execute(MatrixWorkspace_sptr input) const = 0;

  virtual MatrixWorkspace_sptr
  executeAndAppend(MatrixWorkspace_sptr inputWS,
                   MatrixWorkspace_sptr toAppend) const {
    if (!this->isValid()) {
      return toAppend;
    } else {
      MatrixWorkspace_sptr current = this->execute(inputWS);
      Mantid::API::AlgorithmManagerImpl &factory =
          Mantid::API::AlgorithmManager::Instance();
      auto conjoinWorkspaceAlg = factory.create("ConjoinWorkspaces");
      conjoinWorkspaceAlg->setChild(true);
      conjoinWorkspaceAlg->initialize();
      conjoinWorkspaceAlg->setProperty("InputWorkspace1", toAppend);
      conjoinWorkspaceAlg->setProperty("InputWorkspace2", current);
      conjoinWorkspaceAlg->execute();
      MatrixWorkspace_sptr outWS =
          conjoinWorkspaceAlg->getProperty("InputWorkspace1");
      return outWS;
    }
  }

  virtual ~Command() {}
};

/// Helper typedef
typedef std::vector<boost::shared_ptr<Command>> VecCommands;

/**
 * Command yielding no result.
 */
class NullCommand : public Command {
  bool isValid() const override { return false; }
  MatrixWorkspace_sptr execute(MatrixWorkspace_sptr /*input*/) const override {
    throw std::runtime_error(
        "Should not be attempting ::execute on a NullCommand");
  }
  ~NullCommand() override {}
};

/**
 * Addition command for summing spectra together.
 */
class AdditionCommand : public Command {
private:
  std::vector<int> m_indexes;

public:
  explicit AdditionCommand(const std::vector<int> &indexes)
      : m_indexes(indexes) {}

  MatrixWorkspace_sptr execute(MatrixWorkspace_sptr inputWS) const override {
    MatrixWorkspace_sptr outWS;
    if (m_indexes.size() > 0) {
      Mantid::API::AlgorithmManagerImpl &factory =
          Mantid::API::AlgorithmManager::Instance();
      auto sumSpectraAlg = factory.create("SumSpectra");
      sumSpectraAlg->setChild(true);
      sumSpectraAlg->initialize();
      sumSpectraAlg->setProperty("InputWorkspace", inputWS);
      sumSpectraAlg->setProperty("ListOfWorkspaceIndices", m_indexes);
      sumSpectraAlg->setPropertyValue("OutputWorkspace", "outWS");
      sumSpectraAlg->execute();
      outWS = sumSpectraAlg->getProperty("OutputWorkspace");
    }
    return outWS;
  }

  ~AdditionCommand() override {}
};

/**
 * Command for cropping spectra out of a workspace as a new workspace
 */
class CropCommand : public Command {
private:
  std::vector<int> m_indexes;

public:
  explicit CropCommand(const std::vector<int> &indexes) : m_indexes(indexes) {}

  MatrixWorkspace_sptr execute(MatrixWorkspace_sptr inputWS) const override {

    MatrixWorkspace_sptr outWS;
    for (size_t i = 0; i < m_indexes.size(); ++i) {
      Mantid::API::AlgorithmManagerImpl &factory =
          Mantid::API::AlgorithmManager::Instance();
      auto cropWorkspaceAlg = factory.create("CropWorkspace");
      cropWorkspaceAlg->setChild(true);
      cropWorkspaceAlg->initialize();
      cropWorkspaceAlg->setProperty("InputWorkspace", inputWS);
      cropWorkspaceAlg->setProperty("StartWorkspaceIndex", m_indexes[i]);
      cropWorkspaceAlg->setProperty("EndWorkspaceIndex", m_indexes[i]);
      cropWorkspaceAlg->setPropertyValue("OutputWorkspace", "outWS");
      cropWorkspaceAlg->execute();
      MatrixWorkspace_sptr subRange =
          cropWorkspaceAlg->getProperty("OutputWorkspace");
      if (i == 0) {
        outWS = subRange;
      } else {
        auto conjoinWorkspaceAlg = factory.create("ConjoinWorkspaces");
        conjoinWorkspaceAlg->setChild(true);
        conjoinWorkspaceAlg->initialize();
        conjoinWorkspaceAlg->setProperty("InputWorkspace1", outWS);
        conjoinWorkspaceAlg->setProperty("InputWorkspace2", subRange);
        conjoinWorkspaceAlg->execute();
        outWS = conjoinWorkspaceAlg->getProperty("InputWorkspace1");
      }
    }
    return outWS;
  }
  ~CropCommand() override {}
};

/**
 * Abstract type. Command parsing interface.
 */
class CommandParser {
public:
  virtual Command *interpret(const std::string &instruction) const = 0;

  virtual ~CommandParser() {}
};

/// Helper typedef for vector of command parsers
typedef std::vector<boost::shared_ptr<CommandParser>> VecCommandParsers;

/**
 * Command parser base class for common concrete command parser types.
 */
template <typename ProductType> class CommandParserBase : public CommandParser {
public:
  Command *interpret(const std::string &instruction) const override {
    Command *command = nullptr;
    boost::regex ex = getRegex();
    if (boost::regex_match(instruction, ex)) {
      auto indexes =
          Mantid::Kernel::Strings::parseRange(instruction, ",", getSeparator());
      command = new ProductType(indexes);
    } else {
      command = new NullCommand;
    }
    return command;
  }
  ~CommandParserBase() override {}

private:
  virtual std::string getSeparator() const = 0;
  virtual boost::regex getRegex() const = 0;
};

/**
 * Parser to interpret Range Addition instructions.
 */
class AdditionParserRange : public CommandParserBase<AdditionCommand> {
public:
  ~AdditionParserRange() override {}

private:
  boost::regex getRegex() const override {
    return boost::regex("^\\s*[0-9]+\\s*\\-\\s*[0-9]+\\s*$");
  }
  std::string getSeparator() const override { return "-"; }
};

/**
 * Parser to interpret Addition instructions.
 */
class AdditionParser : public CommandParser {
public:
  ~AdditionParser() override {}

  Command *interpret(const std::string &instruction) const override {
    Command *command = nullptr;
    boost::regex ex("^\\s*[0-9]+\\s*\\+\\s*[0-9]+\\s*$");
    if (boost::regex_match(instruction, ex)) {
      std::vector<std::string> arguments;
      boost::split(arguments, instruction, boost::is_any_of("+"));
      int minIndex = -1;
      int maxIndex = -1;
      Mantid::Kernel::Strings::convert<int>(arguments.front(), minIndex);
      Mantid::Kernel::Strings::convert<int>(arguments.back(), maxIndex);
      std::vector<int> indexes;
      indexes.push_back(minIndex);
      indexes.push_back(maxIndex);
      command = new AdditionCommand(indexes);
    } else {
      command = new NullCommand;
    }
    return command;
  }
};

/**
 * Parser to interpret Crop Range instructions.
 */
class CropParserRange : public CommandParserBase<CropCommand> {
public:
  ~CropParserRange() override {}

private:
  boost::regex getRegex() const override {
    return boost::regex("^\\s*[0-9]+\\s*:\\s*[0-9]+\\s*$");
  }
  std::string getSeparator() const override { return ":"; }
};

/**
 * Parser to interpret single index cropping instructions
 */
class CropParserIndex : public CommandParser {
public:
  ~CropParserIndex() override {}

  Command *interpret(const std::string &instruction) const override {
    Command *command = nullptr;
    boost::regex ex("^\\s*[0-9]+\\s*$");
    if (boost::regex_match(instruction, ex)) {
      int index = -1;
      Mantid::Kernel::Strings::convert<int>(instruction, index);
      std::vector<int> indexes(1, index);
      command = new CropCommand(indexes);
    } else {
      command = new NullCommand;
    }
    return command;
  }
};
}

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(PerformIndexOperations)

//------------------------------------------------------------------------------
/** Constructor
 */
PerformIndexOperations::PerformIndexOperations() {}

//------------------------------------------------------------------------------
/** Destructor
 */
PerformIndexOperations::~PerformIndexOperations() {}

//------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string PerformIndexOperations::name() const {
  return "PerformIndexOperations";
}

/// Algorithm's version for identification. @see Algorithm::version
int PerformIndexOperations::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string PerformIndexOperations::category() const {
  return "Transforms\\Grouping";
}

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void PerformIndexOperations::init() {
  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "Input to processes workspace.");
  declareProperty(make_unique<PropertyWithValue<std::string>>(
                      "ProcessingInstructions", "", Direction::Input),
                  "Processing instructions. See full instruction list.");
  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "Output processed workspace");
}

/**
 * Interpret the instructions as an ordered list of commands that can be
 *executed later.
 *
 * @param processingInstructions : Instructions to process
 * @return Vector of Commands. Commands wrap mantid-algorithmic steps to achieve
 *the desired result.
 */
VecCommands interpret(const std::string &processingInstructions) {
  std::vector<std::string> processingInstructionsSplit;
  boost::split(processingInstructionsSplit, processingInstructions,
               boost::is_any_of(","));

  VecCommandParsers commandParsers;
  commandParsers.push_back(boost::make_shared<AdditionParserRange>());
  commandParsers.push_back(boost::make_shared<CropParserRange>());
  commandParsers.push_back(boost::make_shared<CropParserIndex>());
  commandParsers.push_back(boost::make_shared<AdditionParser>());

  VecCommands commands;
  for (auto candidate : processingInstructionsSplit) {
    bool parserFound = false;
    for (auto commandParser : commandParsers) {
      Command *command = commandParser->interpret(candidate);
      boost::shared_ptr<Command> commandSptr(command);
      if (commandSptr->isValid()) // Do not record invalid commands.
      {
        parserFound = true;
        commands.push_back(commandSptr);
      }
    }
    if (!parserFound) {
      throw std::invalid_argument("Cannot interpret " + candidate);
    }
  }
  return commands;
}

//------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void PerformIndexOperations::exec() {
  MatrixWorkspace_sptr inputWorkspace = this->getProperty("InputWorkspace");
  const std::string processingInstructions =
      this->getProperty("ProcessingInstructions");

  boost::regex re(
      "^\\s*[0-9]+\\s*$|^(\\s*,*[0-9]+(\\s*(,|:|\\+|\\-)\\s*)*[0-9]*)*$");
  if (!boost::regex_match(processingInstructions, re)) {
    throw std::invalid_argument("ProcessingInstructions are not well formed: " +
                                processingInstructions);
  }

  if (processingInstructions.empty()) {
    auto cloneWS = this->createChildAlgorithm("CloneWorkspace");
    cloneWS->initialize();
    cloneWS->setProperty("InputWorkspace", inputWorkspace);
    cloneWS->execute();
    Workspace_sptr tmp = cloneWS->getProperty("OutputWorkspace");
    MatrixWorkspace_sptr outWS =
        boost::dynamic_pointer_cast<MatrixWorkspace>(tmp);
    this->setProperty("OutputWorkspace", outWS);
  } else {
    // Interpret the instructions.
    VecCommands commands = interpret(processingInstructions);

    // Execute the commands.
    auto command = commands[0];
    MatrixWorkspace_sptr outWS = command->execute(inputWorkspace);
    for (size_t j = 1; j < commands.size(); ++j) {
      outWS = commands[j]->executeAndAppend(inputWorkspace, outWS);
    }

    this->setProperty("OutputWorkspace", outWS);
  }
}

} // namespace Algorithms
} // namespace Mantid
