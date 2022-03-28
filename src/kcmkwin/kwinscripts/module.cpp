/*
    SPDX-FileCopyrightText: 2011 Tamas Krutki <ktamasw@gmail.com>
    SPDX-FileCopyrightText: 2012 Martin Gräßlin <mgraesslin@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include "module.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QFileDialog>
#include <QStandardPaths>
#include <QStringList>

#include <KAboutData>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KMessageBox>
#include <KMessageWidget>
#include <KNSWidgets/Button>
#include <KPackage/Package>
#include <KPackage/PackageLoader>
#include <KPackage/PackageStructure>
#include <KPluginFactory>

#include <KCMultiDialog>

#include "kwinscriptsdata.h"
#include "config-kwin.h"

Module::Module(QObject *parent, const QVariantList &args)
    : KQuickAddons::ConfigModule(parent, args)
    , m_kwinConfig(KSharedConfig::openConfig("kwinrc"))
    , m_kwinScriptsData(new KWinScriptsData(this))
    , m_model(new KPluginModel(this))
{
    KAboutData *about = new KAboutData("kwin-scripts",
                                       i18n("KWin Scripts"),
                                       KWIN_VERSION_STRING,
                                       i18n("Configure KWin scripts"),
                                       KAboutLicense::GPL_V2);

    about->addAuthor(i18n("Tamás Krutki"));
    setAboutData(about);

    // Hide the help button, because there is no help
    setButtons(Apply | Default);
}

Module::~Module()
{
}

void Module::importScript()
{
    QString path = QFileDialog::getOpenFileName(nullptr, i18n("Import KWin Script"), QDir::homePath(),
                                                i18n("*.kwinscript|KWin scripts (*.kwinscript)"));

    if (path.isNull()) {
        return;
    }

    using namespace KPackage;
    PackageStructure *structure = PackageLoader::self()->loadPackageStructure(QStringLiteral("KWin/Script"));
    Package package(structure);

    KJob *installJob = package.update(path);
    installJob->setProperty("packagePath", path); // so we can retrieve it later for showing the script's name
    connect(installJob, &KJob::result, this, &Module::importScriptInstallFinished);
}

void Module::configure(const KPluginMetaData &data)
{
    qWarning()<<Q_FUNC_INFO << data.fileName();
    auto dialog = new KCMultiDialog();
    dialog->addModule(data);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

void Module::togglePendingDeletion(const KPluginMetaData &data)
{
    if (m_pendingDeletions.contains(data)) {
        m_pendingDeletions.removeOne(data);
    } else {
        m_pendingDeletions.append(data);
    }
    Q_EMIT pendingDeletionsChanged();
}

void Module::importScriptInstallFinished(KJob *job)
{
    // if the applet is already installed, just add it to the containment
    /*if (job->error() != KJob::NoError) {
        ui->messageWidget->setText(i18nc("Placeholder is error message returned from the install service", "Cannot import selected script.\n%1", job->errorString()));
        ui->messageWidget->setMessageType(KMessageWidget::Error);
        ui->messageWidget->animatedShow();
        return;
    }*/

    using namespace KPackage;

    // so we can show the name of the package we just imported
    PackageStructure *structure = PackageLoader::self()->loadPackageStructure(QStringLiteral("KWin/Script"));
    Package package(structure);
    package.setPath(job->property("packagePath").toString());
    Q_ASSERT(package.isValid());

    /*ui->messageWidget->setText(i18nc("Placeholder is name of the script that was imported", "The script \"%1\" was successfully imported.", package.metadata().name()));
    ui->messageWidget->setMessageType(KMessageWidget::Information);
    ui->messageWidget->animatedShow();

    updateListViewContents();
     */

    setNeedsSave(false);
}

void Module::defaults()
{
    m_pendingDeletions.clear();
    Q_EMIT pendingDeletionsChanged();
}

void Module::load()
{
    m_model->addPlugins(m_kwinScriptsData->pluginMetaDataList(), QStringLiteral("bla :)"));
    m_pendingDeletions.clear();
    Q_EMIT pendingDeletionsChanged();

    setNeedsSave(false);
}

void Module::save()
{
    using namespace KPackage;
    PackageStructure *structure = PackageLoader::self()->loadPackageStructure(QStringLiteral("KWin/Script"));
    for (const KPluginMetaData &info : qAsConst(m_pendingDeletions)) {
        // We can get the package root from the entry path
        QDir root = QFileInfo(info.fileName()).dir();
        root.cdUp();
        KJob *uninstallJob = Package(structure).uninstall(info.pluginId(), root.absolutePath());
        connect(uninstallJob, &KJob::result, this, [this, uninstallJob]() {
            // If the uninstallation is successful the entry will be immediately removed
            /*if (!uninstallJob->errorString().isEmpty()) {
                ui->messageWidget->setText(i18n("Error when uninstalling KWin Script: %1", uninstallJob->errorString()));
                ui->messageWidget->setMessageType(KMessageWidget::Error);
                ui->messageWidget->animatedShow();
            }*/
        });
    }
    m_pendingDeletions.clear();

    m_kwinConfig->sync();
    QDBusMessage message = QDBusMessage::createMethodCall("org.kde.KWin", "/Scripting", "org.kde.kwin.Scripting", "start");
    QDBusConnection::sessionBus().asyncCall(message);

    setNeedsSave(false);
}

K_PLUGIN_FACTORY_WITH_JSON(KcmKWinScriptsFactory, "kwinscripts.json",
                 registerPlugin<Module>();
                 registerPlugin<KWinScriptsData>();
                )

#include "module.moc"
