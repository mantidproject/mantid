import mantid,re    
from assistant_common import WEB_BASE

class fixLinks:
    def __init__(self,text):
        self.text=text
        self.results = re.findall(r'\[\[(.*?)\]\]', text)
        #TODO: read link list from a file
        self.wikilinks=['Workspace','MatrixWorkspace','instrument','MDEventWorkspace','Units','Algorithm','InstrumentDefinitionFile']
    def linkAlgorithms(self):
        for item in self.results:
          #check for |
          if item.count("|")==1:
              link,name=item.split("|")
          else:
              name=item
              link=item
          if link in mantid.AlgorithmFactory.getRegisteredAlgorithms(True).keys():
            formatted="<a href=\"Algo_"+link+".html\">"+name+"</a>"
            self.text=self.text.replace("[["+item+"]]",formatted)

    def linkFitFunctions(self):
        for item in self.results:
          #check for |
          if item.count("|")==1:
              link,name=item.split("|")
          else:
              name=item
              link=item
          if link in mantid.FunctionFactory.getFunctionNames():
            formatted="<a href=\"FitFunc_"+link+".html\">"+name+"</a>"
            self.text=self.text.replace("[["+item+"]]",formatted)

    def linkMantidWiki(self):
        for item in self.results:
          #check for |
          if item.count("|")==1:
              link,name=item.split("|")
          else:
              name=item
              link=item
          if link in self.wikilinks:
              formatted="<a href=\""+WEB_BASE+link+"\">"+name+"</a>"
              self.text=self.text.replace("[["+item+"]]",formatted)

    def linkImages(self):
        pass

    def parse(self):
        self.linkAlgorithms()
        self.linkFitFunctions()
        self.linkImages()
        self.linkMantidWiki()
        return self.text

