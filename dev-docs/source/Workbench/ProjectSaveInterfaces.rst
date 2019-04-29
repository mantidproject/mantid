.. _ProjectSaveInterfaces:

============
Project Save
============

Overview
########

Project save is the attempt to allow all parts of a user's project to be saved, namely interfaces (C++ and Python), plots and  workspaces. 

Handy links
###########

- Python JSON Library - https://docs.python.org/2/library/json.html
- QMap - http://doc.qt.io/qt-5/qmap.html
- QVariant - http://doc.qt.io/qt-5/QVariant.html 
- SIP (documentation on SIP is relatively limited) - http://pyqt.sourceforge.net/Docs/sip4/using.html

Implementation
##############

Project save will save interfaces, plots and workspaces, this is achieved by using either dedicated classes, in the case of plots and workspaces, or by adding a encoder and decoder to the EncoderFactory and the DecoderFactory. The way in which the saving is currently achieved is by returning a dictionary containing only either primitive types (by far the most common approach) or by returning current the python JSON library's serializeable types. Examples of how types are converted to JSON and back can be found at https://docs.python.org/2/library/json.html under section 18.2.2 Encoders and Decoders. 

Saving and loading an interface (Python)
########################################
A good example of the way to save and load an interface in python is from the TableWorkspaceDisplay, as it is quite simple yet effective. It covers, saving custom objects, dictionary conventions, and object recreation in the decoder. This example is available in production in the mantidqt/widgets/tableworkspacedisplay/io.py file.

The way the Encoder and Decoder factory works are different, the EncoderFactory will grab the class name of the object passed to it and check if any of it's known Encoders have a "tag" with that name in it. The DecoderFactory will check against a "tag" which is saved in the dictionary but is dynamically grabbed from the tags list of the Encoder that encoded the interface, at the time of saving and a developer will not need to save this tag.

