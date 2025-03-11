#ifndef CALENDARITEM_H
#define CALENDARITEM_H

#include <QObject>
#include <QString>

class CalendarItem : public QObject
{
    Q_OBJECT

public:
    explicit CalendarItem(const QString &uid, QObject *parent = nullptr);
    virtual ~CalendarItem() = default;

    QString uid() const { return m_uid; }
    virtual QString summary() const = 0; // Pure virtual - subclasses define this

protected:
    QString m_uid; // Unique ID, backend-specific for now
};

class Event : public CalendarItem
{
    Q_OBJECT

public:
    explicit Event(const QString &uid, QObject *parent = nullptr);
    QString summary() const override { return m_summary; }
    void setSummary(const QString &summary) { m_summary = summary; }

private:
    QString m_summary; // Stubbed - we'll add more properties later
};

class Todo : public CalendarItem
{
    Q_OBJECT

public:
    explicit Todo(const QString &uid, QObject *parent = nullptr);
    QString summary() const override { return m_summary; }
    void setSummary(const QString &summary) { m_summary = summary; }

private:
    QString m_summary; // Stubbed - mirroring Event for now
};

#endif // CALENDARITEM_H
