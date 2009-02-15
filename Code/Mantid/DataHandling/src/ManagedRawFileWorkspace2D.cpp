#include "MantidDataHandling/ManagedRawFileWorkspace2D.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/RefAxis.h"
#include "MantidAPI/LocatedDataRef.h"
#include "MantidAPI/WorkspaceIterator.h"
#include "MantidAPI/WorkspaceIteratorCode.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/ConfigService.h"

#include "LoadRaw/isisraw2.h"
#include <boost/timer.hpp>
#include "Poco/File.h"
#include "Poco/Path.h"
#include "Poco/Exception.h"

//DECLARE_WORKSPACE(ManagedRawFileWorkspace2D)

namespace Mantid
{
  namespace DataHandling
  {

    // Get a reference to the logger
    Kernel::Logger& ManagedRawFileWorkspace2D::g_log = Kernel::Logger::get("ManagedRawFileWorkspace2D");

    // Initialise the instance count
    int ManagedRawFileWorkspace2D::g_uniqueID = 1;    
    
    /// Constructor
    ManagedRawFileWorkspace2D::ManagedRawFileWorkspace2D():
    isisRaw(new ISISRAW2),m_fileRaw(NULL),m_readIndex(0)
    {
    }

    ///Destructor
    ManagedRawFileWorkspace2D::~ManagedRawFileWorkspace2D()
    {
        delete isisRaw;
        if (m_fileRaw) fclose(m_fileRaw);
        removeTempFile();
    }

     /** Sets the RAW file for this workspace.
     \param fileName The path to the RAW file.
     \param opt Caching option.  0 - cache on local drive if raw file is very slow to read.
            1 - cache anyway, 2 - never cache.
     */
    void ManagedRawFileWorkspace2D::setRawFile(const std::string& fileName,int opt)
    {
        m_filenameRaw = fileName;
        m_fileRaw = fopen(m_filenameRaw.c_str(),"rb");
        if (m_fileRaw == NULL)
        {
            g_log.error("Unable to open file " + m_filenameRaw);
            throw Kernel::Exception::FileError("Unable to open File:" , m_filenameRaw);
        }

        if ( needCache(opt) )
        {
            openTempFile();
        }

        isisRaw->ioRAW(m_fileRaw, true);

        m_numberOfBinBoundaries = isisRaw->t_ntc1 + 1;    
        m_numberOfPeriods = isisRaw->t_nper;
        initialize(isisRaw->t_nsp1,m_numberOfBinBoundaries,isisRaw->t_ntc1);
        int noOfBlocks = m_noVectors / m_vectorsPerBlock;
        if ( noOfBlocks * m_vectorsPerBlock != m_noVectors ) ++noOfBlocks;
        m_changedBlock.resize(noOfBlocks,false);

        float* timeChannels = new float[m_numberOfBinBoundaries];
        isisRaw->getTimeChannels(timeChannels, m_numberOfBinBoundaries);
        isisRaw->skipData(0);
        fgetpos(m_fileRaw, &m_data_pos); //< Save the data start position.
        
        m_timeChannels.reset(new std::vector<double>(timeChannels, timeChannels + m_numberOfBinBoundaries));
        getAxis(0)->unit() = Kernel::UnitFactory::Instance().create("TOF");
    }


    static double dblSqrt(double in)
    {
      return sqrt(in);
    }

