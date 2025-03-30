#ifndef VIEWINTERFACE_H
#define VIEWINTERFACE_H

#include <QWidget>
#include "cal.h"
#include "collection.h"
#include "calendaritem.h"

class ViewInterface : public QWidget {
    Q_OBJECT
public:
    explicit ViewInterface(Collection* collection, QWidget* parent = nullptr);
    virtual ~ViewInterface() override = default;

    virtual void setActiveCal(Cal* cal) = 0;
    virtual void refresh() = 0;
    virtual void setCollection(Collection* collection); // New method

    Collection* collection() const { return m_collection; }
    Cal* activeCal() const { return m_activeCal; }

signals:
    void calChanged(Cal* cal);
    void itemSelected(QList<QSharedPointer<CalendarItem>> items);
    void itemModified(QList<QSharedPointer<CalendarItem>> items);

protected:
    Collection* m_collection;
    Cal* m_activeCal;
};

#endif // VIEWINTERFACE_H
