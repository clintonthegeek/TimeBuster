#include "caldavbackend.h"
#include <KDAV/DavUrl>
#include <KDAV/DavItemsListJob>
#include <KDAV/DavItemsFetchJob>
#include <KDAV/EtagCache>
#include <KCalendarCore/ICalFormat>
#include <QUrl>
#include <QDebug>
#include "cal.h"
#include "calendaritemfactory.h"

CalDAVBackend::CalDAVBackend(const QString &serverUrl, const QString &username, const QString &password, QObject *parent)
    : SyncBackend(parent), m_serverUrl(serverUrl), m_username(username), m_password(password)
{
}

CalDAVBackend::~CalDAVBackend()
{
    // Cancel all active jobs to prevent them from running after destruction
    for (KJob *job : m_activeJobs) {
        job->kill(KJob::Quietly); // Quietly kill the job
    }
    m_activeJobs.clear();
    m_calMap.clear();
    m_itemFetchQueue.clear();
}
QList<CalendarMetadata> CalDAVBackend::fetchCalendars(const QString &collectionId)
{
    qDebug() << "CalDAVBackend: Starting fetchCalendars for collection" << collectionId;
    QUrl url(m_serverUrl);
    url.setUserName(m_username);
    url.setPassword(m_password);
    qDebug() << "CalDAV URL:" << url.toString(QUrl::RemovePassword);

    KDAV::DavUrl davUrl(url, KDAV::CalDav);
    KDAV::DavCollectionsFetchJob *job = new KDAV::DavCollectionsFetchJob(davUrl);
    job->setProperty("collectionId", collectionId); // Pass collectionId to job
    connect(job, &KDAV::DavCollectionsFetchJob::result, this, &CalDAVBackend::onCollectionsFetched);
    qDebug() << "CalDAVBackend: Job created, starting...";
    job->start();

    qDebug() << "CalDAVBackend: fetchCalendars returning empty list (async)";
    return QList<CalendarMetadata>();
}

void CalDAVBackend::onCollectionsFetched(KJob *job)
{
    qDebug() << "CalDAVBackend: onCollectionsFetched triggered";
    if (job->error()) {
        qDebug() << "CalDAVBackend: Error:" << job->error() << job->errorString();
        emit errorOccurred(job->errorString());
        return;
    }

    KDAV::DavCollectionsFetchJob *fetchJob = qobject_cast<KDAV::DavCollectionsFetchJob*>(job);
    KDAV::DavCollection::List collections = fetchJob->collections();
    QList<CalendarMetadata> calendars;

    qDebug() << "CalDAVBackend: Found" << collections.size() << "collections";
    for (const KDAV::DavCollection &col : collections) {
        qDebug() << "CalDAVBackend: Collection URL:" << col.url().toDisplayString()
        << "Name:" << col.displayName()
        << "ContentTypes:" << col.contentTypes();
        if (col.contentTypes() & (KDAV::DavCollection::Events | KDAV::DavCollection::Todos | KDAV::DavCollection::Calendar)) {
            CalendarMetadata meta;
            meta.id = col.url().url().toString();
            meta.name = col.displayName().isEmpty() ? col.url().toDisplayString() : col.displayName();
            calendars.append(meta);
            m_idToUrl[meta.id] = col.url().toDisplayString();
            qDebug() << "CalDAVBackend: Added calendar" << meta.id << meta.name;
        }
    }

    QString collectionId = fetchJob->property("collectionId").toString();
    qDebug() << "CalDAVBackend: Emitting calendarsFetched for" << collectionId << "with" << calendars.size() << "calendars";
    emit calendarsFetched(collectionId, calendars);
    emit dataFetched();
}