    // readData(int) should be changed to readNextSpectrum() returning the spectrum index
    // and skipData to skipNextSpectrum()
    //void ManagedRawFileWorkspace2D::readSpectrum(int index)const
    void ManagedRawFileWorkspace2D::readDataBlock(DataObjects::ManagedDataBlock2D *newBlock,int startIndex)const
    {
        Poco::ScopedLock<Poco::FastMutex> mutex(m_mutex);
        if (!m_fileRaw)
        {
            g_log.error("Raw file was not open.");
            throw std::runtime_error("Raw file was not open.");
        }
        int blockIndex = startIndex / m_vectorsPerBlock;
        // Modified data is stored in ManagedWorkspace2D flat file.
        if (m_changedBlock[blockIndex])
        {
            ManagedWorkspace2D::readDataBlock(newBlock,startIndex);
            return;
        }

        if (startIndex > m_readIndex)
        {
            while(startIndex > m_readIndex)
            {
                isisRaw->skipData(m_readIndex+1);// Adding 1 because we dropped the first spectrum.
                ++m_readIndex;
            }
        }
        else
        {
            int nwords = 0;
            while(startIndex < m_readIndex)
            {
                --m_readIndex;
                nwords += 4*isisRaw->ddes[m_readIndex+1].nwords;
            }
            if (fseek(m_fileRaw,-nwords,SEEK_CUR) != 0)
            {
                fclose(m_fileRaw);
                removeTempFile();
                g_log.error("Error reading RAW file.");
                throw std::runtime_error("ManagedRawFileWorkspace2D: Error reading RAW file.");
            }
        }
        int endIndex = startIndex+m_vectorsPerBlock < m_noVectors?startIndex+m_vectorsPerBlock:m_noVectors;
        if (endIndex >= m_noVectors) endIndex = m_noVectors;
        for(int index = startIndex;index<endIndex;index++,m_readIndex++)
        {
            isisRaw->readData(m_readIndex+1);
            std::vector<double> y(isisRaw->dat1 + 1, isisRaw->dat1 + m_numberOfBinBoundaries);       
            std::vector<double> e(m_numberOfBinBoundaries-1);
            std::transform(y.begin(), y.end(), e.begin(), dblSqrt);
            newBlock->setX(index,m_timeChannels);
            newBlock->setData(index,y,e);
        }
        newBlock->hasChanges(false);
    }

    void ManagedRawFileWorkspace2D::writeDataBlock(DataObjects::ManagedDataBlock2D *toWrite)
    {
        Poco::ScopedLock<Poco::FastMutex> mutex(m_mutex);
        ManagedWorkspace2D::writeDataBlock(toWrite);
        int blockIndex = toWrite->minIndex() / m_vectorsPerBlock;
        m_changedBlock[blockIndex] = toWrite->hasChanges();
    }

    /**
        Decides if the raw file must be copied to a cache file on the local drive to improve reading time.
    */
    bool ManagedRawFileWorkspace2D::needCache(int opt)
    {
        if (opt == 1) return true;
        if (opt == 2) return false;

        FILE * ff = fopen(m_filenameRaw.c_str(),"rb");
        if (ff)
        {
            size_t n = 100000;
            char *buf = new char[n];
            boost::timer tim;
            size_t i = fread(buf,sizeof(char),n,ff);
            double elapsed = tim.elapsed();
            fclose(ff);
            delete[] buf;
            if (i > 0)
            {
                return elapsed > 0.01;
            }
        }
        return false;
    }

    void ManagedRawFileWorkspace2D::openTempFile()
    {
        // Look for the (optional) path from the configuration file
        std::string path = Kernel::ConfigService::Instance().getString("ManagedWorkspace.FilePath");
        if ( !path.empty() )
        {
            if ( ( *(path.rbegin()) != '/' ) && ( *(path.rbegin()) != '\\' ) )
            {
                path.push_back('/');
            }
        }
	
	if( !Poco::File(path).exists() )
	{
	  path = "./";
	}

	if( !Poco::File(path).canWrite() )
	{
	  throw std::runtime_error(std::string("Temporary file path '") + path + "' is not writable");
	}

        std::stringstream filename;
        filename << "WS2D_" << Poco::Path(m_filenameRaw).getBaseName() <<'_'<< ManagedRawFileWorkspace2D::g_uniqueID <<".raw";
        // Increment the instance count
        ++ManagedRawFileWorkspace2D::g_uniqueID;
        m_tempfile = path + filename.str();
	Poco::File(m_filenameRaw).copyTo(m_tempfile);

        FILE *fileRaw = fopen(m_tempfile.c_str(),"rb");
        if (fileRaw)
        {
            fclose(m_fileRaw);
            m_fileRaw = fileRaw;
        }

    }

    void ManagedRawFileWorkspace2D::removeTempFile() const
    {
      if (!m_tempfile.empty()) Poco::File(m_tempfile).remove();
    }

  } // namespace DataHandling
} //NamespaceMantid


///\endcond TEMPLATE
