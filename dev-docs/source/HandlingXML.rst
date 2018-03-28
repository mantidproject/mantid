Handling XML
============

This page provides an introduction into reading/writing XML effectively
within Mantid. In Mantid, we use Poco to handling XML serialisation. A
useful introductory presentation can be found `here. <http://pocoproject.org/slides/170-XML.pdf>`__

**This page is a work in progress.**

Examples
--------

Parsing
~~~~~~~

.. code-block:: cpp


    #include <Poco/DOM/Document.h>
    #include <Poco/DOM/DOMParser.h>
    #include <Poco/DOM/Element.h>

    Poco::XML::DOMParser pParser;
    Poco::AutoPtr<Poco::XML::Document> pDoc;
    try {
      pDoc = pParser.parseString(cuboidStr);
    } catch (...) {
      // Handle the failure as appropriate
    }
    // Get pointer to root element
    Poco::XML::Element *pRootElem = pDoc->documentElement();

Iterating over an element's children
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

    Poco::XML::Element *pRootElem = pDoc->documentElement();
    //Iterate over every child node (non-recursively)
    for (Node *pNode = pRootElem->firstChild(); pNode != 0; pNode = pNode->nextSibling()) {
      auto pElem = dynamic_cast<Poco::XML::Element*>(pNode);
      if(pElem) {
        //We can now do something with this element safely
      }
    }

Inspecting an element
~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

    Poco::XML::Element *pElem;

    //Reasonably quick operations
    const std::string tag = pElem->tagName(); //for <foo>bar</foo> the tag is 'foo'
    const std::string value = pElem->innerText(); //for the above example: 'bar'
    Poco::XML::Node *pNext = pElem->nextSibling();
    Poco::XML::Node *pPrev = pElem->previousSibling();
    Poco::XML::Node *pChild = pElem->firstChild(); //avoid lastChild, it's expensive

Avoid NodeList
--------------

There are numerous functions that return a ``Poco::XML::NodeList``. You
should avoid using them and the list they return as best you can.

NodeList is a very inefficient container. Its ``item`` method has a cost
equivalent to the value of ``i`` given to it, and its ``length`` method
a cost of ``n``, where ``n`` is the number of nodes in the list.

This means that running the following is horrendously slow:

.. code-block:: cpp

    // NEVER DO THIS
    Poco::AutoPtr<Poco::XML::NodeList> pElems = pElem->getElementsByTagName("foo");
    for(int i = 0; i < pElems->length(); ++i) { // length costs N, and is called N times (N² cost)
      Poco::XML::Node* pNode = pElems->item(i); // item costs at least i and is called N times, with i from 0 -> N-1 (N² + N cost)
    }
    // NEVER DO THIS

Even if the compiler is smart enough to optimise ``pElems->length()`` to
a single call, we still have N² + N performance at best. Instead, we
should **always** use the iteration example given earlier, as that runs
in N time, instead of N².
