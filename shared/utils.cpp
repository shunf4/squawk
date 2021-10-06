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
#include <QUuid>

QString Shared::generateUUID()
{
    return QUuid::createUuid().toString();
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
    return "<span style=\"white-space: pre-wrap;\">" + processed + "</span>";
}
