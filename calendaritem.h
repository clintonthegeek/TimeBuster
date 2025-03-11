#ifndef CALENDARITEM_H
#define CALENDARITEM_H

#include <QObject>
#include <KCalendarCore/Incidence>

class CalendarItem : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString id READ id CONSTANT)
    Q_PROPERTY(KCalendarCore::Incidence::Ptr incidence READ incidence WRITE setIncidence)

public:
    explicit CalendarItem(const QString &id, QObject *parent = nullptr);
    virtual ~CalendarItem() = default;

    QString id() const { return m_id; }
    KCalendarCore::Incidence::Ptr incidence() const { return m_incidence; }
    void setIncidence(const KCalendarCore::Incidence::Ptr &incidence);

    virtual QString type() const = 0;
    virtual QVariant data(int role) const = 0;

protected:
    QString m_id;
    KCalendarCore::Incidence::Ptr m_incidence;
};

class Event : public CalendarItem
{
    Q_OBJECT
public:
    explicit Event(const QString &id, QObject *parent = nullptr);
    QString type() const override { return "Event"; }
    QVariant data(int role) const override;
};

class Todo : public CalendarItem
{
    Q_OBJECT
public:
    explicit Todo(const QString &id, QObject *parent = nullptr);
    QString type() const override { return "Todo"; }
    QVariant data(int role) const override;
};

#endif // CALENDARITEM_H
