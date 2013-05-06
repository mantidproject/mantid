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
        for l in lines:
          if l.count("_")!=0:
            self.wikilinks.append(l.replace("_","").strip())      
        
    def linkAlgorithms(self):
        for item in self.results:
          #check for |
          if item.count("|")==1:
              link,name=item.split("|")
          else:
              name=item
              link=item    
          if (link[0].upper()+link[1:]).split("#")[0] in mantid.AlgorithmFactory.getRegisteredAlgorithms(True).keys():
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
          link=link.strip()  
          if (link[0].upper()+link[1:]).split("#")[0] in mantid.FunctionFactory.getFunctionNames():
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
          #split on #
          linkPieces=link.split("#")
          for i in range(len(linkPieces)):
              linkPieces[i]=linkPieces[i].lstrip().rstrip().replace(' ','_')
          linkPieces[0]=linkPieces[0][0].upper()+ linkPieces[0][1:] 
          link="#".join(linkPieces)
          if linkPieces[0] in self.wikilinks:
              formatted="<a href=\""+WEB_BASE+link+"\">"+name+"</a>"
              self.text=self.text.replace("[["+item+"]]",formatted)

    def linkExplicit(self):
        for item in self.explicitLinks:
          if item.strip()[:4]=='http':
              target=item.strip().split(' ')
              if len(target)>1:
                label=" ".join(target[1:])
                link=target[0]
              else:
                label=item
                link=item
              formatted="<a href=\""+link+"\">"+label+"</a>"
              self.text=self.text.replace("["+item+"]",formatted)
              
    def linkCategories(self):
        for item in self.results:
              #check for |
          if item.count("|")==1:
              link,name=item.split("|")
          else:
              name=item
              link=item   
          if link==":Category:Fit_functions":
              formatted="<a href=\"fitfunctions_index.html\">"+name+"</a>"  
              self.text=self.text.replace("[["+item+"]]",formatted) 
          elif link==":Category:Algorithms":
              formatted="<a href=\"algorithms_index.html\">"+name+"</a>"  
              self.text=self.text.replace("[["+item+"]]",formatted)   
          elif link.count("ategory:")==1:
              linkpiece=link.split("ategory:")[1].strip(":")
              formatted="<a href=\"AlgoCat_"+linkpiece+".html\">"+name+"</a>"  
              self.text=self.text.replace("[["+item+"]]",formatted) 
                
    def linkMedia(self):
        for item in self.results:
          #check for |
          if item.count("|")==1:
              link,name=item.split("|")
          else:
              name=item
              link=item  
          if link.find('Media:')!=-1:
              formatted= "<a href=\""+WEB_BASE+link.replace("Media:","File:").strip()+"\">"+name+"</a>"   
              self.text=self.text.replace("[["+item+"]]",formatted)                                   
      
    def parse(self):
        self.linkAlgorithms()
        self.linkFitFunctions()
        self.linkMantidWiki()
        self.linkExplicit()
        self.linkCategories()
        self.linkMedia()
        return self.text
