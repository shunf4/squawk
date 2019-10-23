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

#ifndef VCARD_H
#define VCARD_H

#include <QWidget>
#include <QScopedPointer>
#include <QPixmap>
#include <QMenu>
#include <QFileDialog>
#include <QMimeDatabase>
#include <QImage>
#include <QStandardPaths>

#include <set>

#include "../../global.h"

namespace Ui
{
class VCard;
}

/**
 * @todo write docs
 */
class VCard : public QWidget
{
    Q_OBJECT
public:
    VCard(const QString& jid, bool edit = false, QWidget* parent = nullptr);
    ~VCard();
    
    void setVCard(const Shared::VCard& card);
    void setVCard(const QString& jid, const Shared::VCard& card);
    QString getJid() const;
    
signals:
    void saveVCard(const Shared::VCard& card);
    
private slots:
    void onButtonBoxAccepted();
    void onClearAvatar();
    void onSetAvatar();
    void onAvatarSelected();
    
private:
    QScopedPointer<Ui::VCard> m_ui;
    QSize avatarButtonMargins;
    QMenu* avatarMenu;
    bool editable;
    Shared::Avatar currentAvatarType;
    QString currentAvatarPath;
    
    static const std::set<QString> supportedTypes;
    
private:
    void updateAvatar();
};

#endif // VCARD_H
