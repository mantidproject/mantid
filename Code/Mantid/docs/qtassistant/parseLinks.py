import mantid,re,sys,os    
from assistant_common import WEB_BASE

class fixLinks:
    def __init__(self,text): 
        self.text=text
        self.results = re.findall(r'\[\[(.*?)\]\]', text)
        self.explicitLinks=re.findall(r'\[(.*?)\]', text)
        wikilinksfile=open(os.path.split(sys.argv[0])[0]+'/WikiLinks.txt')
        lines=wikilinksfile.readlines()
        wikilinksfile.close()
        self.wikilinks=[l.strip() for l in lines]
    def linkAlgorithms(self):
        for item in self.results:
          #check for |
          if item.count("|")==1:
              link,name=item.split("|")
          else:
              name=item
              link=item     
          if (link[0].upper()+link[1:]) in mantid.AlgorithmFactory.getRegisteredAlgorithms(True).keys():
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
          if (link[0].upper()+link[1:]) in mantid.FunctionFactory.getFunctionNames():
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
          if "".join((link[0].upper()+link[1:]).split("#")[0].split()) in self.wikilinks:
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
