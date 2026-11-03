#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <functional>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

#include "block_vector.h"

template<typename T, size_t block_size = 256>
class linked_list
{
    struct node
    {
        node* m_prev = nullptr;          // pointer to the previous block in the list, or nullptr if this is the first block
        node* m_next = nullptr;          // pointer to the next block in the list, or nullptr if this is the last block
        T m_data;                        // data in this node
    };

    node* m_first = nullptr;
    node* m_last = nullptr;

    block_vector<node, block_size> m_allocated_nodes; // memory pool of allocated nodes
    node* m_free_nodes = nullptr; // singly-linked list of nodes available for reuse

    node * allocate()
    {
        if(m_free_nodes)
        {
            node * n = m_free_nodes;
            m_free_nodes = n->m_next;
            return n;
        }
        else
        {
            m_allocated_nodes.push_back(node());
            return &m_allocated_nodes.back();
        }
    }
    void deallocate(node * n)
    {
        n->m_prev = nullptr;
        n->m_next = m_free_nodes;
        m_free_nodes = n;
    }

public:
    template<typename ITERATOR, bool IS_CONST, bool IS_REVERSE = false>
    struct iterator_impl
    {
        using node_pointer = typename std::conditional<IS_CONST, const node*, node*>::type;
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using reference = typename std::conditional<IS_CONST, const T&, T&>::type;
        using pointer = typename std::conditional<IS_CONST, const T*, T*>::type;

        struct Hash
        {
            std::size_t operator()(const ITERATOR & iterator) const noexcept
            {
                return std::hash<node_pointer>()(iterator.m_node);
            }
        };

        iterator_impl(node_pointer n) : m_node(n) {}
        reference operator*() const { return m_node->m_data; }
        pointer operator->() const { return &m_node->m_data; }
        ITERATOR& operator++()
        {
            m_node = IS_REVERSE ? m_node->m_prev : m_node->m_next;
            return static_cast<ITERATOR&>(*this);
        }
        ITERATOR operator++(int)
        {
            ITERATOR previous = static_cast<ITERATOR&>(*this);
            ++(*this);
            return previous;
        }
        ITERATOR& operator--()
        {
            m_node = IS_REVERSE ? m_node->m_next : m_node->m_prev;
            return static_cast<ITERATOR&>(*this);
        }
        ITERATOR operator--(int)
        {
            ITERATOR previous = static_cast<ITERATOR&>(*this);
            --(*this);
            return previous;
        }
        bool operator==(const ITERATOR& other) const
        {
            if(m_node == other.m_node)
            {
                return true;
            }
            return false;
        }
        bool operator!=(const ITERATOR& other) const 
        { 
            return !(*this == other); 
        }

    private:
        friend class linked_list;
        node_pointer m_node;
    };

    class iterator : public iterator_impl<iterator, false>
    {
    public:
        using iterator_impl<iterator, false>::iterator_impl;
    };

    class const_iterator : public iterator_impl<const_iterator, true>
    {
    public:
        using iterator_impl<const_iterator, true>::iterator_impl;
    };

    class reverse_iterator : public iterator_impl<reverse_iterator, false, true>
    {
    public:
        using iterator_impl<reverse_iterator, false, true>::iterator_impl;
    };

    class const_reverse_iterator : public iterator_impl<const_reverse_iterator, true, true>
    {
    public:
        using iterator_impl<const_reverse_iterator, true, true>::iterator_impl;
    };

    const_iterator cbegin() const;
    const_iterator cend() const;
    const_iterator begin() const;
    const_iterator end() const;
    iterator begin();
    iterator end();
    reverse_iterator rbegin();
    reverse_iterator rend();
    const_reverse_iterator rbegin() const;
    const_reverse_iterator rend() const;
    const_reverse_iterator crbegin() const;
    const_reverse_iterator crend() const;
    const_reverse_iterator rcbegin() const;
    const_reverse_iterator rcend() const;

    iterator last() { return iterator(m_last); }
    const_iterator last() const { return const_iterator(m_last); }

    linked_list() = default;
    linked_list(const linked_list& other);
    linked_list& operator=(const linked_list& other);
    linked_list(linked_list&& other) noexcept;
    linked_list& operator=(linked_list&& other) noexcept;
    ~linked_list();

    void clear();

    bool empty() const;

    // insert a value as the last value
    void push_back(const T& value);

    // erase one value at position and return the iterator to the next value
    iterator erase(iterator position);
    reverse_iterator erase(reverse_iterator position);

    // insert one value at position, replacing two values, return iterator to inserted value
    iterator replace_two(iterator position, const T & value);
    reverse_iterator replace_two(reverse_iterator position, const T & value);

private:
};

template<typename T, size_t block_size>
typename linked_list<T, block_size>::iterator linked_list<T, block_size>::begin()
{
    return iterator(m_first);
}

template<typename T, size_t block_size>
typename linked_list<T, block_size>::iterator linked_list<T, block_size>::end()
{
    return iterator(nullptr);
}

template<typename T, size_t block_size>
typename linked_list<T, block_size>::const_iterator linked_list<T, block_size>::cbegin() const
{
    return const_iterator(m_first);
}

template<typename T, size_t block_size>
typename linked_list<T, block_size>::const_iterator linked_list<T, block_size>::cend() const
{
    return const_iterator(nullptr);
}

template<typename T, size_t block_size>
typename linked_list<T, block_size>::const_iterator linked_list<T, block_size>::begin() const
{
    return const_iterator(m_first);
}

