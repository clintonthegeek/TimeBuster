#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QDir>
#include <QDebug>
#include <QFile>
#include <QXmlStreamReader>
#include "configmanager.h"
#include "localbackend.h"
#include "backendinfo.h"

class TestConfigManager : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testSaveAndLoadConfig();

private:
    QTemporaryDir tempDir;
    ConfigManager *configManager;
    QString collectionId;
    QString collectionName;
    QString kalbPath;
};

void TestConfigManager::initTestCase()
{
    QVERIFY(tempDir.isValid());
    QString basePath = tempDir.path();
    configManager = new ConfigManager(this);
    // If ConfigManager has a setBasePath method, uncomment this:
    // configManager->setBasePath(basePath);

    collectionId = "col0";
    collectionName = "Remote Collection";
    kalbPath = QDir(basePath).filePath("configs/My Collection.kalb");
}

void TestConfigManager::cleanupTestCase()
{
    delete configManager;
}

void TestConfigManager::testSaveAndLoadConfig()
{
    // Create a LocalBackend with a relative rootPath within the temp directory
    QString relativeRootPath = "local_storage";
    QDir baseDir(tempDir.path());
    if (!baseDir.exists(relativeRootPath) && !baseDir.mkdir(relativeRootPath)) {
        QFAIL("Failed to create local_storage directory");
    }
    LocalBackend *backend = new LocalBackend(QDir(tempDir.path()).filePath(relativeRootPath), this);
    BackendInfo info;
    info.backend = backend;
    info.priority = 1;
    info.syncOnOpen = false;
    QList<BackendInfo> backends = {info};

    // Ensure the configs directory exists
    QDir configsDir(tempDir.path());
    if (!configsDir.exists("configs") && !configsDir.mkdir("configs")) {
        QFAIL("Failed to create configs directory");
    }

    // Save the configuration
    QString savedPath = configManager->saveBackendConfig(collectionId, collectionName, backends, kalbPath);
    QVERIFY(!savedPath.isEmpty());

    // Verify the file exists and contains the expected XML content
    QFile file(savedPath);
    QVERIFY(file.exists());
    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));

    QXmlStreamReader xml(&file);
    QString id, name, type, rootPath, priority, syncOnOpen;
    while (!xml.atEnd() && !xml.hasError()) {
        QXmlStreamReader::TokenType token = xml.readNext();
        if (token == QXmlStreamReader::StartElement) {
            if (xml.name() == "Id") {
                id = xml.readElementText();
            } else if (xml.name() == "Name") {
                name = xml.readElementText();
            } else if (xml.name() == "Backend") {
                type = xml.attributes().value("type").toString();
            } else if (xml.name() == "RootPath") {
                rootPath = xml.readElementText();
            } else if (xml.name() == "priority") {
                priority = xml.readElementText();
            } else if (xml.name() == "SyncOnOpen") {
                syncOnOpen = xml.readElementText();
            }
        }
    }
    QVERIFY(!xml.hasError());
    file.close();

    QCOMPARE(id, QString("col0"));
    QCOMPARE(name, QString("Remote Collection"));
    QCOMPARE(type, QString("local"));
    QString expectedRootPath = QDir(QFileInfo(savedPath).absolutePath()).relativeFilePath(backend->rootPath());
    QCOMPARE(rootPath, expectedRootPath.isEmpty() ? "." : expectedRootPath);
    QCOMPARE(priority, QString("1"));
    QCOMPARE(syncOnOpen, QString("false"));

    // Load the configuration
    QVariantMap config = configManager->loadConfig(collectionId, kalbPath);
    QCOMPARE(config["id"].toString(), QString("col0"));
    QCOMPARE(config["name"].toString(), QString("Remote Collection"));

    QVariantList backendsList = config["backends"].toList();
    QCOMPARE(backendsList.size(), 1);
    ConfigManager::BackendConfig backendConfig = backendsList[0].value<ConfigManager::BackendConfig>();
    QCOMPARE(backendConfig.type, QString("local"));
    QCOMPARE(backendConfig.details["rootPath"].toString(), rootPath); // Relative path as saved
    QCOMPARE(backendConfig.priority, 1);
    QCOMPARE(backendConfig.syncOnOpen, false);
}

QTEST_MAIN(TestConfigManager)
#include "test_configmanager.moc"
