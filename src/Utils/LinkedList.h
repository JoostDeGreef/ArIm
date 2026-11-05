#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <functional>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

#include "BlockVector.h"

// Note:
//   This 'optimal block_size' was determined by running 
//   "make release run_linked_list_benchmark"
template<typename T, size_t BlockSize = 512>
class LinkedList
{
    struct Node
    {
        Node* m_prev = nullptr;          // pointer to the previous block in the list, or nullptr if this is the first block
        Node* m_next = nullptr;          // pointer to the next block in the list, or nullptr if this is the last block
        T m_data;                        // data in this node
    };

    Node* m_first = nullptr;
    Node* m_last = nullptr;

    BlockVector<Node, BlockSize> m_allocatedNodes; // memory pool of allocated nodes
    Node* m_freeNodes = nullptr; // singly-linked list of nodes available for reuse

    Node * Allocate()
    {
        if(m_freeNodes)
        {
            Node * n = m_freeNodes;
            m_freeNodes = n->m_next;
            return n;
        }
        else
        {
            m_allocatedNodes.PushBack(Node());
            return &m_allocatedNodes.Back();
        }
    }
    void DeAllocate(Node * n)
    {
        n->m_prev = nullptr;
        n->m_next = m_freeNodes;
        m_freeNodes = n;
    }

public:
    template<typename Iterator, bool IsConst, bool IsReverse = false>
    struct IteratorImpl
    {
        using NodePointer = typename std::conditional<IsConst, const Node*, Node*>::type;
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using reference = typename std::conditional<IsConst, const T&, T&>::type;
        using pointer = typename std::conditional<IsConst, const T*, T*>::type;

        struct Hash
        {
            std::size_t operator()(const Iterator& iterator) const noexcept
            {
                return std::hash<NodePointer>()(iterator.m_node);
            }
        };

        IteratorImpl(NodePointer n) : m_node(n) {}
        reference operator*() const { return m_node->m_data; }
        pointer operator->() const { return &m_node->m_data; }
        Iterator& operator++()
        {
            m_node = IsReverse ? m_node->m_prev : m_node->m_next;
            return static_cast<Iterator&>(*this);
        }
        Iterator operator++(int)
        {
            Iterator previous = static_cast<Iterator&>(*this);
            ++(*this);
            return previous;
        }
        Iterator& operator--()
        {
            m_node = IsReverse ? m_node->m_next : m_node->m_prev;
            return static_cast<Iterator&>(*this);
        }
        Iterator operator--(int)
        {
            Iterator previous = static_cast<Iterator&>(*this);
            --(*this);
            return previous;
        }
        bool operator==(const Iterator& other) const
        {
            if(m_node == other.m_node)
            {
                return true;
            }
            return false;
        }
        bool operator!=(const Iterator& other) const 
        { 
            return !(*this == other); 
        }

    private:
        friend class LinkedList;
        NodePointer m_node;
    };

    class Iterator : public IteratorImpl<Iterator, false>
    {
    public:
        using IteratorImpl<Iterator, false>::IteratorImpl;
    };

    class ConstIterator : public IteratorImpl<ConstIterator, true>
    {
    public:
        using IteratorImpl<ConstIterator, true>::IteratorImpl;
    };

    class ReverseIterator : public IteratorImpl<ReverseIterator, false, true>
    {
    public:
        using IteratorImpl<ReverseIterator, false, true>::IteratorImpl;
    };

    class ConstReverseIterator : public IteratorImpl<ConstReverseIterator, true, true>
    {
    public:
        using IteratorImpl<ConstReverseIterator, true, true>::IteratorImpl;
    };

    ConstIterator begin() const;
    ConstIterator end() const;
    Iterator begin();
    Iterator end();
    ReverseIterator rbegin();
    ReverseIterator rend();
    ConstReverseIterator rbegin() const;
    ConstReverseIterator rend() const;
    ConstReverseIterator crbegin() const;
    ConstReverseIterator crend() const;

    Iterator Last() { return Iterator(m_last); }
    ConstIterator Last() const { return ConstIterator(m_last); }

    LinkedList() = default;
    LinkedList(const LinkedList& other);
    LinkedList& operator=(const LinkedList& other);
    LinkedList(LinkedList&& other) noexcept;
    LinkedList& operator=(LinkedList&& other) noexcept;
    ~LinkedList();

    void Clear();

    bool Empty() const;

    // insert a value as the last value
    void PushBack(const T& value);

    // construct a value as the last value
    template<typename... Args>
    T& EmplaceBack(Args&&... args);

    // erase one value at position and return the iterator to the next value
    Iterator Erase(Iterator position);
    ReverseIterator Erase(ReverseIterator position);

    // insert one value at position, replacing two values, return iterator to inserted value
    Iterator ReplaceTwo(Iterator position, const T & value);
    ReverseIterator ReplaceTwo(ReverseIterator position, const T & value);

private:
};

template<typename T, size_t BlockSize>
typename LinkedList<T, BlockSize>::Iterator LinkedList<T, BlockSize>::begin()
{
    return Iterator(m_first);
}

template<typename T, size_t BlockSize>
typename LinkedList<T, BlockSize>::Iterator LinkedList<T, BlockSize>::end()
{
    return Iterator(nullptr);
}

template<typename T, size_t BlockSize>
typename LinkedList<T, BlockSize>::ConstIterator LinkedList<T, BlockSize>::begin() const
{
    return ConstIterator(m_first);
}

