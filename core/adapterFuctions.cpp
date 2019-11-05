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
#ifndef CORE_ADAPTER_FUNCTIONS_H
#define CORE_ADAPTER_FUNCTIONS_H

#include "account.h"

void Core::initializeVCard(Shared::VCard& vCard, const QXmppVCardIq& card)
{
    vCard.setFullName(card.fullName());
    vCard.setFirstName(card.firstName());
    vCard.setMiddleName(card.middleName());
    vCard.setLastName(card.lastName());
    vCard.setBirthday(card.birthday());
    vCard.setNickName(card.nickName());
    vCard.setDescription(card.description());
    vCard.setUrl(card.url());
    QXmppVCardOrganization org = card.organization();
    vCard.setOrgName(org.organization());
    vCard.setOrgRole(org.role());
    vCard.setOrgUnit(org.unit());
    vCard.setOrgTitle(org.title());
    
    QList<QXmppVCardEmail> emails = card.emails();
    std::deque<Shared::VCard::Email>& myEmails = vCard.getEmails();
    for (const QXmppVCardEmail& em : emails) {
        QXmppVCardEmail::Type et = em.type();
        bool prefered = false;
        bool accounted = false;
        if (et & QXmppVCardEmail::Preferred) {
            prefered = true;
        }
        if (et & QXmppVCardEmail::Home) {
            myEmails.emplace_back(em.address(), Shared::VCard::Email::home, prefered);
            accounted = true;
        } 
        if (et & QXmppVCardEmail::Work) {
            myEmails.emplace_back(em.address(), Shared::VCard::Email::work, prefered);
            accounted = true;
        }
        if (!accounted) {
            myEmails.emplace_back(em.address(), Shared::VCard::Email::none, prefered);
        }
        
    }
    
    QList<QXmppVCardPhone> phones = card.phones();
    std::deque<Shared::VCard::Phone>& myPhones = vCard.getPhones();
    for (const QXmppVCardPhone& ph : phones) {
        Shared::VCard::Phone mPh(ph.number());
        QXmppVCardPhone::Type pt = ph.type();
        bool prefered = false;
        bool accounted = false;
        if (pt & QXmppVCardPhone::Preferred) {
            prefered = true;
        }
        
        bool home = false;
        bool work = false;
        
        if (pt & QXmppVCardPhone::Home) {
            home = true;
        }
        if (pt & QXmppVCardPhone::Work) {
            work = true;
        }
        
        
        if (pt & QXmppVCardPhone::Fax) {
            if (home || work) {
                if (home) {
                    myPhones.emplace_back(ph.number(), Shared::VCard::Phone::fax, Shared::VCard::Phone::home, prefered);
                }
                if (work) {
                    myPhones.emplace_back(ph.number(), Shared::VCard::Phone::fax, Shared::VCard::Phone::work, prefered);
                }
            } else {
                myPhones.emplace_back(ph.number(), Shared::VCard::Phone::fax, Shared::VCard::Phone::none, prefered);
            }
            accounted = true;
        } 
        if (pt & QXmppVCardPhone::Voice) {
            if (home || work) {
                if (home) {
                    myPhones.emplace_back(ph.number(), Shared::VCard::Phone::voice, Shared::VCard::Phone::home, prefered);
                }
                if (work) {
                    myPhones.emplace_back(ph.number(), Shared::VCard::Phone::voice, Shared::VCard::Phone::work, prefered);
                }
            } else {
                myPhones.emplace_back(ph.number(), Shared::VCard::Phone::voice, Shared::VCard::Phone::none, prefered);
            }
            accounted = true;
        } 
        if (pt & QXmppVCardPhone::Pager) {
            if (home || work) {
                if (home) {
                    myPhones.emplace_back(ph.number(), Shared::VCard::Phone::pager, Shared::VCard::Phone::home, prefered);
                }
                if (work) {
                    myPhones.emplace_back(ph.number(), Shared::VCard::Phone::pager, Shared::VCard::Phone::work, prefered);
                }
            } else {
                myPhones.emplace_back(ph.number(), Shared::VCard::Phone::pager, Shared::VCard::Phone::none, prefered);
            }
            accounted = true;
        } 
        if (pt & QXmppVCardPhone::Cell) {
            if (home || work) {
                if (home) {
                    myPhones.emplace_back(ph.number(), Shared::VCard::Phone::cell, Shared::VCard::Phone::home, prefered);
                }
                if (work) {
                    myPhones.emplace_back(ph.number(), Shared::VCard::Phone::cell, Shared::VCard::Phone::work, prefered);
                }
            } else {
                myPhones.emplace_back(ph.number(), Shared::VCard::Phone::cell, Shared::VCard::Phone::none, prefered);
            }
            accounted = true;
        } 
        if (pt & QXmppVCardPhone::Video) {
            if (home || work) {
                if (home) {
                    myPhones.emplace_back(ph.number(), Shared::VCard::Phone::video, Shared::VCard::Phone::home, prefered);
                }
                if (work) {
                    myPhones.emplace_back(ph.number(), Shared::VCard::Phone::video, Shared::VCard::Phone::work, prefered);
                }
            } else {
                myPhones.emplace_back(ph.number(), Shared::VCard::Phone::video, Shared::VCard::Phone::none, prefered);
            }
            accounted = true;
        } 
        if (pt & QXmppVCardPhone::Modem) {
            if (home || work) {
                if (home) {
                    myPhones.emplace_back(ph.number(), Shared::VCard::Phone::modem, Shared::VCard::Phone::home, prefered);
                }
                if (work) {
                    myPhones.emplace_back(ph.number(), Shared::VCard::Phone::modem, Shared::VCard::Phone::work, prefered);
                }
            } else {
                myPhones.emplace_back(ph.number(), Shared::VCard::Phone::modem, Shared::VCard::Phone::none, prefered);
            }
            accounted = true;
        } 
        if (!accounted) {
            if (home || work) {
                if (home) {
                    myPhones.emplace_back(ph.number(), Shared::VCard::Phone::other, Shared::VCard::Phone::home, prefered);
                }
                if (work) {
                    myPhones.emplace_back(ph.number(), Shared::VCard::Phone::other, Shared::VCard::Phone::work, prefered);
                }
            } else {
                myPhones.emplace_back(ph.number(), Shared::VCard::Phone::other, Shared::VCard::Phone::none, prefered);
            }
        }
    }
}

