This directory stores references to the Test data. The real data is stored on a remote file server and CMake's [ExternalData](http://www.cmake.org/cmake/help/v3.0/module/Extern

### Directories

The data is separated into directories depending on the use case:

* DocTest - files for usage examples in the Sphinx documentation. This is zipped up for users to download;
* UnitTest - files for unit tests. Ideally, this should only be used for data loading code.
