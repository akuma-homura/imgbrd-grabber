#ifndef BATCH_DOWNLOAD_CONTROLLER_H
#define BATCH_DOWNLOAD_CONTROLLER_H

#include <QJsonObject>
#include <QObject>
#include <QSharedPointer>
#include <QString>
#include <memory>
#include "downloader/image-save-result.h"

class BatchDownloader;
class DownloadQueryGroup;
class Image;
class Profile;

// QML-facing adapter around the shared batch downloader.
// Files are staged in app-private storage, then exported to Pictures/Grabber.
class BatchDownloadController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool running READ running NOTIFY changed)
    Q_PROPERTY(bool resumable READ resumable NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY changed)
    Q_PROPERTY(int total READ total NOTIFY changed)
    Q_PROPERTY(int processed READ processed NOTIFY changed)
    Q_PROPERTY(int saved READ saved NOTIFY changed)
    Q_PROPERTY(int skipped READ skipped NOTIFY changed)
    Q_PROPERTY(int failed READ failed NOTIFY changed)
    Q_PROPERTY(int recovered READ recovered NOTIFY changed)

    public:
        explicit BatchDownloadController(Profile *profile, QObject *parent = nullptr);
        ~BatchDownloadController() override;

        bool running() const { return m_running; }
        bool resumable() const { return m_resumable; }
        QString status() const { return m_status; }
        QString error() const { return m_error; }
        int total() const { return m_total; }
        int processed() const { return m_processed; }
        int saved() const { return m_saved; }
        int skipped() const { return m_skipped; }
        int failed() const { return m_failed; }
        int recovered() const { return m_recovered; }

        Q_INVOKABLE bool start(const QString &siteUrl, const QString &query,
            const QString &endpoint, const QString &postFilter,
            int page, int perPage, int limit, bool currentPage,
            bool organizeByArtist, bool originals);
        Q_INVOKABLE void cancel();
        Q_INVOKABLE bool resumeLast();

    signals:
        void changed();

    private:
        void onImageSaved(const QSharedPointer<Image> &image, const QList<ImageSaveResult> &results);
        void onBatchFinished();
        void finishWhenReady();
        void readPendingTask();
        void writePendingTask(bool unfinished);
        QString pendingFilePath() const;

        Profile *m_profile;
        std::unique_ptr<DownloadQueryGroup> m_query;
        BatchDownloader *m_downloader = nullptr;
        QString m_stageRoot;
        QString m_status = QStringLiteral("Idle");
        QString m_error;
        QJsonObject m_lastTask;
        bool m_running = false;
        bool m_resumable = false;
        bool m_cancelled = false;
        bool m_batchFinished = false;
        bool m_recovering = false;
        int m_pendingExports = 0;
        int m_total = 0;
        int m_processed = 0;
        int m_saved = 0;
        int m_skipped = 0;
        int m_failed = 0;
        int m_recovered = 0;
};

#endif // BATCH_DOWNLOAD_CONTROLLER_H
