#include "batch-download-controller.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QJsonDocument>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>
#include <QTimer>
#include <QtConcurrent>
#include "downloader/batch-downloader.h"
#include "downloader/download-query-group.h"
#include "models/image.h"
#include "models/profile.h"
#include "models/search-query/search-query.h"
#include "models/site.h"

#ifdef Q_OS_ANDROID
#include <QJniObject>
#include <QNativeInterface>
#endif

namespace {
int exportToPictures(const QString &source, const QString &relative)
{
    const QString cleaned = QDir::cleanPath(relative).replace('\\', '/');
    if (cleaned.isEmpty() || cleaned == "." || cleaned == ".."
        || cleaned.startsWith("../") || QDir::isAbsolutePath(cleaned)) {
        return 0;
    }

#ifdef Q_OS_ANDROID
    const QJniObject context = QNativeInterface::QAndroidApplication::context();
    if (!context.isValid()) {
        return 0;
    }

    const QJniObject src = QJniObject::fromString(source);
    const QJniObject dst = QJniObject::fromString(cleaned);
    return QJniObject::callStaticMethod<jint>(
        "org/bionus/grabber/MediaExporter", "exportFile",
        "(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;)I",
        context.object<jobject>(), src.object<jstring>(), dst.object<jstring>());
#else
    const QString pictures = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    const QString destination = QDir(pictures + "/Grabber").filePath(cleaned);
    if (!QDir().mkpath(QFileInfo(destination).absolutePath())) {
        return 0;
    }
    if (QFile::exists(destination)) {
        return 2;
    }
    return QFile::copy(source, destination) ? 1 : 0;
#endif
}
}

BatchDownloadController::BatchDownloadController(Profile *profile, QObject *parent)
    : QObject(parent), m_profile(profile)
{
    m_stageRoot = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        + "/grabber-batch";
    QDir().mkpath(m_stageRoot);
    readPendingTask();
}

BatchDownloadController::~BatchDownloadController()
{
    if (m_downloader != nullptr) {
        m_downloader->abort();
    }
}

QString BatchDownloadController::pendingFilePath() const
{
    return QDir(m_stageRoot).filePath("pending-task.json");
}

void BatchDownloadController::readPendingTask()
{
    QFile file(pendingFilePath());
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) {
        return;
    }
    m_lastTask = doc.object();
    m_resumable = m_lastTask.value("unfinished").toBool(false);
    if (m_resumable) {
        m_status = QStringLiteral("Interrupted task available");
    }
}

void BatchDownloadController::writePendingTask(bool unfinished)
{
    m_lastTask["unfinished"] = unfinished;
    QSaveFile file(pendingFilePath());
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(m_lastTask).toJson(QJsonDocument::Compact));
        file.commit();
    }
    m_resumable = unfinished;
    emit changed();
}

bool BatchDownloadController::start(const QString &siteUrl, const QString &query,
    const QString &endpoint, const QString &postFilter,
    int page, int perPage, int limit, bool currentPage,
    bool organizeByArtist, bool originals)
{
    if (m_running) {
        m_error = QStringLiteral("Another download is running");
        emit changed();
        return false;
    }
    Site *site = m_profile->getSites().value(siteUrl, nullptr);
    if (site == nullptr || page < 1 || perPage < 1 || perPage > 500
        || limit < 1 || limit > 10000) {
        m_error = QStringLiteral("Invalid site, page or image limit");
        emit changed();
        return false;
    }
    if (!QDir().mkpath(m_stageRoot)) {
        m_error = QStringLiteral("Unable to create private download directory");
        emit changed();
        return false;
    }

    if (m_downloader != nullptr) {
        m_downloader->deleteLater();
        m_downloader = nullptr;
    }
    m_query.reset();

    const SearchQuery search = endpoint.isEmpty()
        ? SearchQuery(query.trimmed().split(' ', Qt::SkipEmptyParts))
        : SearchQuery(endpoint, {{ "search", query.trimmed() }});
    const QString filename = organizeByArtist
        ? QStringLiteral("%website%/%artist%/%md5%.%ext%")
        : QStringLiteral("%website%/%md5%.%ext%");
    const int maxImages = currentPage ? perPage : limit;
    m_query.reset(new DownloadQueryGroup(search, page, perPage, maxImages,
        postFilter.trimmed().split(' ', Qt::SkipEmptyParts), false, site, filename, m_stageRoot));
    m_query->pageOnly = currentPage;

    m_downloader = new BatchDownloader(m_query.get(), m_profile, this);
    connect(m_downloader, &BatchDownloader::imageSaveFinished,
        this, &BatchDownloadController::onImageSaved);
    connect(m_downloader, &BatchDownloader::finished,
        this, &BatchDownloadController::onBatchFinished);

    m_total = maxImages;
    m_processed = m_saved = m_skipped = m_failed = m_pendingExports = 0;
    m_cancelled = false;
    m_batchFinished = false;
    m_running = true;
    m_error.clear();
    m_status = QStringLiteral("Downloading");
    m_profile->getSettings()->setValue("Save/downloadoriginals", originals);
    m_lastTask = {
        {"site", siteUrl}, {"query", query}, {"endpoint", endpoint},
        {"postFilter", postFilter}, {"page", page}, {"perPage", perPage},
        {"limit", limit}, {"currentPage", currentPage},
        {"organizeByArtist", organizeByArtist}, {"originals", originals}
    };
    writePendingTask(true);
    QTimer::singleShot(0, m_downloader, &BatchDownloader::start);
    return true;
}

