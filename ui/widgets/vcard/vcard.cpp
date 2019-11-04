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

#include <algorithm>

const std::set<QString> VCard::supportedTypes = {"image/jpeg", "image/png"};

VCard::VCard(const QString& jid, bool edit, QWidget* parent):
    QWidget(parent),
    m_ui(new Ui::VCard()),
    avatarButtonMargins(),
    avatarMenu(nullptr),
    editable(edit),
    currentAvatarType(Shared::Avatar::empty),
    currentAvatarPath(""),
    progress(new Progress(100)),
    progressLabel(new QLabel()),
    overlay(new QWidget()),
    contextMenu(new QMenu()),
    emails(edit),
    roleDelegate(new ComboboxDelegate())
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
    
    roleDelegate->addEntry(tr(Shared::VCard::Email::roleNames[0].toStdString().c_str()));
    roleDelegate->addEntry(tr(Shared::VCard::Email::roleNames[1].toStdString().c_str()));
    roleDelegate->addEntry(tr(Shared::VCard::Email::roleNames[2].toStdString().c_str()));
    
    m_ui->emailsView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_ui->emailsView->setModel(&emails);
    m_ui->emailsView->setItemDelegateForColumn(1, roleDelegate);
    m_ui->emailsView->setColumnWidth(2, 30);
    m_ui->emailsView->horizontalHeader()->setStretchLastSection(false);
    m_ui->emailsView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    
    connect(m_ui->emailsView, &QWidget::customContextMenuRequested, this, &VCard::onContextMenu);
    
    if (edit) {
        avatarMenu = new QMenu();
        m_ui->avatarButton->setMenu(avatarMenu);
        avatarMenu->addAction(setAvatar);
        avatarMenu->addAction(clearAvatar);
        m_ui->title->setText(tr("Account %1 card").arg(jid));
    } else {
        m_ui->buttonBox->hide();
        m_ui->fullName->setReadOnly(true);
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
        m_ui->title->setText(tr("Contact %1 card").arg(jid));
    }
    
    connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, &VCard::onButtonBoxAccepted);
    connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &VCard::close);
    
    avatarButtonMargins = m_ui->avatarButton->size();
    
    int height = m_ui->personalForm->minimumSize().height() - avatarButtonMargins.height();
    m_ui->avatarButton->setIconSize(QSize(height, height));
    
    QGridLayout* gr = static_cast<QGridLayout*>(layout());
    gr->addWidget(overlay, 0, 0, 4, 1);
    QVBoxLayout* nl = new QVBoxLayout();
    QGraphicsOpacityEffect* opacity = new QGraphicsOpacityEffect();
    opacity->setOpacity(0.8);
    overlay->setLayout(nl);
    overlay->setBackgroundRole(QPalette::Base);
    overlay->setAutoFillBackground(true);
    overlay->setGraphicsEffect(opacity);
    progressLabel->setAlignment(Qt::AlignCenter);
    progressLabel->setStyleSheet("font: 16pt");
    nl->addStretch();
    nl->addWidget(progress);
    nl->addWidget(progressLabel);
    nl->addStretch();
    overlay->hide();
}

VCard::~VCard()
{
    if (editable) {
        avatarMenu->deleteLater();
    }
    
    roleDelegate->deleteLater();
    contextMenu->deleteLater();
}

void VCard::setVCard(const QString& jid, const Shared::VCard& card)
{
    m_ui->jabberID->setText(jid);
    setVCard(card);
}

void VCard::setVCard(const Shared::VCard& card)
{
    m_ui->fullName->setText(card.getFullName());
    m_ui->firstName->setText(card.getFirstName());
    m_ui->middleName->setText(card.getMiddleName());
    m_ui->lastName->setText(card.getLastName());
    m_ui->nickName->setText(card.getNickName());
    m_ui->birthday->setDate(card.getBirthday());
    m_ui->organizationName->setText(card.getOrgName());
    m_ui->organizationDepartment->setText(card.getOrgUnit());
    m_ui->organizationTitle->setText(card.getOrgTitle());
    m_ui->organizationRole->setText(card.getOrgRole());
    m_ui->description->setText(card.getDescription());
    m_ui->url->setText(card.getUrl());
    
    QDateTime receivingTime = card.getReceivingTime();
    m_ui->receivingTimeLabel->setText(tr("Received %1 at %2").arg(receivingTime.date().toString()).arg(receivingTime.time().toString()));
    currentAvatarType = card.getAvatarType();
    currentAvatarPath = card.getAvatarPath();
    
    updateAvatar();
    
    const std::deque<Shared::VCard::Email>& ems = card.getEmails();
    emails.setEmails(ems);
}

