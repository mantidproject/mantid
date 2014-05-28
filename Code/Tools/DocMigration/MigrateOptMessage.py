import argparse
import fnmatch
import os
import re


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-d","--dry", help="dry run, does not change files",action="store_true")
    parser.add_argument("codedir", help="The directory to start searching for algorithms", type=str)
    args = parser.parse_args()


    cppFiles = []
    for root, dirnames, filenames in os.walk(args.codedir):
      for filename in fnmatch.filter(filenames, '*.cpp'):
          cppFiles.append(os.path.join(root, filename))

    for cppFile in cppFiles:
        cppdir = os.path.dirname(cppFile)
        (cppname,cppext) = os.path.splitext(os.path.basename(cppFile))
        print cppname,"\t",

        #get .h file
        moduledir = os.path.dirname(cppdir)
        incdir = os.path.join(moduledir,"inc")
        #this should contain only one directory
        hdir = ""
        for x in os.listdir(incdir):
            if os.path.isfile(x):
                pass
            else: 
                hdir = os.path.join(incdir,x)
        hFile = os.path.join(hdir,cppname+".h")
        if not os.path.isfile(hFile):
            print "HEADER NOT FOUND"
            #next file
            break

        #read cppFile
        cppText=  ""
        with open (cppFile, "r") as cppfileHandle:
            cppText=cppfileHandle.read()

        #read hFile
        hText=  ""
        with open (hFile, "r") as hfileHandle:
            hText=hfileHandle.read()

        summary = readOptionalMessage(cppText)
        summary = striplinks(summary)

        if summary != "":
            hText=insertSummaryCommand(hText,summary)
            hText=removeHeaderInitDocs(hText)
            if hText != "":
                cppText=removeOptionalMessage(cppText)
                if not args.dry:
                    with open(hFile, "w") as outHFile:
                        outHFile.write(hText)
                    with open(cppFile, "w") as outCppFile:
                        outCppFile.write(cppText)

                print
            else: 
                print "Could not find h instertion position"
        else:
           print "Could not find summary"

def striplinks(text):
    retVal = text.replace("[[","")
    retVal = retVal.replace("]]","")
    return retVal

def readOptionalMessage(cppText):
    retVal = ""
    match = re.search(r'^.*setOptionalMessage\s*\(\s*"(.+)"\s*\)\s*;\.*$',cppText,re.MULTILINE)
    if match:
        retVal = match.group(1)
    else:
        wikiMatch = re.search(r'^.*setWikiSummary\s*\(\s*"(.+)"\s*\)\s*;\.*$',cppText,re.MULTILINE)
        if wikiMatch:
            retVal = wikiMatch.group(1)
    return retVal

def removeOptionalMessage(cppText):
    retVal = regexReplace(r'^.*setOptionalMessage\s*\(\s*"(.+)"\s*\)\s*;\.*$','',cppText,re.MULTILINE)
    retVal = regexReplace(r'^.*setWikiSummary\s*\(\s*"(.+)"\s*\)\s*;\.*$','',retVal,re.MULTILINE)
    retVal = regexReplace(r'[\w\s/]*::initDocs.*?\}','',retVal,re.DOTALL)
    return retVal

def removeHeaderInitDocs(hText):
    retVal = regexReplace(r'//[\w\s/]*initDocs.*?$','',hText,re.MULTILINE+re.DOTALL)
    return retVal

def insertSummaryCommand(hText,summary):
    retVal = ""
    newLine = '\n    ///Summary of algorithms purpose\n    virtual const std::string summary() const {return "' + summary + '";}\n'
    match = re.search(r'^.*const.*string\s+name\(\)\s+const.*$',hText,re.MULTILINE)
    if match:
        endpos = match.end(0)
        #insert new line
        retVal = hText[:endpos] + newLine + hText[endpos:]
    else:
        print "DID NOT MATCH!!!"
    return retVal

def regexReplace(regex,replaceString,inputString,regexOpts):
    retVal = inputString
    match = re.search(regex,inputString,regexOpts)
    if match:
        retVal = inputString[:match.start(0)] + replaceString + inputString[match.end(0):]
    return retVal




main()