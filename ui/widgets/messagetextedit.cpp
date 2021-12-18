#include <QMimeData>
#include "messagetextedit.h"

bool MessageTextEdit::canInsertFromMimeData(const QMimeData* source) const
{
    return source->hasImage() || source->hasUrls() || source->hasText();
}

void MessageTextEdit::insertFromMimeData(const QMimeData* source)
{
    if (source == nullptr) {
        return;
    }

    if (source->hasImage()) {
        emit imageInserted(source->imageData().value<QImage>());
    } else if (source->hasUrls()) {
        QList<QUrl> urls = source->urls();
        for (const QUrl& url : qAsConst(urls)) {
            if (url.isLocalFile()) {
                emit fileInserted(url.toLocalFile());
            }
        }
    } else {
        insertPlainText(source->text());
    }
}
