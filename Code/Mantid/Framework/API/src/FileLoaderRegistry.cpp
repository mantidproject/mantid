#include "MantidAPI/FileLoaderRegistry.h"
#include "MantidAPI/IFileLoader.h"
#include "MantidAPI/IHDFFileLoader.h"
#include "MantidKernel/FileDescriptor.h"

#include <Poco/File.h>

namespace Mantid
{
  namespace API
  {
    namespace
    {
      //----------------------------------------------------------------------------------------------
      // Anonymous namespace helpers
      //----------------------------------------------------------------------------------------------
      /// @cond
      template<typename T>
      struct DescriptorCallback
      {
        void apply(T &) {} //general one does nothing
      };
      template<>
      struct DescriptorCallback<Kernel::FileDescriptor>
      {
        void apply(Kernel::FileDescriptor & descriptor) { descriptor.resetStreamToStart(); }
      };
      ///endcond

      /**
       * @param descriptor A descriptor object describing the file
       * @param names The collection of names to search through
       * @param logger A reference to a Mantid Logger object
       * @return  A string containing the name of an algorithm to load the file, or an empty string if nothing
       * was found
       */
      template<typename DescriptorType, typename FileLoaderType>
      const std::string searchForLoader(DescriptorType & descriptor,const std::set<std::string> & names,
                                        Kernel::Logger & logger)
      {
        const auto & factory = AlgorithmFactory::Instance();
        std::string bestLoader;
        int maxConfidence(0);
        DescriptorCallback<DescriptorType> callback;

        auto iend = names.end();
        for(auto it = names.begin(); it != iend; ++it)
        {
          const std::string & name = *it;
          logger.debug() << "Checking " << name << std::endl;

          auto alg = boost::static_pointer_cast<FileLoaderType>(factory.create(name, -1)); // highest version
          try
          {
            const int confidence = alg->confidence(descriptor);
            callback.apply(descriptor);

            if(confidence > maxConfidence) // strictly greater
            {
              bestLoader = name;
              maxConfidence = confidence;
            }
          }
          catch(std::exception & exc)
          {
            logger.warning() << "Checking loader '" << name << "' raised an error: '" << exc.what() << "'. Loader skipped." << std::endl;
          }
        }
        return bestLoader;
      }
    }// end anonymous


    //----------------------------------------------------------------------------------------------
    // Public members
    //----------------------------------------------------------------------------------------------
    /**
     * Creates an empty registry
     */
    FileLoaderRegistry::FileLoaderRegistry() :
        m_names(2, std::set<std::string>()), m_totalSize(0),
        m_log(Kernel::Logger::get("FileLoaderRegistry"))
    {
    }

    /**
     * Queries each registered algorithm and asks it how confident it is that it can
     * load the given file. The name of the one with the highest confidence is returned.
     * @param filename A full file path pointing to an existing file
     * @return A string containing the name of an algorithm to load the file
     * @throws Exception::NotFoundError if an algorithm cannot be found
     */
    const std::string FileLoaderRegistry::chooseLoader(const std::string &filename) const
    {
      using Kernel::FileDescriptor;
      using Kernel::HDFDescriptor;

      m_log.debug() << "Trying to find loader for '" << filename << "'" << std::endl;

      std::string bestLoader;
      if(HDFDescriptor::isHDF(filename))
      {
        m_log.debug() << filename << " looks like a HDF file. Checking registered HDF loaders\n";
        HDFDescriptor descriptor(filename);
        bestLoader = searchForLoader<HDFDescriptor,IHDFFileLoader>(descriptor, m_names[HDF], m_log);
      }
      else
      {
        m_log.debug() << "Checking registered non-HDF loaders\n";
        FileDescriptor descriptor(filename);
        bestLoader = searchForLoader<FileDescriptor,IFileLoader>(descriptor, m_names[NonHDF], m_log);
      }

      if(bestLoader.empty())
      {
        throw Kernel::Exception::NotFoundError(filename, "Unable to find loader");
      }
      m_log.debug() << "Found loader " << bestLoader << " for file '" << filename << "'" << std::endl;
      return bestLoader;
    }


    //----------------------------------------------------------------------------------------------
    // Private members
    //----------------------------------------------------------------------------------------------

  } // namespace API
} // namespace Mantid
