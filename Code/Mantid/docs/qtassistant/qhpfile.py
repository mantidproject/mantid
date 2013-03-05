#!/usr/bin/env python
from xml.dom.minidom import Document
from assistant_common import addEle, addTxtEle

class QHPFile:
    def __init__(self, namespace, folder="doc"):
        self.__doc = Document()
        self.__root = addEle(self.__doc, self.__doc,"QtHelpProject",
                             {"version":"1.0"})

        addTxtEle(self.__doc, self.__root, "namespace", namespace)
        addTxtEle(self.__doc, self.__root, "virtualFolder", folder)


        self.__filterSect =  addEle(self.__doc, self.__root, "filterSection")

        self.__keywordsEle = None # should have 'keywords' section
        self.__keywords = []
        self.__filesEle = None # should have 'files' section
        self.__files = []

    def addTOC(self, contents):
        """
        The contents must all be specified at once. They should be in the
        form of [(title1, ref1), (title2, ref2), ...].
        """
        toc = le.SubElement(self.__filterSect, "toc")
        for (title, ref) in contents:
            le.SubElement(toc, "section",
                          **{"title":title, "ref":ref})

    def addFile(self, filename, keyword=None):
        if not keyword is None:
            if self.__keywordsEle is None:
                self.__keywordsEle = addEle(self.__doc, self.__filterSect,
                                            "keywords")
            if not keyword in self.__keywords:
                addEle(self.__doc, self.__keywordsEle, "keyword",
                       {"name":keyword, "ref":filename})
                self.__keywords.append(keyword)
        if self.__filesEle is None:
            self.__filesEle = addEle(self.__doc, self.__filterSect, "files")
        if not filename in self.__files:
            addTxtEle(self.__doc, self.__filesEle, "file", filename)
            self.__files.append(filename)

    def __str__(self):
        return self.__doc.toprettyxml(indent="  ")

    def write(self, filename):
        """
        Write the help configuration to the given filename.
        """
        handle = open(filename, 'w')
        handle.write(str(self))
