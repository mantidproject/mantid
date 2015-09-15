import sys

tracking = """<script>
  (function(i,s,o,g,r,a,m){i['GoogleAnalyticsObject']=r;i[r]=i[r]||function(){
  (i[r].q=i[r].q||[]).push(arguments)},i[r].l=1*new Date();a=s.createElement(o),
  m=s.getElementsByTagName(o)[0];a.async=1;a.src=g;m.parentNode.insertBefore(a,m)
  })(window,document,'script','//www.google-analytics.com/analytics.js','ga');

  ga('create', 'UA-59110517-2', 'auto');
  ga('send', 'pageview');

</script>
"""

inFile = file(sys.argv[1], 'r')
content = inFile.readlines()
inFile.close()

for i in xrange(len(content)):
    if r"</head>" in content[i]:
        content[i] = tracking + content[i]

outFile = file(sys.argv[1][:-3], 'w')
outFile.write(''.join(content))
outFile.close()
