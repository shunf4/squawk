/*
 * Squawk messenger. 
 * Copyright (C) 2019 Yury Gubich <blue@macaw.me>
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

#include "badge.h"

Badge::Badge(const QString& p_id, const QString& p_text, const QIcon& icon, QWidget* parent):
    QFrame(parent),
    id(p_id),
    image(new QLabel()),
    text(new QLabel(p_text)),
    closeButton(new QPushButton()),
    layout(new QHBoxLayout(this))
{
    setBackgroundRole(QPalette::Base);
    //setAutoFillBackground(true);
    setFrameStyle(QFrame::StyledPanel);
    setFrameShadow(QFrame::Raised);
    
    image->setPixmap(icon.pixmap(25, 25));
    closeButton->setIcon(QIcon::fromTheme("tab-close"));
    QIcon qIcon;
    qIcon.addFile(QString::fromUtf8(":/images/fallback/dark/big/edit-none.svg"), QSize(), QIcon::Normal, QIcon::Off);
    closeButton->setIcon(qIcon);
    closeButton->setMaximumHeight(25);
    closeButton->setMaximumWidth(25);
    
    layout->addWidget(image);
    layout->addWidget(text);
    layout->addWidget(closeButton);
    
    layout->setContentsMargins(2, 2, 2, 2);
    
    connect(closeButton, &QPushButton::clicked, this, &Badge::close);
}

Badge::~Badge()
{
}

bool Badge::Comparator::operator()(const Badge* a, const Badge* b) const
{
    return a->id < b->id;
}

bool Badge::Comparator::operator()(const Badge& a, const Badge& b) const
{
    return a.id < b.id;
}
