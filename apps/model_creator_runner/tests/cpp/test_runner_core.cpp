#include "ohqdiscovery.h"
#include "structure_registry.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

class RunnerCoreTest : public QObject
{
    Q_OBJECT

private slots:
    void registryExposesSupportedModels();
    void registryRejectsCrossModelPresets();
    void discoveryPrefersConfiguredRoot();
    void discoveryPrefersConfiguredTemplateDirectory();
};

void RunnerCoreTest::registryExposesSupportedModels()
{
    const QStringList models = StructureRegistry::ModelTypes();
    QVERIFY(models.contains(QStringLiteral("HQ_Drywell")));
    QVERIFY(models.contains(QStringLiteral("VN_Drywell")));
    QVERIFY(models.contains(QStringLiteral("R_Bioswale")));
    QVERIFY(models.contains(QStringLiteral("JM_Bioretention")));
}

void RunnerCoreTest::registryRejectsCrossModelPresets()
{
    QVERIFY(StructureRegistry::IsPresetCompatibleWithModel(
        QStringLiteral("JM_Bioretention_Underdrain"), QStringLiteral("JM_Bioretention")));
    QVERIFY(!StructureRegistry::IsPresetCompatibleWithModel(
        QStringLiteral("JM_Bioretention_Underdrain"), QStringLiteral("HQ_Drywell")));
}

void RunnerCoreTest::discoveryPrefersConfiguredRoot()
{
    QTemporaryDir root;
    QVERIFY(root.isValid());
    qputenv("OHQ_ROOT", root.path().toUtf8());
    const QStringList candidates = OhqDiscovery::CandidateRoots(QString());
    QCOMPARE(candidates.constFirst(), QDir(root.path()).absolutePath());
    qunsetenv("OHQ_ROOT");
}

void RunnerCoreTest::discoveryPrefersConfiguredTemplateDirectory()
{
    QTemporaryDir templates;
    QVERIFY(templates.isValid());
    qputenv("OHQ_TEMPLATE_DIR", templates.path().toUtf8());
    QCOMPARE(OhqDiscovery::DetectTemplateDirectory({}, QString()),
             QDir(templates.path()).absolutePath());
    qunsetenv("OHQ_TEMPLATE_DIR");
}

QTEST_APPLESS_MAIN(RunnerCoreTest)
#include "test_runner_core.moc"
