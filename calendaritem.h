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
    QString versionIdentifier() const { return m_etag; }
    void setVersionIdentifier(const QString &id) { m_etag = id; }

    bool isDirty() const { return m_dirty; }
    void setDirty(bool dirty) { m_dirty = dirty; }

    KCalendarCore::Incidence::Ptr incidence() const { return m_incidence; }
    void setIncidence(const KCalendarCore::Incidence::Ptr &incidence);

    virtual QString type() const = 0;
    virtual QVariant data(int role) const = 0;

    // New getters/setters
    virtual QDateTime dtStart() const = 0;
    virtual void setDtStart(const QDateTime &dtStart) = 0;
    virtual QDateTime dtEndOrDue() const = 0; // dtEnd for events, due for todos
    virtual void setDtEndOrDue(const QDateTime &dtEndOrDue) = 0;
    virtual QStringList categories() const = 0;
    virtual void setCategories(const QStringList &categories) = 0;
    virtual QString description() const = 0;
    virtual void setDescription(const QString &description) = 0;
    virtual bool allDay() const = 0;
    virtual void setAllDay(bool allDay) = 0;

    // Define an enum to track conflict status.
    enum class ConflictStatus {
        None,
        Pending,
        Resolved
    };

    // Getter and setter for conflict status.
    ConflictStatus conflictStatus() const { return m_conflictStatus; }
    void setConflictStatus(ConflictStatus status) { m_conflictStatus = status; }

protected:
    ConflictStatus m_conflictStatus = ConflictStatus::None;


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

    QDateTime dtStart() const override;
    void setDtStart(const QDateTime &dtStart) override;
    QDateTime dtEndOrDue() const override;
    void setDtEndOrDue(const QDateTime &dtEndOrDue) override;
    QStringList categories() const override;
    void setCategories(const QStringList &categories) override;
    QString description() const override;
    void setDescription(const QString &description) override;
    bool allDay() const override;
    void setAllDay(bool allDay) override;
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


    QDateTime dtStart() const override;
    void setDtStart(const QDateTime &dtStart) override;
    QDateTime dtEndOrDue() const override;
    void setDtEndOrDue(const QDateTime &dtEndOrDue) override;
    QStringList categories() const override;
    void setCategories(const QStringList &categories) override;
    QString description() const override;
    void setDescription(const QString &description) override;
    bool allDay() const override;
    void setAllDay(bool allDay) override;
};

#endif // CALENDARITEM_H