template<typename T, size_t block_size>
typename linked_list<T, block_size>::const_iterator linked_list<T, block_size>::end() const
{
    return const_iterator(nullptr);
}

template<typename T, size_t block_size>
typename linked_list<T, block_size>::reverse_iterator linked_list<T, block_size>::rbegin()
{
    return reverse_iterator(m_last);
}

template<typename T, size_t block_size>
typename linked_list<T, block_size>::reverse_iterator linked_list<T, block_size>::rend()
{
    return reverse_iterator(nullptr);
}

template<typename T, size_t block_size>
typename linked_list<T, block_size>::const_reverse_iterator linked_list<T, block_size>::rbegin() const
{
    return const_reverse_iterator(m_last);
}

template<typename T, size_t block_size>
typename linked_list<T, block_size>::const_reverse_iterator linked_list<T, block_size>::rend() const
{
    return const_reverse_iterator(nullptr);
}

template<typename T, size_t block_size>
typename linked_list<T, block_size>::const_reverse_iterator linked_list<T, block_size>::crbegin() const
{
    return const_reverse_iterator(m_last);
}

template<typename T, size_t block_size>
typename linked_list<T, block_size>::const_reverse_iterator linked_list<T, block_size>::crend() const
{
    return const_reverse_iterator(nullptr);
}

template<typename T, size_t block_size>
typename linked_list<T, block_size>::const_reverse_iterator linked_list<T, block_size>::rcbegin() const
{
    return crbegin();
}

template<typename T, size_t block_size>
typename linked_list<T, block_size>::const_reverse_iterator linked_list<T, block_size>::rcend() const
{
    return crend();
}

template<typename T, size_t block_size>
linked_list<T, block_size>::~linked_list()
{
    clear();
}

template<typename T, size_t block_size>
bool linked_list<T, block_size>::empty() const
{
    return m_first == nullptr;
}

template<typename T, size_t block_size>
linked_list<T, block_size>::linked_list(const linked_list& other)
{
    for(const T& value : other)
    {
        push_back(value);
    }
}

template<typename T, size_t block_size>
linked_list<T, block_size>& linked_list<T, block_size>::operator=(const linked_list& other)
{
    if(this != &other)
    {
        linked_list copy(other);
        *this = std::move(copy);
    }
    return *this;
}

template<typename T, size_t block_size>
linked_list<T, block_size>::linked_list(linked_list&& other) noexcept
    : m_first(other.m_first),
      m_last(other.m_last),
      m_allocated_nodes(std::move(other.m_allocated_nodes)),
      m_free_nodes(other.m_free_nodes)
{
    other.m_first = nullptr;
    other.m_last = nullptr;
    other.m_free_nodes = nullptr;
}

template<typename T, size_t block_size>
linked_list<T, block_size>& linked_list<T, block_size>::operator=(linked_list&& other) noexcept
{
    if(this != &other)
    {
        clear();
        m_allocated_nodes = std::move(other.m_allocated_nodes);
        m_first = other.m_first;
        m_last = other.m_last;
        m_free_nodes = other.m_free_nodes;
        other.m_first = nullptr;
        other.m_last = nullptr;
        other.m_free_nodes = nullptr;
    }
    return *this;
}

template<typename T, size_t block_size>
void linked_list<T, block_size>::clear()
{
    m_first = nullptr;
    m_last = nullptr;
    m_allocated_nodes.clear();
    m_free_nodes = nullptr;
}

template<typename T, size_t block_size>
typename linked_list<T, block_size>::iterator linked_list<T, block_size>::erase(iterator position)
{
    assert(position.m_node != nullptr);
    node * current = position.m_node;
    ++position;
    if(current->m_prev)
    {
        current->m_prev->m_next = current->m_next;
    }
    else
    {
        m_first = current->m_next;
    }
    if(current->m_next)
    {
        current->m_next->m_prev = current->m_prev;
    }
    else
    {
        m_last = current->m_prev;
    }
    deallocate(current);
    return position;
}

template<typename T, size_t block_size>
typename linked_list<T, block_size>::reverse_iterator linked_list<T, block_size>::erase(reverse_iterator position)
{
    assert(position.m_node != nullptr);
    node * current = position.m_node;
    ++position;
    if(current->m_prev)
    {
        current->m_prev->m_next = current->m_next;
    }
    else
    {
        m_first = current->m_next;
    }
    if(current->m_next)
    {
        current->m_next->m_prev = current->m_prev;
    }
    else
    {
        m_last = current->m_prev;
    }
    deallocate(current);
    return position;
}

template<typename T, size_t block_size>
typename linked_list<T, block_size>::iterator linked_list<T, block_size>::replace_two(iterator position, const T & value)
{
    assert(position.m_node != nullptr);
    assert(position.m_node->m_next != nullptr);
    position = erase(position);
    *position = value;
    return position;
}

template<typename T, size_t block_size>
typename linked_list<T, block_size>::reverse_iterator linked_list<T, block_size>::replace_two(reverse_iterator position, const T & value)
{
    assert(position.m_node != nullptr);
    assert(position.m_node->m_prev != nullptr);
    position = erase(position);
    *position = value;
    return position;
}

template<typename T, size_t block_size>
void linked_list<T, block_size>::push_back(const T& value)
{
    if(!m_last)
    {
        node * n = allocate();
        n->m_data = value;
        n->m_prev = nullptr;
        n->m_next = nullptr;
        m_last = n;
        m_first = n;
    }
    else
    {
        node * n = allocate();
        n->m_data = value;
        n->m_prev = m_last;
        n->m_next = nullptr;
        m_last->m_next = n;
        m_last = n;
    }
}
