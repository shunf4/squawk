#include <QUrl>
#include <QObject>
#include <KIO/OpenFileManagerWindowJob>

extern "C" void highlightInFileManager(const QUrl& url) {
    KIO::OpenFileManagerWindowJob* job = KIO::highlightInFileManager({url});
    QObject::connect(job, &KIO::OpenFileManagerWindowJob::result, job, &KIO::OpenFileManagerWindowJob::deleteLater);
}
