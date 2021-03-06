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

#ifndef ORDER_H
#define ORDER_H

#include <map>
#include <list>

#include "exception.h"

namespace W 
{
    template <typename data_type, typename comparator = std::less<data_type>>
    class Order
    {
    public:
        class Duplicates: 
            public Utils::Exception
        {
        public:
            Duplicates():Exception(){}
            
            std::string getMessage() const{return "Inserting element duplicates existing";}
        };
        
        class NotFound: 
            public Utils::Exception
        {
        public:
            NotFound():Exception(){}
            
            std::string getMessage() const{return "Erasing element haven't been found";}
        };
        
    protected:
        typedef std::list<data_type> List;
        
    public:
        typedef typename List::size_type size_type;
        typedef typename List::const_iterator const_iterator;
        typedef typename List::iterator iterator;
        
    protected:
        typedef std::map<data_type, const_iterator, comparator> Map;
        typedef typename Map::const_iterator m_const_itr;
        typedef typename Map::iterator m_itr;
    
    public:
        Order():
            order(),
            r_map() 
        {}
        ~Order() {};
        
        size_type size() const {
            return order.size();
        }
        
        void push_back(data_type element) {
            m_const_itr m_itr = r_map.find(element);
            if (m_itr != r_map.end()) {
                throw Duplicates();
            }
            
            const_iterator itr = order.insert(order.end(), element);
            r_map.insert(std::make_pair(element, itr));
        }
        
        void erase(data_type element) {
            m_const_itr itr = r_map.find(element);
            if (itr == r_map.end()) {
                throw NotFound();
            }
            order.erase(itr->second);
            r_map.erase(itr);
            
        }
        
        void clear() {
            order.clear();
            r_map.clear();
        }
        
        void insert(const_iterator pos, data_type element) {
            m_const_itr m_itr = r_map.find(element);
            if (m_itr != r_map.end()) {
                throw Duplicates();
            }
            
            const_iterator itr = order.insert(pos, element);
            r_map.insert(std::make_pair(element, itr));
        }
        
        void insert(iterator pos, data_type element) {
            m_const_itr m_itr = r_map.find(element);
            if (m_itr != r_map.end()) {
                throw Duplicates();
            }
            
            const_iterator itr = order.insert(pos, element);
            r_map.insert(std::make_pair(element, itr));
        }
        
        const_iterator find(data_type element) const {
            m_const_itr itr = r_map.find(element);
            
            if (itr == r_map.end()) {
                return end();
            } else {
                return itr->second;
            }
        }
        
        const_iterator begin() const {
            return order.begin();
        }
        
        const_iterator end() const {
            return order.end();
        }
        
        iterator begin() {
            return order.begin();
        }
        
        iterator end() {
            return order.end();
        }
        
    private:
        List order;
        Map r_map;
    };
}



#endif // ORDER_H
