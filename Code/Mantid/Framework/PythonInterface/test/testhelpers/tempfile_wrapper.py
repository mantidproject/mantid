from tempfile import NamedTemporaryFile
import os


class TemporaryFileHelper(object):
    """Helper class for temporary files in unit tests

    This class is a small helper for using temporary files for unit test. On instantiation, a temporary file will be
    created (using NamedTemporaryFile from the tempfile module). If the string argument to the constructor is not empty,
    its content will be written to that file. The getName()-method provides the name of the temporary file, which can
    for example be passed to an algorithm that expects a FileProperty. On destruction of the TemporaryFileHelper object,
    the temporary file is removed automatically using os.unlink().

    Usage:
      emptyFileHelper = TemporaryFileHelper()
      fh = open(emptyFileHelper.getName(), 'r+')
      fh.write("Something or other\n")
      fh.close()

      filledFileHelper = TemporaryFileHelper("Something or other\n")
      other = open(filledFileHelper.getName(), 'r')
      for line in other:
        print line
      other.close()

      del emptyFileHelper
      del filledFileHelper
    """
    tempFile = None

    def __init__(self, fileContent=""):
        self.tempFile = NamedTemporaryFile('r+', delete=False)

        if fileContent:
            self._setFileContent(fileContent)

    def __del__(self):
        os.unlink(self.tempFile.name)

    def getName(self):
        return self.tempFile.name

    def _setFileContent(self, content):
        fileHandle = open(self.getName(), 'r+')
        fileHandle.write(content)
        fileHandle.close()