void CalDAVBackend::processNextItemFetch()
{
    if (m_itemFetchQueue.isEmpty()) {
        qDebug() << "CalDAVBackend: Item fetch queue empty";
        return;
    }

    QString calId = m_itemFetchQueue.first();
    qDebug() << "CalDAVBackend: Processing fetchItems for" << calId;

    QUrl url(m_idToUrl[calId]);
    url.setUserName(m_username);
    url.setPassword(m_password);
    KDAV::DavUrl davUrl(url, KDAV::CalDav);
    auto cache = std::make_shared<KDAV::EtagCache>(this);
    KDAV::DavItemsListJob *listJob = new KDAV::DavItemsListJob(davUrl, cache, this);
    listJob->setProperty("calId", calId);
    m_activeJobs.append(listJob); // Track the job

    connect(listJob, &KJob::finished, this, [this, listJob](KJob *j) {
        qDebug() << "CalDAVBackend: List job finished:" << j << "Error:" << j->error() << j->errorString();
        m_activeJobs.removeOne(j); // Remove from active jobs
    });

    connect(listJob, &KDAV::DavItemsListJob::result, this, [this, calId, davUrl](KJob *job) {
        qDebug() << "CalDAVBackend: Items list completed for" << calId;
        if (job->error()) {
            qDebug() << "CalDAVBackend: List error for" << calId << ":" << job->errorString();
            emit errorOccurred(job->errorString());
            m_itemFetchQueue.removeFirst();
            removeCal(calId);
            processNextItemFetch();
            return;
        }

        KDAV::DavItemsListJob *listJob = qobject_cast<KDAV::DavItemsListJob*>(job);
        KDAV::DavItem::List items = listJob->items();
        qDebug() << "CalDAVBackend: Listed" << items.size() << "items for" << calId;
        if (items.isEmpty()) {
            emit itemsFetched(calId, QList<CalendarItem*>());
            emit dataFetched();
            m_itemFetchQueue.removeFirst();
            removeCal(calId);
            processNextItemFetch();
            return;
        }

        QStringList urls;
        for (const KDAV::DavItem &item : items) {
            urls.append(item.url().toDisplayString());
        }
        qDebug() << "CalDAVBackend: Preparing MULTIGET for" << urls.size() << "items";

        KDAV::DavItemsFetchJob *fetchJob = new KDAV::DavItemsFetchJob(davUrl, urls, this);
        m_activeJobs.append(fetchJob); // Track the job

        connect(fetchJob, &KDAV::DavItemsFetchJob::result, this, [this, calId](KJob *job) {
            if (job->error()) {
                qDebug() << "CalDAVBackend: MULTIGET error for" << calId << ":" << job->errorString();
                emit errorOccurred(job->errorString());
            } else {
                KDAV::DavItemsFetchJob *fJob = qobject_cast<KDAV::DavItemsFetchJob*>(job);
                KDAV::DavItem::List fetchedItems = fJob->items();
                qDebug() << "CalDAVBackend: MULTIGET fetched" << fetchedItems.size() << "items for" << calId;

                QList<CalendarItem*> result;
                KCalendarCore::ICalFormat format;
                Cal *cal = getCal(calId);
                if (!cal) {
                    qDebug() << "CalDAVBackend: No Cal found for" << calId;
                    return;
                }

                for (const KDAV::DavItem &item : fetchedItems) {
                    QByteArray rawData = item.data();
                    if (rawData.isEmpty()) {
                        qDebug() << "CalDAVBackend: Empty data for" << item.url().toDisplayString() << "- skipping";
                        continue;
                    }
                    qDebug() << "CalDAVBackend: Raw data size:" << rawData.size() << "bytes" << rawData.left(100);
                    auto incidence = format.fromString(QString::fromUtf8(rawData));
                    if (!incidence) {
                        qDebug() << "CalDAVBackend: Failed to parse item" << item.url().toDisplayString();
                        continue;
                    }

                    CalendarItem *calItem = CalendarItemFactory::createItem(item.url().toDisplayString(), incidence, cal);
                    if (calItem) {
                        result.append(calItem);
                        cal->addItem(calItem);
                        qDebug() << "CalDAVBackend: Added" << calItem->type() << calItem->id();
                    }
                }

                emit itemsFetched(calId, result);
                emit dataFetched();
            }
            m_itemFetchQueue.removeFirst();
            removeCal(calId);
            processNextItemFetch();
        });

        connect(fetchJob, &KJob::finished, this, [this, fetchJob](KJob *j) {
            m_activeJobs.removeOne(j); // Remove from active jobs
        });

        qDebug() << "CalDAVBackend: Starting MULTIGET job for" << calId;
        fetchJob->start();
    });

    qDebug() << "CalDAVBackend: Starting items list job for" << calId << "Job:" << listJob;
    listJob->start();
}

