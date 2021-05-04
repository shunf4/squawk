/*
 * Squawk messenger. 
 * Copyright (C) 2019  Yury Gubich <blue@macaw.me>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "utils.h"

QString Shared::generateUUID()
{
    uuid_t uuid;
    uuid_generate(uuid);
    
    char uuid_str[36];
    uuid_unparse_lower(uuid, uuid_str);
    return uuid_str;
}


static const QRegularExpression urlReg("(?<!<a\\shref=['\"])(?<!<img\\ssrc=['\"])("
                                "(?:https?|ftp):\\/\\/"
                                    "\\w+"
                                    "(?:"
                                        "[\\w\\.\\,\\/\\:\\;\\?\\&\\=\\@\\%\\#\\+\\-]?"
                                        "(?:"
                                            "\\([\\w\\.\\,\\/\\:\\;\\?\\&\\=\\@\\%\\#\\+\\-]+\\)"
                                        ")?"
                                    ")*"
                                ")");

QString Shared::processMessageBody(const QString& msg)
{
    QString processed = msg.toHtmlEscaped();
    processed.replace(urlReg, "<a href=\"\\1\">\\1</a>");
    return "<p style=\"white-space: pre-wrap;\">" + processed + "</p>";
}

static const QStringList query = {"query", "default", "inode/directory"};
static const QRegularExpression dolphinReg("[Dd]olphin");
static const QRegularExpression nautilusReg("[Nn]autilus");
static const QRegularExpression cajaReg("[Cc]aja");
static const QRegularExpression nemoReg("[Nn]emo");
static const QRegularExpression konquerorReg("kfmclient");

void Shared::showInDirectory(const QString& path)
{
    QFileInfo info = path;
    if (info.exists()) {
        QProcess proc;
        proc.start("xdg-mime", query);
        proc.waitForFinished();
        QString output = proc.readLine().simplified();
        if (output.contains(dolphinReg)) {
            proc.startDetached("dolphin", QStringList() << "--select" << info.canonicalFilePath());
        } else if (output.contains(nautilusReg)) {
            proc.startDetached("nautilus", QStringList() << "--no-desktop" << info.canonicalFilePath());
        } else if (output.contains(cajaReg)) {
            proc.startDetached("caja", QStringList() << "--no-desktop" << info.canonicalFilePath());
        } else if (output.contains(nemoReg)) {
            proc.startDetached("nemo", QStringList() << "--no-desktop" << info.canonicalFilePath());
        } else if (output.contains(konquerorReg)) {
            proc.startDetached("konqueror", QStringList() << "--select" << info.canonicalFilePath());
        } else {
            QString folder;
            if (info.isDir()) {
                folder = info.canonicalFilePath();
            } else {
                folder = info.canonicalPath();
            }
            QDesktopServices::openUrl(QUrl::fromLocalFile(folder));
        }
    }
}