QString VCard::getJid() const
{
    return m_ui->jabberID->text();
}

void VCard::onButtonBoxAccepted()
{
    Shared::VCard card;
    card.setFullName(m_ui->fullName->text());
    card.setFirstName(m_ui->firstName->text());
    card.setMiddleName(m_ui->middleName->text());
    card.setLastName(m_ui->lastName->text());
    card.setNickName(m_ui->nickName->text());
    card.setBirthday(m_ui->birthday->date());
    card.setDescription(m_ui->description->toPlainText());
    card.setUrl(m_ui->url->text());
    card.setOrgName(m_ui->organizationName->text());
    card.setOrgUnit(m_ui->organizationDepartment->text());
    card.setOrgRole(m_ui->organizationRole->text());
    card.setOrgTitle(m_ui->organizationTitle->text());
    card.setAvatarPath(currentAvatarPath);
    card.setAvatarType(currentAvatarType);
    
    emails.getEmails(card.getEmails());
    
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

void VCard::showProgress(const QString& line)
{
    progressLabel->setText(line);
    overlay->show();
    progress->start();
}

void VCard::hideProgress()
{
    overlay->hide();
    progress->stop();
}

void VCard::onContextMenu(const QPoint& point)
{
    contextMenu->clear();
    bool hasMenu = false;
    QAbstractItemView* snd = static_cast<QAbstractItemView*>(sender());
    if (snd == m_ui->emailsView) {
        if (editable) {
            hasMenu = true;
            QAction* add = contextMenu->addAction(Shared::icon("list-add"), tr("Add email address"));
            connect(add, &QAction::triggered, this, &VCard::onAddEmail);
            
            QItemSelectionModel* sm = m_ui->emailsView->selectionModel();
            int selectionSize = sm->selectedRows().size();
            
            if (selectionSize > 0) {
                if (selectionSize == 1) {
                    int row = sm->selectedRows().at(0).row();
                    if (emails.isPreferred(row)) {
                        QAction* rev = contextMenu->addAction(Shared::icon("view-media-favorite"), tr("Unset this email as preferred"));
                        connect(rev, &QAction::triggered, std::bind(&UI::VCard::EMailsModel::revertPreferred, &emails, row));
                    } else {
                        QAction* rev = contextMenu->addAction(Shared::icon("favorite"), tr("Set this email as preferred"));
                        connect(rev, &QAction::triggered, std::bind(&UI::VCard::EMailsModel::revertPreferred, &emails, row));
                    }
                }
                
                QAction* del = contextMenu->addAction(Shared::icon("remove"), tr("Remove selected email addresses"));
                connect(del, &QAction::triggered, this, &VCard::onRemoveEmail);
            }
        }
    }
    
    if (hasMenu) {
        contextMenu->popup(snd->viewport()->mapToGlobal(point));
    }
}

void VCard::onAddEmail()
{
    QModelIndex index = emails.addNewEmptyLine();
    m_ui->emailsView->setCurrentIndex(index);
    m_ui->emailsView->edit(index);
}

void VCard::onAddAddress()
{
    
}
void VCard::onAddPhone()
{
}
void VCard::onRemoveAddress()
{
}
void VCard::onRemoveEmail()
{
    QItemSelection selection(m_ui->emailsView->selectionModel()->selection());
    
    QList<int> rows;
    for (const QModelIndex& index : selection.indexes()) {
        rows.append(index.row());
    }
    
    std::sort(rows.begin(), rows.end());
    
    int prev = -1;
    for (int i = rows.count() - 1; i >= 0; i -= 1) {
        int current = rows[i];
        if (current != prev) {
            emails.removeLines(current, 1);
            prev = current;
        }
    }
}

void VCard::onRemovePhone()
{
}
