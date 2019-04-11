/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2019  Юрий Губич <y.gubich@initi.ru>
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

#include "messageline.h"

MessageLine::MessageLine(QWidget* parent):
    QWidget(parent),
    messageIndex(),
    messageOrder(),
    layout(new QVBoxLayout())
{
    setLayout(layout);
    setBackgroundRole(QPalette::Base);
}

MessageLine::~MessageLine()
{
}

void MessageLine::message(const Shared::Message& msg)
{
    QVBoxLayout* vBox = new QVBoxLayout();
    QHBoxLayout* hBox = new QHBoxLayout();
    QWidget* message = new QWidget();
    message->setLayout(vBox);
    
    QLabel* body = new QLabel(msg.getBody());
    QLabel* sender = new QLabel(msg.getFrom());
    
    vBox->addWidget(body);
    vBox->addWidget(sender);
    
    hBox->addStretch();
    hBox->addWidget(message);
    
    layout->addItem(hBox);
}