void Core::initializeQXmppVCard(QXmppVCardIq& iq, const Shared::VCard& card) {
    iq.setFullName(card.getFullName());
    iq.setFirstName(card.getFirstName());
    iq.setMiddleName(card.getMiddleName());
    iq.setLastName(card.getLastName());
    iq.setNickName(card.getNickName());
    iq.setBirthday(card.getBirthday());
    iq.setDescription(card.getDescription());
    iq.setUrl(card.getUrl());
    QXmppVCardOrganization org;
    org.setOrganization(card.getOrgName());
    org.setUnit(card.getOrgUnit());
    org.setRole(card.getOrgRole());
    org.setTitle(card.getOrgTitle());
    iq.setOrganization(org);
    
    const std::deque<Shared::VCard::Email>& myEmails = card.getEmails();
    QList<QXmppVCardEmail> emails;
    for (const Shared::VCard::Email& mEm : myEmails) {
        QXmppVCardEmail em;
        QXmppVCardEmail::Type t = QXmppVCardEmail::Internet;
        if (mEm.prefered) {
            t = t | QXmppVCardEmail::Preferred;
        }
        if (mEm.role == Shared::VCard::Email::home) {
            t = t | QXmppVCardEmail::Home;
        } else if (mEm.role == Shared::VCard::Email::work) {
            t = t | QXmppVCardEmail::Work;
        }
        em.setType(t);
        em.setAddress(mEm.address);
        
        emails.push_back(em);
    }
    
    std::map<QString, QXmppVCardPhone> phones;
    QList<QXmppVCardPhone> phs;
    const std::deque<Shared::VCard::Phone>& myPhones = card.getPhones();
    for (const Shared::VCard::Phone& mPh : myPhones) {
        std::map<QString, QXmppVCardPhone>::iterator itr = phones.find(mPh.number);
        if (itr == phones.end()) {
            itr = phones.emplace(mPh.number, QXmppVCardPhone()).first;
        }
        QXmppVCardPhone& phone = itr->second;
        
        switch (mPh.type) {
            case Shared::VCard::Phone::fax:
                phone.setType(phone.type() | QXmppVCardPhone::Fax);
                break;
            case Shared::VCard::Phone::pager:
                phone.setType(phone.type() | QXmppVCardPhone::Pager);
                break;
            case Shared::VCard::Phone::voice:
                phone.setType(phone.type() | QXmppVCardPhone::Voice);
                break;
            case Shared::VCard::Phone::cell:
                phone.setType(phone.type() | QXmppVCardPhone::Cell);
                break;
            case Shared::VCard::Phone::video:
                phone.setType(phone.type() | QXmppVCardPhone::Video);
                break;
            case Shared::VCard::Phone::modem:
                phone.setType(phone.type() | QXmppVCardPhone::Modem);
                break;
            case Shared::VCard::Phone::other:
                phone.setType(phone.type() | QXmppVCardPhone::PCS);     //loss of information, but I don't even know what the heck is this type of phone!
                break;
        }
        
        switch (mPh.role) {
            case Shared::VCard::Phone::home:
                phone.setType(phone.type() | QXmppVCardPhone::Home);
                break;
            case Shared::VCard::Phone::work:
                phone.setType(phone.type() | QXmppVCardPhone::Work);
                break;
            default:
                break;
        }
        
        if (mPh.prefered) {
            phone.setType(phone.type() | QXmppVCardPhone::Preferred);
        }
    }
    for (const std::pair<QString, QXmppVCardPhone>& phone : phones) {
        phs.push_back(phone.second);
    }
    
    iq.setEmails(emails);
    iq.setPhones(phs);
}

#endif // CORE_ADAPTER_FUNCTIONS_H
