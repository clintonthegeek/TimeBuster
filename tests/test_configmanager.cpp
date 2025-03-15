#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QDir>
#include <QDebug>
#include <QSettings>
#include "configmanager.h"
#include "localbackend.h"

class TestConfigManager : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testSaveAndLoadConfig();

private:
    QTemporaryDir tempDir; // Will use this for a clean test environment within build dir
    ConfigManager *configManager;
    QString collectionId;
    QString collectionName;
    QString kalbPath;
};

void TestConfigManager::initTestCase()
{
    // Ensure tempDir is valid within the build directory
    QVERIFY(tempDir.isValid());
    QString basePath = tempDir.path(); // This will be a unique temp dir under build (e.g., /build/test_configmanager-XXXXXX)
    configManager = new ConfigManager(this);
    configManager->setBasePath(basePath);

    collectionId = "col0";
    collectionName = "Remote Collection";
    // Use a relative path within the temp directory
    kalbPath = QDir(basePath).filePath("configs/My Collection.kalb");
}

void TestConfigManager::cleanupTestCase()
{
    // tempDir will automatically clean up when it goes out of scope
    delete configManager;
}

void TestConfigManager::testSaveAndLoadConfig()
{
    // Create a LocalBackend with a relative rootPath within the temp directory
    QString relativeRootPath = "local_storage"; // Relative to basePath
    QDir baseDir(tempDir.path());
    if (!baseDir.exists(relativeRootPath) && !baseDir.mkdir(relativeRootPath)) {
        QFAIL("Failed to create local_storage directory");
    }
    LocalBackend *backend = new LocalBackend(QDir(tempDir.path()).filePath(relativeRootPath), this);
    QList<SyncBackend*> backends = {backend};

    // Ensure the configs directory exists
    QDir configsDir(tempDir.path());
    if (!configsDir.exists("configs") && !configsDir.mkdir("configs")) {
        QFAIL("Failed to create configs directory");
    }

    // Save the configuration
    configManager->saveBackendConfig(collectionId, collectionName, backends, kalbPath);

    // Verify the file exists and contains the expected content using QSettings
    QSettings settings(kalbPath, QSettings::IniFormat);
    settings.beginGroup("Collection");
    QCOMPARE(settings.value("id").toString(), QString("col0"));
    QCOMPARE(settings.value("name").toString(), QString("Remote Collection"));
    settings.endGroup();

    int size = settings.beginReadArray("backends");
    QCOMPARE(size, 1);
    settings.setArrayIndex(0);
    QCOMPARE(settings.value("type").toString(), QString("local"));
    QCOMPARE(settings.value("rootPath").toString(), QDir(tempDir.path()).filePath(relativeRootPath));
    settings.endArray();

    // Load the configuration
    QVariantMap config = configManager->loadConfig(collectionId, kalbPath);
    QCOMPARE(config["id"].toString(), QString("col0"));
    QCOMPARE(config["name"].toString(), QString("Remote Collection"));

    QVariantList backendsList = config["backends"].toList();
    QCOMPARE(backendsList.size(), 1);
    QVariantMap backendConfig = backendsList[0].toMap();
    QCOMPARE(backendConfig["type"].toString(), QString("local"));
    QCOMPARE(backendConfig["rootPath"].toString(), QDir(tempDir.path()).filePath(relativeRootPath));
}

QTEST_MAIN(TestConfigManager)
#include "test_configmanager.moc"
