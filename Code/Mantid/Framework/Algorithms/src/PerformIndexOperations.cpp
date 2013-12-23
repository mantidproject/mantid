/*WIKI*
 *
 * Peforms index operations on a workspace which involve cropping out spectra and summing spectra together. See [[MultiFileLoading]] for the syntax to use.
 *
 *WIKI*/

#include "MantidAlgorithms/PerformIndexOperations.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/Strings.h"
#include <boost/regex.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/algorithm/string.hpp>

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace
{
  class Command
  {
  public:

    virtual bool isValid() const
    {
      return true;
    }

    virtual MatrixWorkspace_sptr execute(MatrixWorkspace_sptr input) const = 0;

    virtual MatrixWorkspace_sptr executeAndAppend(MatrixWorkspace_sptr inputWS,
        MatrixWorkspace_sptr toAppend) const
    {
      if (!this->isValid())
      {
        return toAppend;
      }
      else
      {
        MatrixWorkspace_sptr current = this->execute(inputWS);
        Mantid::API::AlgorithmManagerImpl& factory = Mantid::API::AlgorithmManager::Instance();
        auto conjoinWorkspaceAlg = factory.create("ConjoinWorkspaces");
        conjoinWorkspaceAlg->setChild(true);
        conjoinWorkspaceAlg->initialize();
        conjoinWorkspaceAlg->setProperty("InputWorkspace1", toAppend);
        conjoinWorkspaceAlg->setProperty("InputWorkspace2", current);
        conjoinWorkspaceAlg->execute();
        MatrixWorkspace_sptr outWS = conjoinWorkspaceAlg->getProperty("InputWorkspace1");
        return outWS;
      }
    }

    virtual ~Command()
    {
    }
  };

  typedef std::vector<boost::shared_ptr<Command> > VecCommands;

  class NullCommand: public Command
  {
    virtual bool isValid() const
    {
      return false;
    }
    virtual MatrixWorkspace_sptr execute(MatrixWorkspace_sptr inputWS) const
    {
      throw std::runtime_error("Should not be attempting ::execute on a NullCommand");
    }
    virtual ~NullCommand()
    {
    }
  };

  class AdditionCommand: public Command
  {
  private:
    std::vector<int> m_indexes;
  public:
    AdditionCommand(const std::vector<int>& indexes) :
        m_indexes(indexes)
    {
    }

    virtual MatrixWorkspace_sptr execute(MatrixWorkspace_sptr inputWS) const
    {
      MatrixWorkspace_sptr outWS;
      if (m_indexes.size() > 0)
      {
        Mantid::API::AlgorithmManagerImpl& factory = Mantid::API::AlgorithmManager::Instance();
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

    virtual ~AdditionCommand()
    {
    }
  };

  class CropCommand: public Command
  {
  private:
    std::vector<int> m_indexes;
  public:
    CropCommand(const std::vector<int>& indexes) :
        m_indexes(indexes)
    {
    }

    MatrixWorkspace_sptr execute(MatrixWorkspace_sptr inputWS) const
    {

      MatrixWorkspace_sptr outWS;

      for (size_t i = 0; i < m_indexes.size(); ++i)
      {
        Mantid::API::AlgorithmManagerImpl& factory = Mantid::API::AlgorithmManager::Instance();
        auto cropWorkspaceAlg = factory.create("CropWorkspace");
        cropWorkspaceAlg->setChild(true);
        cropWorkspaceAlg->initialize();
        cropWorkspaceAlg->setProperty("InputWorkspace", inputWS);
        cropWorkspaceAlg->setProperty("StartWorkspaceIndex", m_indexes[i]);
        cropWorkspaceAlg->setProperty("EndWorkspaceIndex", m_indexes[i]);
        cropWorkspaceAlg->setPropertyValue("OutputWorkspace", "outWS");
        cropWorkspaceAlg->execute();
        MatrixWorkspace_sptr subRange = cropWorkspaceAlg->getProperty("OutputWorkspace");
        if (i == 0)
        {
          outWS = subRange;
        }
        else
        {
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
    virtual ~CropCommand()
    {
    }
  };

  class CommandParser
  {
  public:
    virtual Command* interpret(const std::string& instruction) const = 0;

    virtual ~CommandParser()
    {
    }
  };

  typedef std::vector<boost::shared_ptr<CommandParser> > VecCommandParsers;

  template<typename ProductType>
  class CommandParserBase: public CommandParser
  {
  public:
    virtual Command* interpret(const std::string& instruction) const
    {
      Command* command = NULL;
      boost::regex ex = getRegex();
      if (boost::regex_match(instruction, ex))
      {
        auto indexes = Mantid::Kernel::Strings::parseRange(instruction, ",", getSeparator());
        command = new ProductType(indexes);
      }
      else
      {
        command = new NullCommand;
      }
      return command;
    }
    virtual ~CommandParserBase()
    {
    }
  private:
    virtual std::string getSeparator() const = 0;
    virtual boost::regex getRegex() const = 0;
  };

  class AdditionParser: public CommandParserBase<AdditionCommand>
  {
  public:

    virtual ~AdditionParser()
    {
    }

  private:
    boost::regex getRegex() const
    {
      return boost::regex("\\s*[0-9]+\\s*\\-\\s*[0-9]+\\s*");
    }
    std::string getSeparator() const
    {
      return "-";
    }
  };

  class CropParserRange: public CommandParserBase<CropCommand>
  {
  public:

    virtual ~CropParserRange()
    {
    }
  private:
    boost::regex getRegex() const
    {
      return boost::regex("\\s*[0-9]+\\s*:\\s*[0-9]+\\s*");
    }
    std::string getSeparator() const
    {
      return ":";
    }
  };

  class CropParserIndex: public CommandParser
  {
  public:

    virtual ~CropParserIndex()
    {
    }

    virtual Command* interpret(const std::string& instruction) const
    {
      Command* command = NULL;
      boost::regex ex("^\\s*[0-9]+\\s*$");
      if (boost::regex_match(instruction, ex))
      {
        int index = -1;
        Mantid::Kernel::Strings::convert<int>(instruction, index);
        std::vector<int> indexes(1, index);
        command = new CropCommand(indexes);
      }
      else
      {
        command = new NullCommand;
      }
      return command;
    }

  };

}

namespace Mantid
{
  namespace Algorithms
  {

    // Register the algorithm into the AlgorithmFactory
    DECLARE_ALGORITHM(PerformIndexOperations)

    //----------------------------------------------------------------------------------------------
    /** Constructor
     */
    PerformIndexOperations::PerformIndexOperations()
    {
    }

    //----------------------------------------------------------------------------------------------
    /** Destructor
     */
    PerformIndexOperations::~PerformIndexOperations()
    {
    }

    //----------------------------------------------------------------------------------------------
    /// Algorithm's name for identification. @see Algorithm::name
    const std::string PerformIndexOperations::name() const
    {
      return "PerformIndexOperations";
    }
    ;

    /// Algorithm's version for identification. @see Algorithm::version
    int PerformIndexOperations::version() const
    {
      return 1;
    }
    ;

    /// Algorithm's category for identification. @see Algorithm::category
    const std::string PerformIndexOperations::category() const
    {
      return "Algorithms;Transforms;Splitting";
    }

    //----------------------------------------------------------------------------------------------
    /// Sets documentation strings for this algorithm
    void PerformIndexOperations::initDocs()
    {
      this->setWikiSummary("Process the workspace according to the Index operations provided.");
      this->setOptionalMessage(this->getWikiSummary());
    }

    //----------------------------------------------------------------------------------------------
    /** Initialize the algorithm's properties.
     */
    void PerformIndexOperations::init()
    {
      declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace", "", Direction::Input),
          "Input to processes workspace.");
      declareProperty(new PropertyWithValue<std::string>("ProcessingInstructions", "", Direction::Input),
          "Processing instructions. See full instruction list.");
      declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace", "", Direction::Output),
          "Output processed workspace");
    }

    //----------------------------------------------------------------------------------------------
    /** Execute the algorithm.
     */
    void PerformIndexOperations::exec()
    {
      MatrixWorkspace_sptr inputWorkspace = this->getProperty("InputWorkspace");
      const std::string processingInstructions = this->getProperty("ProcessingInstructions");

      boost::regex re("^\\s*[0-9]+\\s*$|^(\\s*,*[0-9]+(\\s*(,|:|\\+|\\-)\\s*)*[0-9]*)*$");
      if (!boost::regex_match(processingInstructions, re))
      {
        throw std::invalid_argument(
            "ProcessingInstructions are not well formed: " + processingInstructions);
      }

      if (processingInstructions.empty())
      {
        auto cloneWS = this->createChildAlgorithm("CloneWorkspace");
        cloneWS->initialize();
        cloneWS->setProperty("InputWorkspace", inputWorkspace);
        cloneWS->execute();
        Workspace_sptr tmp = cloneWS->getProperty("OutputWorkspace");
        MatrixWorkspace_sptr outWS = boost::dynamic_pointer_cast<MatrixWorkspace>(tmp);
        this->setProperty("OutputWorkspace", outWS);
      }
      else
      {
        std::vector<std::string> processingInstructionsSplit;
        boost::split(processingInstructionsSplit, processingInstructions, boost::is_any_of(","));

        VecCommandParsers commandParsers;
        commandParsers.push_back(boost::make_shared<AdditionParser>());
        commandParsers.push_back(boost::make_shared<CropParserRange>());
        commandParsers.push_back(boost::make_shared<CropParserIndex>());

        VecCommands commands;
        for (auto it = processingInstructionsSplit.begin(); it != processingInstructionsSplit.end(); ++it)
        {
          const std::string candidate = *it;
          for (auto parserIt = commandParsers.begin(); parserIt != commandParsers.end(); ++parserIt)
          {
            auto commandParser = *parserIt;
            Command* command = commandParser->interpret(candidate);
            boost::shared_ptr<Command> commandSptr(command);
            if(commandSptr->isValid())
              commands.push_back(commandSptr);
          }
        }

        auto command = commands[0];
        MatrixWorkspace_sptr outWS = command->execute(inputWorkspace);
        for (int j = 1; j < commands.size(); ++j)
        {
          outWS = commands[j]->executeAndAppend(inputWorkspace, outWS);
        }

        this->setProperty("OutputWorkspace", outWS);

      }

    }

  } // namespace Algorithms
} // namespace Mantid
