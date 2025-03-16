#include "caldavbackend.h"
#include <KDAV/DavUrl>
#include <KDAV/DavItemsListJob>
#include <KDAV/DavItemsFetchJob>
#include <KDAV/EtagCache>
#include <KCalendarCore/ICalFormat>
#include <QUrl>
#include <QDebug>
#include <QRegularExpression>
#include "cal.h"
#include "calendaritem.h"

CalDAVBackend::CalDAVBackend(const QString &serverUrl, const QString &username, const QString &password, QObject *parent)
    : SyncBackend(parent), m_serverUrl(serverUrl), m_username(username), m_password(password)
{
}

CalDAVBackend::~CalDAVBackend()
{
    for (KJob *job : m_activeJobs) {
        job->kill(KJob::Quietly);
    }
    m_activeJobs.clear();
    m_calMap.clear();
    m_itemFetchQueue.clear();
}

QList<CalendarMetadata> CalDAVBackend::loadCalendars(const QString &collectionId)
{
    qDebug() << "CalDAVBackend: Starting loadCalendars for collection" << collectionId;
    QUrl url(m_serverUrl);
    url.setUserName(m_username);
    url.setPassword(m_password);
    qDebug() << "CalDAV URL:" << url.toString(QUrl::RemovePassword);

    KDAV::DavUrl davUrl(url, KDAV::CalDav);
    KDAV::DavCollectionsFetchJob *job = new KDAV::DavCollectionsFetchJob(davUrl);
    job->setProperty("collectionId", collectionId);
    connect(job, &KDAV::DavCollectionsFetchJob::result, this, &CalDAVBackend::onCollectionsLoaded);
    qDebug() << "CalDAVBackend: Job created, starting...";
    m_activeJobs.append(job); // Use append for QList
    job->start();

    qDebug() << "CalDAVBackend: loadCalendars returning empty list (async)";
    return QList<CalendarMetadata>();
}

void CalDAVBackend::storeCalendars(const QString &collectionId, const QList<Cal*> &calendars)
{
    qDebug() << "CalDAVBackend: storeCalendars stub for" << collectionId;
    for (Cal *cal : calendars) {
        m_idToUrl[cal->id()] = m_serverUrl + "/" + cal->id();
    }
}

void CalDAVBackend::storeItems(Cal *cal, const QList<QSharedPointer<CalendarItem>> &items)
{
    qDebug() << "CalDAVBackend: storeItems stub for" << cal->id();
    Q_UNUSED(items);
}

QList<QSharedPointer<CalendarItem>> CalDAVBackend::loadItems(Cal *cal)
{
    QString calId = cal->id();
    qDebug() << "CalDAVBackend: Queuing loadItems for" << calId;
    m_calMap[calId] = cal;
    if (!m_idToUrl.contains(calId)) {
        qDebug() << "CalDAVBackend: Unknown calId" << calId;
        return {};
    }
    if (m_itemFetchQueue.contains(calId)) {
        qDebug() << "CalDAVBackend: Already loading items for" << calId;
        return {};
    }
    m_itemFetchQueue.append(calId);
    if (m_itemFetchQueue.size() == 1) {
        processNextItemLoad();
    }
    return {};
}

void CalDAVBackend::onCollectionsLoaded(KJob *job)
{
    qDebug() << "CalDAVBackend: onCollectionsLoaded triggered";
    m_activeJobs.removeOne(job); // Use removeOne for QList
    if (job->error()) {
        qDebug() << "CalDAVBackend: Error:" << job->error() << job->errorString();
        emit errorOccurred(job->errorString());
        return;
    }

    KDAV::DavCollectionsFetchJob *fetchJob = qobject_cast<KDAV::DavCollectionsFetchJob*>(job);
    KDAV::DavCollection::List collections = fetchJob->collections();
    QString collectionId = fetchJob->property("collectionId").toString();

    qDebug() << "CalDAVBackend: Found" << collections.size() << "collections";
    for (const KDAV::DavCollection &col : collections) {
        qDebug() << "CalDAVBackend: Collection URL:" << col.url().toDisplayString()
        << "Name:" << col.displayName()
        << "ContentTypes:" << col.contentTypes();
        if (col.contentTypes() & (KDAV::DavCollection::Events | KDAV::DavCollection::Todos | KDAV::DavCollection::Calendar)) {
            CalendarMetadata meta;
            QString simplifiedName = col.displayName().toLower().replace(QRegularExpression("[^a-z0-9]"), "_");
            meta.id = collectionId + "_" + simplifiedName;
            meta.name = col.displayName().isEmpty() ? col.url().toDisplayString() : col.displayName();
            m_idToUrl[meta.id] = col.url().toDisplayString();
            qDebug() << "CalDAVBackend: Discovered calendar" << meta.id << meta.name;
            emit calendarDiscovered(collectionId, meta);
        }
    }
}

