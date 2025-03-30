#include "viewinterface.h"
#include <QDebug>

ViewInterface::ViewInterface(Collection* collection, QWidget* parent)
    : QWidget(parent), m_collection(collection), m_activeCal(nullptr)
{
    qDebug() << "ViewInterface: Created for collection" << (collection ? collection->id() : "null");
}
