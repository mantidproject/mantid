#ifndef COMPOSITEEDITORFACTORY_H
#define COMPOSITEEDITORFACTORY_H

#include "qtpropertybrowser.h"
#include <QPushButton>

/**
 * Composite factory for a particular property manger type.
 * Client must specify a secondary factory for properties with a particular
 * option set. If this option is set crates editor using the scondary factory.
 * Creates the default editor (with the factory passed to the constructor)
 * if the option isn't set or property doesn't have this option.
 * @param ManagerType :: Manager class to use
 */
template <class ManagerType>
class CompositeEditorFactory : public QtAbstractEditorFactory<ManagerType>
{
  typedef QtAbstractEditorFactory<ManagerType> FactoryBaseType;
public:
  CompositeEditorFactory(QObject *parent, FactoryBaseType *defaultFactory)
    : QtAbstractEditorFactory<ManagerType>(parent), 
    m_defaultFactory(defaultFactory),
    m_secondaryFactory(NULL)
  {
  }

  void setSecondaryFactory( const QString &optionName, FactoryBaseType *factory )
  {
    m_optionName = optionName;
    m_secondaryFactory = factory;
  }

protected:
  void connectPropertyManager(ManagerType *manager)
  {
    (void) manager; // Unused
    // Do nothing
  }

  void disconnectPropertyManager(ManagerType *manager)
  {
    (void) manager; // Unused
    // Do nothing
  }

  QWidget *createEditor(ManagerType *manager, QtProperty *property, QWidget *parent)
  {
    if ( !m_secondaryFactory )
    {
      throw std::logic_error("Secondary editor factory isn't set.");
    }

    if ( property->hasOption(m_optionName) && property->checkOption(m_optionName) )
    {
      return m_secondaryFactory->createEditor( manager, property, parent );
    }

    return m_defaultFactory->createEditor( manager, property, parent );
  }
private:
  FactoryBaseType *m_defaultFactory;
  FactoryBaseType *m_secondaryFactory;
  QString m_optionName;
};

#endif // COMPOSITEEDITORFACTORY_H