There is a slightly more advanced version of registering an Encoder with the EncoderFactory (This isn't required by DecoderFactory due to the implementation). Alongside adding a tag equal to the class name of the Encoded variable, you can pass a function that will return True if the encoder can encode the passed object. A good example for these is the MatrixWorkspaceDisplay and TableWorkspaceDisplay as they have a containing object which is the same. This allows seperate encoders and decoders for technically the same object, with different refferences.

The basic template for an io file:

.. code-block:: python

  class InterfaceAttributes(object):
      # WARNING: If you delete a tag from here instead of adding a new one, it will make old project files obsolete so
      # just add an extra tag to the list e.g. ["InstrumentWidget", "IWidget"]
      # This list must contain the name of the class that will be found at the top level of Widgets, this is usually the view 
      # class
      tags = ["Interface"]

  class InterfaceEncoder(InterfaceAttributes):
      def __init__(self):
          super(InterfaceEncoder, self).__init__()
      
      def encode(self, obj, project_path=None):
          presenter = obj.presenter
          return {"info": presenter.getInfo(), "state": presenter.getState()}
      
      @classmethod
      def has_tag(cls, tag):
          return tag in cls.tags

  class InterfaceDecoder(InterfaceAttributes):
      def __init__(self):
          super(InterfaceDecoder, self).__init__()

      def decoder(self, obj_dict, project_path=None):
          # Recreate the GUI in a base state
          presenter = InterfacePresenter()

          # Restore the state from the dictionary
          presenter.restoreInfo(obj_dict["info"])
          presenter.restoreState(obj_dict["state"])

          # Return the view of the GUI or whatever object can have .show() called on it
          return presenter.view
      
      @classmethod
      def has_tag(cls, tag):
          return tag in cls.tags

Alongside the io file you will need to register the InterfaceEncoder and InterfaceDecoder with the relevant factories this should be done in the __init__.py file of the module containing the overall Interface. This can be done by getting the Encoder and Decoder factory objects and running register_encoder and register_decoder respectively on the factories and passing the class of the Encoder and Decoder. An example of this can be found in mantidqt/widgets/tableworkspacedisplay/__init__.py. Continuing with our previous example it would look like this:

.. code-block:: python

  from mantidqt.project.encoderfactory import EncoderFactory
  from mantidqt.project.decoderfactory import DecoderFactory
  from mantidqt.widget.interface.io import InterfaceEncoder, InterfaceDecoder
  
  EncoderFactory.register_encoder(InterfaceEncoder)
  DecoderFactory.register_decoder(InterfaceDecoder)

Now that those two tasks are complete your Interface will be saved and loaded alongside a normal project save operation.

Saving and loading an interface (C++)
#####################################

When an interface is written in C++ it comes with some challenges on top of the python interface challenges that have already been discussed, luckily the plan is to no longer create any GUIs in C++ so this should become less of a problem as time goes on, a good example of how this can be achieved is by looking at the InstrumentView implementation on workbench in the mantidqt/widgets/instrumentview python package.

To access the data from python that is stored in C++ there are two options, collect all the data together and pass one big chunk across the language gap, or gather the data separately by exposing all methods for getting and setting to python. The aim is to get all the data back to python so it can be saved alongside all other information.

The way that is easiest would be creating a C++ class and grab all the state and information needed to recreate the interface to the same state it was in. The formation of the information should be a QMap<QString, QVariant>, this is because it will quickly and natively convert this to a python dict object when using SIP, with the only caveat being that before returning this dictionary from the encode method it is encouraged to check if any QtObjects have been transferred across, for example QtColors will transfer seamlessly across, but cannot be serialized by JSON, so this would need to be converted into a JSON serializable form and then back to QtColors before transferring to C++ or at least handled on the C++ side to create the objects back.

Before the tutorial really starts some advice about QMap, if using the [] operator and it cannot find the correct value, it will return a default constructed version of the value in the key value pair, i.e. if it can't find the QVariant it will return an empty one, which in turn will also return a default value when converted back into a normal type, i.e. QVariant.toBool() will return false, in the given cases, always.

To start with the encoding we would have a C++ called Interface with class Interface, we would create a class called InterfaceEncoder and have the method QMap<QString, QVariant> encode(Interface &interface), this method does not need to follow any specific format but it would be good practice to follow the encode and decode naming scheme. As an example:

Header File:

.. code-block:: cpp

    class EXPORT_OPT_MANTIDQT_INTERFACE InterfaceEncoder {
    public:
    InterfaceEncoder();
    QMap<QString, QVariant> encode (const Interface &interface);

    private:
    QMap<QString, QVariant> encodeInfo(const InterfaceInfo &info);
    QMap<QString, QVariant> encodeState(const InterfaceState &state);
    };

Source File:

.. code-block:: cpp

    InterfaceEncoder::InterfaceEncoder()

    QMap<QString, QVariant> InterfaceEncoder::encode(const Interface &interface){
      QMap<QString, QVariant> map;
      // It is encouraged to not add extra methods to the Interface class for getting information unless already present
      // Instead add the encoder as a friend class and access the member variables directly
      map.insert(QString("info"), QVariant(encodeInfo(interface.m_interfaceInfo));
      map.insert(QString("state"), QVariant(encodeState(interface.getStateObject()));
      return map;
    }

    QMap<QString, QVariant> InterfaceEncoder::encodeInfo(const InterfaceInfo &info){
      QMap<QString, QVariant> map;
      map.insert(QString("info1"), QVariant(info.m_info1));
      map.insert(QString("info2"), QVariant(info.m_info2));
      return map;
    }

    QMap<QString, QVariant> InterfaceEncoder::encodeState(const InterfaceState &state){
      QMap<QString, QVariant> map;
      map.insert(QString("state1"), QVariant(state.m_state1));
      map.insert(QString("state2"), QVariant(state.m_state2));
      return map;
    }

With the encoder classes done it needs to be exposed to python via SIP, this can be done by adding the InterfaceEncoder to a compiling sip file, now the placement of this is not necessarily mandated, but InstrumentView had it's own SIP file and it made sense to expand it to encompass it's encoder and decoders.

SIP File:

.. code-block:: text

    class InterfaceEncoder {
    %TypeHeaderCode
    #include "MantidQtWidgets/Interface/InterfaceEncoder.h"
    %End
    public:
      InterfaceEncoder();
      QMap<QString, QVariant> encode(const Interface &interface) /ReleaseGIL/;
    };
    class InterfaceDecoder : QObject{
    %TypeHeaderCode
    #include "MantidQtWidgets/Interface/InterfaceDecoder.h"
    %End
    public:
      InterfaceDecoder();
      void decode(const QMap<QString, QVariant> &map) /ReleaseGIL/;
    };

The last thing to discuss is that the decoder would be structured very similarly to the encoder, but instead of constructing a map you are just setting the details back from the map. This is achieved by using ``map[QString("key")].toInt()`` for a int, as the value stored is a QVariant so a conversion is needed.
