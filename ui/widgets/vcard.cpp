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

#include "vcard.h"
#include "ui_vcard.h"

#include <QDebug>

const std::set<QString> VCard::supportedTypes = {"image/jpeg", "image/png"};

VCard::VCard(const QString& jid, bool edit, QWidget* parent):
    QWidget(parent),
    m_ui(new Ui::VCard()),
    avatarButtonMargins(),
    avatarMenu(nullptr),
    editable(edit),
    currentAvatarType(Shared::Avatar::empty),
    currentAvatarPath("")
{
    m_ui->setupUi(this);
    m_ui->jabberID->setText(jid);
    m_ui->jabberID->setReadOnly(true);
    
    QAction* setAvatar = m_ui->actionSetAvatar;
    QAction* clearAvatar = m_ui->actionClearAvatar;
    
    connect(setAvatar, &QAction::triggered, this, &VCard::onSetAvatar);
    connect(clearAvatar, &QAction::triggered, this, &VCard::onClearAvatar);
    
    setAvatar->setEnabled(true);
    clearAvatar->setEnabled(false);
    
    if (edit) {
        avatarMenu = new QMenu();
        m_ui->avatarButton->setMenu(avatarMenu);
        avatarMenu->addAction(setAvatar);
        avatarMenu->addAction(clearAvatar);
    } else {
        m_ui->buttonBox->hide();
        m_ui->firstName->setReadOnly(true);
        m_ui->middleName->setReadOnly(true);
        m_ui->lastName->setReadOnly(true);
        m_ui->nickName->setReadOnly(true);
        m_ui->birthday->setReadOnly(true);
        m_ui->organizationName->setReadOnly(true);
        m_ui->organizationDepartment->setReadOnly(true);
        m_ui->organizationTitle->setReadOnly(true);
        m_ui->organizationRole->setReadOnly(true);
        m_ui->description->setReadOnly(true);
        m_ui->url->setReadOnly(true);
    }
    
    connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, &VCard::onButtonBoxAccepted);
    connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &VCard::close);
    
    avatarButtonMargins = m_ui->avatarButton->size();
    
    int height = m_ui->personalForm->minimumSize().height() - avatarButtonMargins.height();
    m_ui->avatarButton->setIconSize(QSize(height, height));
}

VCard::~VCard()
{
    if (editable) {
        avatarMenu->deleteLater();
    }
}

void VCard::setVCard(const QString& jid, const Shared::VCard& card)
{
    m_ui->jabberID->setText(jid);
    setVCard(card);
}

void VCard::setVCard(const Shared::VCard& card)
{
    m_ui->firstName->setText(card.getFirstName());
    m_ui->middleName->setText(card.getMiddleName());
    m_ui->lastName->setText(card.getLastName());
    m_ui->nickName->setText(card.getNickName());
    m_ui->birthday->setDate(card.getBirthday());
    //m_ui->organizationName->setText(card.get());
    //m_ui->organizationDepartment->setText(card.get());
    //m_ui->organizationTitle->setText(card.get());
    //m_ui->organizationRole->setText(card.get());
    m_ui->description->setText(card.getDescription());
    currentAvatarType = card.getAvatarType();
    currentAvatarPath = card.getAvatarPath();
    
    updateAvatar();
}

QString VCard::getJid() const
{
    return m_ui->jabberID->text();
}

void VCard::onButtonBoxAccepted()
{
    Shared::VCard card;
    card.setFirstName(m_ui->firstName->text());
    card.setMiddleName(m_ui->middleName->text());
    card.setLastName(m_ui->lastName->text());
    card.setNickName(m_ui->nickName->text());
    card.setBirthday(m_ui->birthday->date());
    card.setDescription(m_ui->description->toPlainText());
    card.setAvatarPath(currentAvatarPath);
    card.setAvatarType(currentAvatarType);
    
    emit saveVCard(card);
}

void VCard::onClearAvatar()
{
    currentAvatarType = Shared::Avatar::empty;
    currentAvatarPath = "";
    
    updateAvatar();
}

void VCard::onSetAvatar()
{
    QFileDialog* d = new QFileDialog(this, tr("Chose your new avatar"));
    d->setFileMode(QFileDialog::ExistingFile);
    d->setNameFilter(tr("Images (*.png *.jpg *.jpeg)"));
    
    connect(d, &QFileDialog::accepted, this, &VCard::onAvatarSelected);
    connect(d, &QFileDialog::rejected, d, &QFileDialog::deleteLater);
    
    d->show();
}

void VCard::updateAvatar()
{
    int height = m_ui->personalForm->minimumSize().height() - avatarButtonMargins.height();
    switch (currentAvatarType) {
        case Shared::Avatar::empty:
            m_ui->avatarButton->setIcon(Shared::icon("user", true));
            m_ui->avatarButton->setIconSize(QSize(height, height));
            m_ui->actionClearAvatar->setEnabled(false);
            break;
        case Shared::Avatar::autocreated:
        case Shared::Avatar::valid:
            QPixmap pixmap(currentAvatarPath);
            qreal h = pixmap.height();
            qreal w = pixmap.width();
            qreal aspectRatio = w / h;
            m_ui->avatarButton->setIconSize(QSize(height * aspectRatio, height));
            m_ui->avatarButton->setIcon(QIcon(currentAvatarPath));
            m_ui->actionClearAvatar->setEnabled(true);
            break;
    }
}

void VCard::onAvatarSelected()
{
    QFileDialog* d = static_cast<QFileDialog*>(sender());
    QMimeDatabase db;
    QString path = d->selectedFiles().front();
    QMimeType type = db.mimeTypeForFile(path);
    d->deleteLater();
    
    if (supportedTypes.find(type.name()) == supportedTypes.end()) {
        qDebug() << "Selected for avatar file is not supported, skipping";
    } else {
        QImage src(path);
        QImage dst;
        if (src.width() > 160 || src.height() > 160) {
            dst = src.scaled(160, 160, Qt::KeepAspectRatio);
        }
        QString path = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/" + m_ui->jabberID->text() + ".temp." + type.preferredSuffix();
        QFile oldTemp(path);
        if (oldTemp.exists()) {
            if (!oldTemp.remove()) {
                qDebug() << "Error removing old temp avatar" << path;
                return;
            }
        }
        bool success = dst.save(path);
        if (success) {
            currentAvatarPath = path;
            currentAvatarType = Shared::Avatar::valid;
            
            updateAvatar();
        } else {
            qDebug() << "couldn't save avatar" << path << ", skipping";
        }
    }
}