void CalDAVBackend::processNextItemLoad()
{
    if (m_itemFetchQueue.isEmpty()) {
        qDebug() << "CalDAVBackend: Item load queue empty";
        return;
    }

    QString calId = m_itemFetchQueue.first();
    qDebug() << "CalDAVBackend: Processing loadItems for" << calId;

    Cal *cal = m_calMap.value(calId, nullptr);
    if (!cal) {
        qDebug() << "CalDAVBackend: No Cal found in m_calMap for" << calId;
        m_itemFetchQueue.removeFirst();
        processNextItemLoad();
        return;
    }

    QUrl url(m_idToUrl[calId]);
    url.setUserName(m_username);
    url.setPassword(m_password);
    KDAV::DavUrl davUrl(url, KDAV::CalDav);
    auto cache = std::make_shared<KDAV::EtagCache>(this);
    KDAV::DavItemsListJob *listJob = new KDAV::DavItemsListJob(davUrl, cache, this);
    listJob->setProperty("calId", calId);

    connect(listJob, &KJob::finished, this, [listJob](KJob *j) {
        qDebug() << "CalDAVBackend: List job finished:" << j << "Error:" << j->error() << j->errorString();
    });

    connect(listJob, &KDAV::DavItemsListJob::result, this, [this, cal, davUrl](KJob *job) {
        QString calId = cal->id();
        qDebug() << "CalDAVBackend: Items list completed for" << calId;
        if (job->error()) {
            qDebug() << "CalDAVBackend: List error for" << calId << ":" << job->errorString();
            emit errorOccurred(job->errorString());
            m_itemFetchQueue.removeFirst();
            processNextItemLoad();
            return;
        }

        KDAV::DavItemsListJob *listJob = qobject_cast<KDAV::DavItemsListJob*>(job);
        KDAV::DavItem::List items = listJob->items();
        qDebug() << "CalDAVBackend: Listed" << items.size() << "items for" << calId;
        if (items.isEmpty()) {
            emit calendarLoaded(cal);
            m_itemFetchQueue.removeFirst();
            processNextItemLoad();
            return;
        }

        QStringList urls;
        for (const KDAV::DavItem &item : items) {
            urls.append(item.url().toDisplayString());
        }
        qDebug() << "CalDAVBackend: Preparing MULTIGET for" << urls.size() << "items";

        KDAV::DavItemsFetchJob *fetchJob = new KDAV::DavItemsFetchJob(davUrl, urls, this);
        connect(fetchJob, &KDAV::DavItemsFetchJob::result, this, [this, cal](KJob *job) {
            QString calId = cal->id();
            if (job->error()) {
                qDebug() << "CalDAVBackend: MULTIGET error for" << calId << ":" << job->errorString();
                emit errorOccurred(job->errorString());
            } else {
                KDAV::DavItemsFetchJob *fJob = qobject_cast<KDAV::DavItemsFetchJob*>(job);
                KDAV::DavItem::List fetchedItems = fJob->items();
                qDebug() << "CalDAVBackend: MULTIGET fetched" << fetchedItems.size() << "items for" << calId;

                KCalendarCore::ICalFormat format;
                for (const KDAV::DavItem &item : fetchedItems) {
                    QByteArray rawData = item.data();
                    if (rawData.isEmpty()) {
                        qDebug() << "CalDAVBackend: Empty data for" << item.url().toDisplayString() << "- skipping";
                        continue;
                    }
                    auto incidence = format.fromString(QString::fromUtf8(rawData));
                    if (!incidence) {
                        qDebug() << "CalDAVBackend: Failed to parse item" << item.url().toDisplayString();
                        continue;
                    }

                    QString itemUid = incidence->uid().isEmpty() ? QString::number(qHash(item.url().toDisplayString())) : incidence->uid();
                    QSharedPointer<CalendarItem> calItem;
                    if (incidence->type() == KCalendarCore::IncidenceBase::TypeEvent) {
                        calItem = QSharedPointer<CalendarItem>(new Event(calId, itemUid, nullptr));
                    } else if (incidence->type() == KCalendarCore::IncidenceBase::TypeTodo) {
                        calItem = QSharedPointer<CalendarItem>(new Todo(calId, itemUid, nullptr));
                    }
                    if (calItem) {
                        calItem->setIncidence(incidence);
                        emit itemLoaded(cal, calItem);
                    }
                }
                emit calendarLoaded(cal);
            }
            m_itemFetchQueue.removeFirst();
            processNextItemLoad();
        });

        qDebug() << "CalDAVBackend: Starting MULTIGET job for" << calId;
        m_activeJobs.append(fetchJob); // Use append for QList
        fetchJob->start();
    });

    qDebug() << "CalDAVBackend: Starting items list job for" << calId << "Job:" << listJob;
    m_activeJobs.append(listJob); // Use append for QList
    listJob->start();
}

void CalDAVBackend::updateItem(const QString &calId, const QString &itemId, const QString &icalData)
{
    qDebug() << "CalDAVBackend: updateItem stub for" << calId << "item" << itemId;
    emit errorOccurred("Update not implemented for CalDAVBackend");
}

void CalDAVBackend::startSync(const QString &collectionId)
{
    qDebug() << "CalDAVBackend: Starting sync for collection" << collectionId;
    QList<CalendarMetadata> calendars = loadCalendars(collectionId); // Triggers async fetch
    connect(this, &CalDAVBackend::calendarLoaded, this, [this, collectionId](Cal *cal) {
        if (m_itemFetchQueue.isEmpty() && m_activeJobs.isEmpty()) {
            qDebug() << "CalDAVBackend: Sync completed for" << collectionId;
            emit syncCompleted(collectionId);
        }
    });
}
