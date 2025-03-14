#ifndef CALENDARITEM_H
#define CALENDARITEM_H

#include <QObject>
#include <QSharedPointer>
#include <KCalendarCore/Incidence>
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

    QString id() const { return m_id; }
    KCalendarCore::Incidence::Ptr incidence() const { return m_incidence; }
    void setIncidence(const KCalendarCore::Incidence::Ptr &incidence);

    QDateTime lastModified() const { return m_lastModified; }
    void setLastModified(const QDateTime &lastModified) { m_lastModified = lastModified; }
    QString etag() const { return m_etag; }
    void setEtag(const QString &etag) { m_etag = etag; }

    virtual QString type() const = 0;
    virtual QVariant data(int role) const = 0;
    virtual CalendarItem* clone(QObject *parent = nullptr) const = 0;

protected:
    QString m_id;
    KCalendarCore::Incidence::Ptr m_incidence;
    QDateTime m_lastModified;
    QString m_etag;
};

class Event : public CalendarItem
{
    Q_OBJECT
public:
    explicit Event(const QString &calId, const QString &itemUid, QObject *parent = nullptr);
    QString type() const override { return "Event"; }
    QVariant data(int role) const override;
    CalendarItem* clone(QObject *parent = nullptr) const override;
};

class Todo : public CalendarItem
{
    Q_OBJECT
public:
    explicit Todo(const QString &calId, const QString &itemUid, QObject *parent = nullptr);
    QString type() const override { return "Todo"; }
    QVariant data(int role) const override;
    CalendarItem* clone(QObject *parent = nullptr) const override;
};

#endif // CALENDARITEM_H
