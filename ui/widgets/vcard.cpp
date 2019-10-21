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

VCard::VCard(bool edit, QWidget* parent):
    QWidget(parent),
    m_ui(new Ui::VCard()),
    avatar(":/images/logo.svg", 64)
{
    m_ui->setupUi(this);
    
    if (edit) {
        
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
        m_ui->jabberID->setReadOnly(true);
    }
    
    connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, &VCard::onButtonBoxAccepted);
    connect(m_ui->buttonBox, &QDialogButtonBox::rejected, m_ui->buttonBox, &QDialogButtonBox::deleteLater);
}

VCard::~VCard()
{
}

void VCard::setVCard(const QString& jid, const Shared::VCard& card)
{
    m_ui->jabberID->setText(jid);
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
    
    QString path;
    switch (card.getAvatarType()) {
        case Shared::Avatar::empty:
            path = QApplication::palette().window().color().lightnessF() > 0.5 ? ":/images/fallback/dark/big/user.svg" : ":/images/fallback/light/big/user.svg";
            break;
        case Shared::Avatar::autocreated:
        case Shared::Avatar::valid:
            path = card.getAvatarPath();
            break;
    }
    avatar.setPath(path);
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
    
    emit saveVCard(m_ui->jabberID->text(), card);
}
