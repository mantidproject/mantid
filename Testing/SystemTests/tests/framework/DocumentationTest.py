# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from abc import ABCMeta, abstractmethod
from pathlib import Path
from typing import List

from systemtesting import MantidSystemTest

PATH_TO_DOCS = (Path(__file__).resolve() / "../../../../../docs/source").resolve()


class DocumentationTestBase(MantidSystemTest, metaclass=ABCMeta):
    def __init__(self):
        super(DocumentationTestBase, self).__init__()
        self._known_no_docs: List[str] = []
        self._docs_path: Path = Path("")
        self._files: List[List[str]] = []
        self._test_type: str = ""
        self._setup_test()

    @abstractmethod
    def _setup_test(self):
        raise NotImplementedError("This method needs implementing in a subclass of this class.")

    def file_exists_under_docs_path(self, file_name: str) -> bool:
        """
        Search for file name in the given directory and all its subdirectories
        :param file_name: name of the file
        :return: bool, true if file found
        """
        found_files = [path for path in self._docs_path.rglob(file_name) if path.is_file]
        return len(found_files) > 0

    def runTest(self):
        self.assertGreaterThan(len(self._files), 0, msg=f"No {self._test_type} interfaces found")
        missing = []

        for file_name_options in self._files:
            if not any(self.file_exists_under_docs_path(file_name) for file_name in file_name_options):
                if all(file_name not in self._known_no_docs for file_name in file_name_options):
                    missing.append(file_name_options[0])
            elif any(file_name in self._known_no_docs for file_name in file_name_options):
                raise FileExistsError(
                    f"{' or '.join(file_name_options)} exists but is still in the exceptions list. Please update the list."
                )

        self.assertEqual(len(missing), 0, msg=f"Missing the following {self._test_type} docs:\n" + "\n".join(missing))
