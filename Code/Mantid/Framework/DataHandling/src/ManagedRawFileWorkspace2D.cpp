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
#include "MantidKernel/System.h"

#include "LoadRaw/isisraw2.h"
#include <cstdio>
#include <boost/timer.hpp>
#include <Poco/File.h>
#include <Poco/Path.h>
#include <Poco/Exception.h>

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
    ManagedRawFileWorkspace2D::ManagedRawFileWorkspace2D(const std::string& fileName, int opt) :
      ManagedWorkspace2D(),
      isisRaw(new ISISRAW2),m_filenameRaw(fileName),m_fileRaw(NULL),m_readIndex(0),m_nmonitorSkipCounter(0)
    {
      this->setRawFile(opt);
    }

    ///Destructor
    ManagedRawFileWorkspace2D::~ManagedRawFileWorkspace2D()
    {
      if (m_fileRaw) fclose(m_fileRaw);
      removeTempFile();
    }

    /** Sets the RAW file for this workspace.
        @param opt :: Caching option.  0 - cache on local drive if raw file is very slow to read.
        1 - cache anyway, 2 - never cache.
    */
    void ManagedRawFileWorkspace2D::setRawFile(const int opt)
    {
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

      isisRaw->skipData(m_fileRaw,0);
      fgetpos(m_fileRaw, &m_data_pos); //< Save the data start position.

      getTimeChannels();
      getAxis(0)->unit() = Kernel::UnitFactory::Instance().create("TOF");
    }

    /// Constructs the time channel (X) vector(s)
    void ManagedRawFileWorkspace2D::getTimeChannels()
    {
      float* const timeChannels = new float[m_numberOfBinBoundaries];
      isisRaw->getTimeChannels(timeChannels, m_numberOfBinBoundaries);

      const int regimes = isisRaw->daep.n_tr_shift;
      if ( regimes >=2 )
      {
        g_log.debug() << "Raw file contains " << regimes << " time regimes\n"; 
        // If more than 1 regime, create a timeChannelsVec for each regime
        for (int i=0; i < regimes; ++i)
        {
          // Create a vector with the 'base' time channels
          boost::shared_ptr<MantidVec> channelsVec(
            new MantidVec(timeChannels,timeChannels + m_numberOfBinBoundaries));
          const double shift = isisRaw->daep.tr_shift[i];
          g_log.debug() << "Time regime " << i+1 << " shifted by " << shift << " microseconds\n";
          // Add on the shift for this vector
          std::transform(channelsVec->begin(), channelsVec->end(),
            channelsVec->begin(), std::bind2nd(std::plus<double>(),shift));
          m_timeChannels.push_back(channelsVec);
        }
        // In this case, also need to populate the map of spectrum-regime correspondence
        const int ndet = isisRaw->i_det;
        std::map<int,int>::iterator hint = m_specTimeRegimes.begin();
        for (int j=0; j < ndet; ++j)
        {
          // No checking for consistency here - that all detectors for given spectrum
          // are declared to use same time regime. Will just use first encountered
          hint = m_specTimeRegimes.insert(hint,std::make_pair(isisRaw->spec[j],isisRaw->timr[j]));
        }
      }
      else // Just need one in this case
      {
        boost::shared_ptr<MantidVec> channelsVec(
          new MantidVec(timeChannels,timeChannels + m_numberOfBinBoundaries));
        m_timeChannels.push_back(channelsVec);
      }
      // Done with the timeChannels C array so clean up
      delete[] timeChannels;
    }

    // Pointer to sqrt function (used in calculating errors below)
    typedef double (*uf)(double);
    static uf dblSqrt = std::sqrt;

    // readData(int) should be changed to readNextSpectrum() returning the spectrum index
    // and skipData to skipNextSpectrum()
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
  	  if(m_monitorList.size()>0)
	  {	
		  if (startIndex > m_readIndex)
		  {
			  while(startIndex > m_readIndex-m_nmonitorSkipCounter)
			  {
				  isisRaw->skipData(m_fileRaw,m_readIndex+1);// Adding 1 because we dropped the first spectrum.
				  ++m_readIndex;
				  if(isMonitor(m_readIndex))
					  ++m_nmonitorSkipCounter;
			  }
		  }
		  else
		  {
			  int nwords = 0;
			  while(startIndex+m_nmonitorSkipCounter+1 < m_readIndex)
			  {
				  if(isMonitor(m_readIndex))
					  --m_nmonitorSkipCounter;
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
		  int index=startIndex;
		  while(index<endIndex)
		  {
			  if(isMonitor(m_readIndex))
			  {	isisRaw->skipData(m_fileRaw,m_readIndex+1);
			  //g_log.error()<<"skipData called for monitor index"<<m_readIndex<<std::endl;
			  ++m_nmonitorSkipCounter;
			  ++m_readIndex;
			  }
			  else
			  {
				  isisRaw->readData(m_fileRaw,m_readIndex+1);
				  //g_log.error()<<"readData called for spectrum index"<<m_readIndex<< " and wsIndex is "<<index<< std::endl;
				  if( m_readIndex == ( m_noVectors+static_cast<int>(m_monitorList.size()) ) )
					  break;
				  MantidVec& y = newBlock->dataY(index);
				  y.assign(isisRaw->dat1 + 1, isisRaw->dat1 + m_numberOfBinBoundaries);  
				  //g_log.error()<<"readData called for m_readIndex"<<m_readIndex<< " and wsIndex is "<<index<< "Y value at 0  column is "<<y[0]<<std::endl;
				  MantidVec& e = newBlock->dataE(index);
				  std::transform(y.begin(), y.end(), e.begin(), dblSqrt);
				  if (m_timeChannels.size() == 1)
					  newBlock->setX(index,m_timeChannels[0]);
				  else
				  {
					 // std::map<int,int>::const_iterator regime = m_specTimeRegimes.find(index+1);
					  std::map<int,int>::const_iterator regime = m_specTimeRegimes.find(m_readIndex+1);
					  if ( regime == m_specTimeRegimes.end() ) 
					  {
						  g_log.error() << "Spectrum " << index << " not present in spec array:\n";
						  g_log.error(" Assuming time regime of spectrum 1");
						  regime = m_specTimeRegimes.begin();
					  }
					  newBlock->setX(index,m_timeChannels[(*regime).second-1]);
				  }
				  ++index;
				  ++m_readIndex;
			  }
		  }
 
	  }
	  else
	  {	
		  if (startIndex > m_readIndex)
		  {
			  while(startIndex > m_readIndex)
			  {
				  isisRaw->skipData(m_fileRaw,m_readIndex+1);// Adding 1 because we dropped the first spectrum.
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
			  isisRaw->readData(m_fileRaw,m_readIndex+1);
			  // g_log.error()<<"counter is "<<counter<<std::endl;
			  MantidVec& y = newBlock->dataY(index);
			  y.assign(isisRaw->dat1 + 1, isisRaw->dat1 + m_numberOfBinBoundaries);   
			  MantidVec& e = newBlock->dataE(index);
			  std::transform(y.begin(), y.end(), e.begin(), dblSqrt);
			  if (m_timeChannels.size() == 1)
				  newBlock->setX(index,m_timeChannels[0]);
			  else
			  {
				  std::map<int,int>::const_iterator regime = m_specTimeRegimes.find(index+1);
				  if ( regime == m_specTimeRegimes.end() ) 
				  {
					  g_log.error() << "Spectrum " << index << " not present in spec array:\n";
					  g_log.error(" Assuming time regime of spectrum 1");
					  regime = m_specTimeRegimes.begin();
				  }
				  newBlock->setX(index,m_timeChannels[(*regime).second-1]);
			  }

		  }
	  }

	  newBlock->hasChanges(false);
	}
	/** This method checks given spectrum is a monitor
	  * @param readIndex :: a spectrum index
	  * @return true if it's a monitor ,otherwise false
	*/
	bool ManagedRawFileWorkspace2D::isMonitor(const int readIndex)const
	{
		std::vector<int>::const_iterator itr;
		for(itr=m_monitorList.begin();itr!=m_monitorList.end();++itr)
		{
			if((*itr)==readIndex)
				return true;
		}
		return false;
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
    bool ManagedRawFileWorkspace2D::needCache(const int opt)
    {
      if (opt == 1) return true;
      if (opt == 2) return false;

      return Kernel::isNetworkDrive(m_filenameRaw);
    }

    /**   Opens a temporary file
    */
    void ManagedRawFileWorkspace2D::openTempFile()
    {
      // Look for the (optional) path from the configuration file
      std::string path = Kernel::ConfigService::Instance().getString("ManagedWorkspace.FilePath");
      if( path.empty() || !Poco::File(path).exists() || !Poco::File(path).canWrite() )
      {
        path = Mantid::Kernel::ConfigService::Instance().getUserPropertiesDir();
        g_log.debug() << "Temporary file written to " << path << std::endl;
      }
      if ( ( *(path.rbegin()) != '/' ) && ( *(path.rbegin()) != '\\' ) )
      {
        path.push_back('/');
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

    /**   Removes the temporary file
    */
    void ManagedRawFileWorkspace2D::removeTempFile() const
    {
      if (!m_tempfile.empty()) Poco::File(m_tempfile).remove();
    }
	

  } // namespace DataHandling
} //NamespaceMantid

