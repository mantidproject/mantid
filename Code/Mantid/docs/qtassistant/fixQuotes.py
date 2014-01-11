import re

def fixQuotes(text):
  text=fixBoldItalic(text)
  text=fixBold(text)
  text=fixItalic(text)
  return text
  
def fixBoldItalic(text):
  results=re.findall(r"\'\'\'\'\'(.+?)\'\'\'\'\'",text)
  for item in results:
    text = text.replace("'''''"+item+"'''''", "<b><i>"+item+"</i></b>")
  return text
  
def fixBold(text):
  results=re.findall(r"\'\'\'(.+?)\'\'\'",text)
  for item in results:
    text = text.replace("'''"+item+"'''", "<b>"+item+"</b>")
  return text
  
def fixItalic(text):
  results=re.findall(r"\'\'(.+?)\'\'",text)
  for item in results:
    text = text.replace("''"+item+"''", "<i>"+item+"</i>")
  return text