void CalDAVBackend::fetchItemData(const QString &calId, const KDAV::DavItem::List &items, int index)
{
    if (index >= items.size()) {
        qDebug() << "CalDAVBackend: All items fetched for" << calId;
        QList<CalendarItem*> result;
        KCalendarCore::ICalFormat format;
        Cal *cal = getCal(calId); // Retrieve cal from map
        if (!cal) {
            qDebug() << "CalDAVBackend: No Cal found for" << calId;
            return;
        }

        for (const KDAV::DavItem &item : items) {
            QByteArray rawData = item.data();
            if (rawData.isEmpty()) {
                qDebug() << "CalDAVBackend: Empty data for" << item.url().toDisplayString() << "- skipping";
                continue;
            }
            qDebug() << "CalDAVBackend: Raw data size:" << rawData.size() << "bytes" << rawData.left(100);
            auto incidence = format.fromString(QString::fromUtf8(rawData));
            if (!incidence) {
                qDebug() << "CalDAVBackend: Failed to parse item" << item.url().toDisplayString();
                continue;
            }

            // Use CalendarItemFactory to create items, parent to cal
            CalendarItem *calItem = CalendarItemFactory::createItem(item.url().toDisplayString(), incidence, cal);
            if (calItem) {
                result.append(calItem);
                cal->addItem(calItem); // Ensure cal owns the item
                qDebug() << "CalDAVBackend: Added" << calItem->type() << calItem->id();
            }
        }

        emit itemsFetched(calId, result);
        emit dataFetched();
        m_itemFetchQueue.removeFirst();
        removeCal(calId); // Clean up cal reference
        processNextItemFetch();
        return;
    }

    KDAV::DavItemFetchJob *fetchJob = new KDAV::DavItemFetchJob(items[index], this);
    connect(fetchJob, &KDAV::DavItemFetchJob::result, this, [this, calId, items, index](KJob *job) {
        if (job->error()) {
            qDebug() << "CalDAVBackend: Fetch error for item" << items[index].url().toDisplayString() << ":" << job->errorString();
        } else {
            KDAV::DavItemFetchJob *fJob = qobject_cast<KDAV::DavItemFetchJob*>(job);
            KDAV::DavItem fetchedItem = fJob->item();
            qDebug() << "CalDAVBackend: Fetched item" << fetchedItem.url().toDisplayString() << "size:" << fetchedItem.data().size();
            const_cast<KDAV::DavItem&>(items[index]) = fetchedItem; // Update item with full data
        }
        fetchItemData(calId, items, index + 1); // Next item
    });
    qDebug() << "CalDAVBackend: Starting fetch job for" << items[index].url().toDisplayString();
    fetchJob->start();
}

QList<CalendarItem*> CalDAVBackend::fetchItems(Cal *cal)
{
    qDebug() << "CalDAVBackend: Queuing fetchItems for" << cal->id();
    if (!m_idToUrl.contains(cal->id())) {
        qDebug() << "CalDAVBackend: Unknown calId" << cal->id();
        emit dataFetched();
        return QList<CalendarItem*>();
    }

    m_itemFetchQueue.append(cal->id());
    setCal(cal->id(), cal); // Store cal reference with calId
    if (m_itemFetchQueue.size() == 1) {
        processNextItemFetch(); // Start if queue was empty
    }
    // Return an empty list for now; items will be added asynchronously
    return QList<CalendarItem*>();
}

void CalDAVBackend::storeCalendars(const QString &collectionId, const QList<Cal*> &calendars)
{
    qDebug() << "CalDAVBackend: storeCalendars stub for" << collectionId;
    for (Cal *cal : calendars) {
        m_idToUrl[cal->id()] = m_serverUrl + "/" + cal->id();
    }
}

void CalDAVBackend::storeItems(Cal *cal, const QList<CalendarItem*> &items)
{
    qDebug() << "CalDAVBackend: storeItems stub for" << cal->id();
}
