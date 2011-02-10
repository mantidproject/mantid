#ifndef BINARYFILE_H_
#define BINARYFILE_H_

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "MantidKernel/System.h"
#include "Poco/File.h"
#include "Poco/Path.h"

namespace Mantid
{
namespace Kernel
{

/// Default number of items to read in from any of the files.
static const size_t DEFAULT_BLOCK_SIZE = 100000; // 100,000

/// Max size block to read from a file (memory limitations)
static const size_t MAX_BLOCK_SIZE = 100000000; // 100 million

/// Min size of a block (too small is inefficient)
static const size_t MIN_BLOCK_SIZE = 1000;

/**
 * The BinaryFile template is a helper function for loading simple binary files.
 *  - The file format must be a simple sequence of objects of type T.
 *  - To load, first create an object of type BinaryFile<T>
 *  - The file provided when opening is checked so that its size is an even multiple
 *    of sizeof(T); an error is thrown otherwise.
 *
 * NOTE: Data saving and loading is little-endian (on Win, Linux, and Intel Mac).
 *       Converting from a byte buffer loaded from disk to
 *       the T type is done with a reinterpret_cast<T>.
 *
 */
template<typename T>
class DLLExport BinaryFile
{
public:
  //------------------------------------------------------------------------------------
  /// Empty constructor
  BinaryFile() : handle(NULL), num_elements(0), offset(0)
  { }

  //------------------------------------------------------------------------------------
  /// Constructor - open a file
  BinaryFile(std::string filename)
  {
    this->open(filename);
  }

  /// Destructor, close the file if needed
  ~BinaryFile()
  {
    this->close();
  }

  //------------------------------------------------------------------------------------
  /** Open a file and keep a handle to the file
   * @param filename :: full path to open
   * @throw runtime_error if the file size is not an even multiple of the type size
   * @throw invalid_argument if the file does not exist
   * */
  void open(std::string filename)
  {
    this->handle = NULL;
    if (!Poco::File(filename).exists())
    {
      std::stringstream msg;
      msg << "BinaryFile::open: File " << filename << " was not found.";
      throw std::invalid_argument("File does not exist.");
    }
    //Open the file
    this->handle = new std::ifstream(filename.c_str(), std::ios::binary);
    //Count the # of elements.
    this->num_elements = this->getFileSize();
    //Make sure we are starting at 0
    this->offset = 0;

  }

  //------------------------------------------------------------------------------------
  /** Close the file
   * */
  void close()
  {
    delete handle;
    handle = NULL;
  }


  //-----------------------------------------------------------------------------
  /** Get the size of a file as a multiple of a particular data type
   * @return the size of the file normalized to the data type
   * @throw runtime_error if the file size is not compatible
   * @throw runtime_error if the handle is not open.
   * */
  size_t getFileSize()
  {
    this->obj_size = sizeof(T);

    if (!handle) {
      throw std::runtime_error("BinaryFile::getFileSize: Cannot find the size of a file from a null handle");
    }

    // get the size of the file in bytes and reset the handle back to the beginning
    handle->seekg(0, std::ios::end);
    size_t filesize = static_cast<size_t>(handle->tellg());
    handle->seekg(0, std::ios::beg);

    // check the file is a compatible size
    if (filesize % obj_size != 0) {
      std::stringstream msg;
      msg << "BinaryFile::getFileSize: File size is not compatible with data size ";
      msg << filesize << "%" << obj_size << "=";
      msg << filesize % obj_size;
      throw std::runtime_error(msg.str());
    }

    return filesize / sizeof(T);
  }

  //-----------------------------------------------------------------------------
  /// Returns the # of elements in the file
  size_t getNumElements()
  {
    return this->num_elements;
  }

  /// Returns the current offset into the file.
  size_t getOffset()
  {
    return this->offset;
  }


  //-----------------------------------------------------------------------------
  /** Get a buffer size for loading blocks of data.
   * @param num_items
::    * @return the buffer size
   */
  size_t getBufferSize(const size_t num_items)
  {
    if (num_items < DEFAULT_BLOCK_SIZE)
      return num_items;
    else
      return DEFAULT_BLOCK_SIZE;
  }

  //-----------------------------------------------------------------------------
  /**
   * Loads the entire contents of the file into a pointer to a std::vector.
   * The file is closed once done.
   * @return file contents in a vector
   */
  std::vector<T> * loadAll()
  {
    if (!handle) {
      throw std::runtime_error("BinaryFile: file is not open.");
    }

    //Initialize the pointer
    std::vector<T> * data = new std::vector<T>;

    //A buffer to load from
    size_t buffer_size = getBufferSize(num_elements);
    T * buffer = new T[buffer_size];

    //Make sure we are at the beginning
    offset = 0;
    handle->seekg(0, std::ios::beg);

    size_t loaded_size;
    while (offset < num_elements)
    {
      //Load that block of data
      loaded_size = loadBlock(buffer, buffer_size);
      // Insert into the data
      data->insert(data->end(), buffer, (buffer + loaded_size));
    }

    //Close the file, since we are done.
    this->close();
    // Free memory
    delete[] buffer;

    //Here's your vector!
    return data;
  }

  //-----------------------------------------------------------------------------
  /**
   * Loads the entire contents of the file into a std::vector.
   * The file is closed once done.
   * @param data :: The contents to load into the file
   */
  void loadAllInto(std::vector<T> &data)
  {
    if (!handle) {
      throw std::runtime_error("BinaryFile: file is not open.");
    }

    //Clear the vector
    data.clear();

    //A buffer to load from
    size_t buffer_size = getBufferSize(num_elements);
    T * buffer = new T[buffer_size];

    //Make sure we are at the beginning
    offset = 0;
    handle->seekg(0, std::ios::beg);

    size_t loaded_size;
    while (offset < num_elements)
    {
      //Load that block of data
      loaded_size = loadBlock(buffer, buffer_size);
      // Insert into the data
      data.insert(data.end(), buffer, (buffer + loaded_size));
    }

    //Close the file, since we are done.
    this->close();
    // Free memory
    delete[] buffer;
  }


  //-----------------------------------------------------------------------------
  /**
   * Loads a single block from file and returns a pointer to a vector containing it.
   *  This can be called repeatedly to load an entire file.
   *
   * @param block_size: how many elements to load in the block. If there are not enough elements,
   *  the vector returned is smaller than block_size
   * @param buffer: array of block_size[] of T; must have been allocated before.
   * @return loaded_size, actually how many elements were loaded.
   */
  size_t loadBlock(T * buffer, size_t block_size)
  {
    if (!handle) {
      throw std::runtime_error("BinaryFile: file is not open.");
    }

    size_t loaded_size;
    //Limit how much is loaded
    loaded_size = block_size;
    if (offset + loaded_size > num_elements)
      loaded_size = num_elements - offset;
    //Read it right into the buffer
    handle->read(reinterpret_cast<char *>(buffer), loaded_size * obj_size);
    offset += loaded_size;
    return loaded_size;
  }


private:
  /// File stream
  std::ifstream * handle;
  /// Size of each object.
  size_t obj_size;
  /// Number of elements of size T in the file
  size_t num_elements;
  /// Offset into the file, if loading in blocks.
  size_t offset;

};


} //Namespace Kernel

} //Namespace Mantid

#endif /* BINARYFILE_H_ */