template<typename T, size_t BlockSize>
typename LinkedList<T, BlockSize>::ConstIterator LinkedList<T, BlockSize>::end() const
{
    return ConstIterator(nullptr);
}

template<typename T, size_t BlockSize>
typename LinkedList<T, BlockSize>::ReverseIterator LinkedList<T, BlockSize>::rbegin()
{
    return ReverseIterator(m_last);
}

template<typename T, size_t BlockSize>
typename LinkedList<T, BlockSize>::ReverseIterator LinkedList<T, BlockSize>::rend()
{
    return ReverseIterator(nullptr);
}

template<typename T, size_t BlockSize>
typename LinkedList<T, BlockSize>::ConstReverseIterator LinkedList<T, BlockSize>::rbegin() const
{
    return ConstReverseIterator(m_last);
}

template<typename T, size_t BlockSize>
typename LinkedList<T, BlockSize>::ConstReverseIterator LinkedList<T, BlockSize>::rend() const
{
    return ConstReverseIterator(nullptr);
}

template<typename T, size_t BlockSize>
typename LinkedList<T, BlockSize>::ConstReverseIterator LinkedList<T, BlockSize>::crbegin() const
{
    return ConstReverseIterator(m_last);
}

template<typename T, size_t BlockSize>
typename LinkedList<T, BlockSize>::ConstReverseIterator LinkedList<T, BlockSize>::crend() const
{
    return ConstReverseIterator(nullptr);
}

template<typename T, size_t BlockSize>
LinkedList<T, BlockSize>::~LinkedList()
{
    Clear();
}

template<typename T, size_t BlockSize>
bool LinkedList<T, BlockSize>::Empty() const
{
    return m_first == nullptr;
}

template<typename T, size_t BlockSize>
LinkedList<T, BlockSize>::LinkedList(const LinkedList& other)
{
    for(const T& value : other)
    {
        PushBack(value);
    }
}

template<typename T, size_t BlockSize>
LinkedList<T, BlockSize>& LinkedList<T, BlockSize>::operator=(const LinkedList& other)
{
    if(this != &other)
    {
        LinkedList copy(other);
        *this = std::move(copy);
    }
    return *this;
}

template<typename T, size_t BlockSize>
LinkedList<T, BlockSize>::LinkedList(LinkedList&& other) noexcept
    : m_first(other.m_first),
      m_last(other.m_last),
      m_allocatedNodes(std::move(other.m_allocatedNodes)),
      m_freeNodes(other.m_freeNodes)
{
    other.m_first = nullptr;
    other.m_last = nullptr;
    other.m_freeNodes = nullptr;
}

template<typename T, size_t BlockSize>
LinkedList<T, BlockSize>& LinkedList<T, BlockSize>::operator=(LinkedList&& other) noexcept
{
    if(this != &other)
    {
        Clear();
        m_allocatedNodes = std::move(other.m_allocatedNodes);
        m_first = other.m_first;
        m_last = other.m_last;
        m_freeNodes = other.m_freeNodes;
        other.m_first = nullptr;
        other.m_last = nullptr;
        other.m_freeNodes = nullptr;
    }
    return *this;
}

template<typename T, size_t BlockSize>
void LinkedList<T, BlockSize>::Clear()
{
    m_first = nullptr;
    m_last = nullptr;
    m_allocatedNodes.Clear();
    m_freeNodes = nullptr;
}

template<typename T, size_t BlockSize>
typename LinkedList<T, BlockSize>::Iterator LinkedList<T, BlockSize>::Erase(Iterator position)
{
    assert(position.m_node != nullptr);
    Node * current = position.m_node;
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
    DeAllocate(current);
    return position;
}

template<typename T, size_t BlockSize>
typename LinkedList<T, BlockSize>::ReverseIterator LinkedList<T, BlockSize>::Erase(ReverseIterator position)
{
    assert(position.m_node != nullptr);
    Node * current = position.m_node;
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
    DeAllocate(current);
    return position;
}

template<typename T, size_t BlockSize>
typename LinkedList<T, BlockSize>::Iterator LinkedList<T, BlockSize>::ReplaceTwo(Iterator position, const T & value)
{
    assert(position.m_node != nullptr);
    assert(position.m_node->m_next != nullptr);
    position = Erase(position);
    *position = value;
    return position;
}

template<typename T, size_t BlockSize>
typename LinkedList<T, BlockSize>::ReverseIterator LinkedList<T, BlockSize>::ReplaceTwo(ReverseIterator position, const T & value)
{
    assert(position.m_node != nullptr);
    assert(position.m_node->m_prev != nullptr);
    position = Erase(position);
    *position = value;
    return position;
}

template<typename T, size_t BlockSize>
void LinkedList<T, BlockSize>::PushBack(const T& value)
{
    EmplaceBack(value);
}

template<typename T, size_t BlockSize>
template<typename... Args>
T& LinkedList<T, BlockSize>::EmplaceBack(Args&&... args)
{
    Node* n = Allocate();
    std::destroy_at(&n->m_data);
    try
    {
        std::construct_at(&n->m_data, std::forward<Args>(args)...);
    }
    catch(...)
    {
        std::construct_at(&n->m_data);
        throw;
    }

    n->m_prev = m_last;
    n->m_next = nullptr;
    if(m_last)
    {
        m_last->m_next = n;
    }
    else
    {
        m_first = n;
    }
    m_last = n;
    return n->m_data;
}
