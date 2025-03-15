#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QDir>
#include <QFile>
#include <QDebug>
#include "localbackend.h"
#include "cal.h"
#include "calendaritem.h"
#include <KCalendarCore/ICalFormat>
#include <KCalendarCore/MemoryCalendar>

class TestLocalBackend : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testLoadCalendars();
    void testLoadItems();

private:
    QTemporaryDir tempDir;
    LocalBackend *backend;
    QString collectionId;
};

void TestLocalBackend::initTestCase()
{
    QVERIFY(tempDir.isValid());
    QString rootPath = tempDir.path();

    // Create a calendar directory
    QDir rootDir(rootPath);
    QVERIFY(rootDir.mkdir("Test Calendar"));

    // Create a sample .ics file for an Event
    QString icsData = QString(
        "BEGIN:VCALENDAR\n"
        "VERSION:2.0\n"
        "PRODID:-//Test//TimeBuster//EN\n"
        "BEGIN:VEVENT\n"
        "UID:12345\n"
        "SUMMARY:Test Event\n"
        "DTSTART:20250315T100000Z\n"
        "DTEND:20250315T110000Z\n"
        "END:VEVENT\n"
        "END:VCALENDAR\n"
    );

    QFile eventFile(rootPath + "/Test Calendar/12345.ics");
    QVERIFY(eventFile.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out(&eventFile);
    out << icsData;
    eventFile.close();

    // Create a sample .ics file for a Todo
    icsData = QString(
        "BEGIN:VCALENDAR\n"
        "VERSION:2.0\n"
        "PRODID:-//Test//TimeBuster//EN\n"
        "BEGIN:VTODO\n"
        "UID:67890\n"
        "SUMMARY:Test Todo\n"
        "DUE:20250315T120000Z\n"
        "END:VTODO\n"
        "END:VCALENDAR\n"
    );

    QFile todoFile(rootPath + "/Test Calendar/67890.ics");
    QVERIFY(todoFile.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream out2(&todoFile);
    out2 << icsData;
    todoFile.close();

    backend = new LocalBackend(rootPath, this);
    collectionId = "col0";
}

void TestLocalBackend::testLoadCalendars()
{
    QList<CalendarMetadata> calendars = backend->loadCalendars(collectionId);
    QCOMPARE(calendars.size(), 1);
    QCOMPARE(calendars[0].id, "col0_test_calendar");
    QCOMPARE(calendars[0].name, "Test Calendar");
}

void TestLocalBackend::testLoadItems()
{
    // First, load calendars to populate internal state
    QList<CalendarMetadata> calendars = backend->loadCalendars(collectionId);
    QVERIFY(!calendars.isEmpty());

    // Create a Cal object to pass to loadItems
    Cal *cal = new Cal("col0_test_calendar", "Test Calendar", nullptr);
    QList<QSharedPointer<CalendarItem>> items;

    // Connect to itemsLoaded signal to capture the result
    bool signalReceived = false;
    connect(backend, &LocalBackend::itemsLoaded, this,
            [&items, &signalReceived](Cal *c, const QList<QSharedPointer<CalendarItem>> &loadedItems) {
                Q_UNUSED(c);
                items = loadedItems;
                signalReceived = true;
            });

    backend->loadItems(cal);
    QTRY_VERIFY_WITH_TIMEOUT(signalReceived, 5000);

    QCOMPARE(items.size(), 2);

    // Verify the Event
    QSharedPointer<CalendarItem> eventItem = items[0];
    QCOMPARE(eventItem->type(), QString("Event"));
    QCOMPARE(eventItem->id(), QString("col0_test_calendar_12345"));
    QCOMPARE(eventItem->incidence()->summary(), QString("Test Event"));

    // Verify the Todo
    QSharedPointer<CalendarItem> todoItem = items[1];
    QCOMPARE(todoItem->type(), QString("Todo"));
    QCOMPARE(todoItem->id(), QString("col0_test_calendar_67890"));
    QCOMPARE(todoItem->incidence()->summary(), QString("Test Todo"));
}

QTEST_MAIN(TestLocalBackend)
#include "test_localbackend.moc"
