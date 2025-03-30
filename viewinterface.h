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

    // Pure virtual methods for all views
    virtual void setActiveCal(Cal* cal) = 0; // Set or react to active calendar
    virtual void refresh() = 0;              // Update display based on model changes

    Collection* collection() const { return m_collection; }
    Cal* activeCal() const { return m_activeCal; }

signals:
    // Proactive signals
    void calChanged(Cal* cal);                         // Emitted when focus shifts
    void itemSelected(QList<QSharedPointer<CalendarItem>> items); // Selection changed

    // Shared signal for edits
    void itemModified(QList<QSharedPointer<CalendarItem>> items); // Edit applied

protected:
    Collection* m_collection; // Not owned, set by MainWindow or creator
    Cal* m_activeCal;         // Current calendar context, may be null
};

#endif // VIEWINTERFACE_H