void BatchDownloadController::onImageSaved(const QSharedPointer<Image> &image,
    const QList<ImageSaveResult> &results)
{
    Q_UNUSED(image)
    if (results.isEmpty()) {
        ++m_failed;
        ++m_processed;
        emit changed();
        return;
    }

    const ImageSaveResult &result = results.first();
    if (result.result == Image::SaveResult::Blacklisted
        || result.result == Image::SaveResult::AlreadyExistsMd5
        || result.result == Image::SaveResult::AlreadyExistsDeletedMd5) {
        ++m_skipped;
        ++m_processed;
        emit changed();
        return;
    }

    const QFileInfo source(result.path);
    if (!source.isFile()) {
        ++m_failed;
        ++m_processed;
        emit changed();
        return;
    }

    const QString relative = QDir(m_stageRoot).relativeFilePath(source.absoluteFilePath());
    const QString path = source.absoluteFilePath();
    ++m_pendingExports;

    auto *watcher = new QFutureWatcher<int>(this);
    connect(watcher, &QFutureWatcher<int>::finished, this, [this, watcher, path]() {
        const int exportResult = watcher->result();
        if (exportResult == 1) {
            ++m_saved;
        } else if (exportResult == 2) {
            ++m_skipped;
        } else {
            ++m_failed;
            m_error = QStringLiteral("Failed to export one or more files to Pictures/Grabber");
        }
        if (exportResult != 0) {
            QFile::remove(path);
        }
        ++m_processed;
        --m_pendingExports;
        watcher->deleteLater();
        emit changed();
        finishWhenReady();
    });
    watcher->setFuture(QtConcurrent::run([path, relative]() {
        return exportToPictures(path, relative);
    }));
}

void BatchDownloadController::onBatchFinished()
{
    m_batchFinished = true;
    m_status = QStringLiteral("Finishing file exports");
    emit changed();
    finishWhenReady();
}

void BatchDownloadController::finishWhenReady()
{
    if (!m_running || !m_batchFinished || m_pendingExports > 0) {
        return;
    }
    m_running = false;
    m_status = m_cancelled ? QStringLiteral("Cancelled")
        : (m_failed > 0 ? QStringLiteral("Finished with errors") : QStringLiteral("Finished"));
    writePendingTask(m_cancelled || m_failed > 0);
}

void BatchDownloadController::cancel()
{
    if (!m_running) {
        return;
    }
    m_cancelled = true;
    if (m_downloader != nullptr) {
        m_downloader->abort();
    }
    m_batchFinished = true;
    m_status = QStringLiteral("Stopping");
    emit changed();
    finishWhenReady();
}

bool BatchDownloadController::resumeLast()
{
    if (m_running || !m_resumable) {
        return false;
    }
    const QJsonObject task = m_lastTask;
    return start(task.value("site").toString(),
        task.value("query").toString(),
        task.value("endpoint").toString(),
        task.value("postFilter").toString(),
        task.value("page").toInt(1),
        task.value("perPage").toInt(20),
        task.value("limit").toInt(500),
        task.value("currentPage").toBool(false),
        task.value("organizeByArtist").toBool(true),
        task.value("originals").toBool(true));
}
