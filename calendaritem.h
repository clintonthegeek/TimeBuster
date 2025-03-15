#ifndef CALENDARITEM_H
#define CALENDARITEM_H

#include <QObject>
#include <QSharedPointer>
#include <KCalendarCore/Incidence>
#include <KCalendarCore/ICalFormat>
#include <KCalendarCore/MemoryCalendar>
#include <KCalendarCore/Event>  // Added for Event definition
#include <KCalendarCore/Todo>   // Added for Todo definition

class CalendarItem : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString id READ id CONSTANT)
    Q_PROPERTY(KCalendarCore::Incidence::Ptr incidence READ incidence WRITE setIncidence)

public:
    explicit CalendarItem(const QString &calId, const QString &itemUid, QObject *parent = nullptr);
    virtual ~CalendarItem() = default;

    virtual QString id() const { return m_itemId; }
    virtual QString toICal() const = 0;
    virtual CalendarItem* clone(QObject *parent = nullptr) const = 0; // Single declaration with default parameter

    QString calId() const { return m_calId; }
    QDateTime lastModified() const { return m_lastModified; }
    void setLastModified(const QDateTime &lastModified) { m_lastModified = lastModified; }
    QString etag() const { return m_etag; }
    void setEtag(const QString &etag) { m_etag = etag; }

    bool isDirty() const { return m_dirty; }
    void setDirty(bool dirty) { m_dirty = dirty; }

    KCalendarCore::Incidence::Ptr incidence() const { return m_incidence; }
    void setIncidence(const KCalendarCore::Incidence::Ptr &incidence);

    virtual QString type() const = 0;
    virtual QVariant data(int role) const = 0;

protected:
    QString m_calId;
    QString m_itemId;
    KCalendarCore::Incidence::Ptr m_incidence;
    QDateTime m_lastModified;
    QString m_etag;
    bool m_dirty = false; // New member to track dirty state
};

class Event : public CalendarItem
{
    Q_OBJECT
public:
    explicit Event(const QString &calId, const QString &itemUid, QObject *parent = nullptr);
    QString type() const override;
    QVariant data(int role) const override;
    QString toICal() const override;
    CalendarItem* clone(QObject *parent = nullptr) const override;
};

class Todo : public CalendarItem
{
    Q_OBJECT
public:
    explicit Todo(const QString &calId, const QString &itemUid, QObject *parent = nullptr);
    QString type() const override;
    QVariant data(int role) const override;
    QString toICal() const override;
    CalendarItem* clone(QObject *parent = nullptr) const override;
};

#endif // CALENDARITEM_H
