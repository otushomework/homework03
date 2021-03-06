#include <iostream>
#include <vector>
#include <map>
#include <typeinfo>

#define COUNT 10
//#define _DEBUG_

template<typename T, size_t MaxSize>
struct allocator
{
    using value_type = T;
    using pointer = T *;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;

private:
    unsigned char *data = nullptr;
    std::size_t offset = 0;

public:
    template<typename U>
    struct rebind
    {
        using other = allocator<U, MaxSize>;
    };

    pointer allocate(std::size_t n)
    {
        if ( (offset + n * sizeof(value_type)) > (MaxSize * sizeof(value_type)) )
            throw std::bad_alloc();

        if (data == nullptr)
        {
            data = ( unsigned char * ) malloc( MaxSize * sizeof( value_type ) );
#ifdef _DEBUG_
            std::cout << "allocate " << MaxSize * sizeof( value_type ) << std::endl;
#endif
        }

        void *addr = data + offset;
        offset += sizeof(value_type) * n;

        return static_cast<T *>(addr);
    }

    void deallocate(pointer, std::size_t n)
    {
        if (data != nullptr)
        {
            offset -= sizeof(value_type) * n;

            if (offset == 0)
            {
                free(data);
                data = nullptr;
#ifdef _DEBUG_
            std::cout << "deallocate " << MaxSize * sizeof( value_type ) << std::endl;
#endif
            }
        }
    }

    template <typename ... Args >
    void construct(pointer p, Args&& ... args)
    {
        new(p) T(std::forward <Args>(args) ... );
    }

    void destroy(pointer p)
    {
        p->~T();
    }
};

template <typename T, typename Allocator = std::allocator<T> >
struct ForwardList
{
private:
    template <typename T2>
    struct Node
    {
        Node(const T& _data)
            : data{_data}
            , next(nullptr)
        { }

        ~Node()
        {

        }

        T2 data;
        Node *next;
    };

    typename Allocator::template rebind<Node<T> >::other m_allocator;
    Node<T> *m_root = nullptr;
    Node<T> *m_head = nullptr;

public:
    ForwardList() = default;

    ForwardList(const ForwardList &other)
    {
        for(const auto& it : other)
            append(it);
    }

    ForwardList(ForwardList&& other)
        : m_allocator(other.m_allocator)
        , m_root(other.m_root)
        , m_head(other.m_head)
    {
        other.m_root = nullptr;
        other.m_head = nullptr;
    }

    ~ForwardList()
    {
        auto node = m_root;
        while (node != nullptr)
        {
            auto tmpNode = node->next;
            m_allocator.destroy(node);
            m_allocator.deallocate(node, 1);
            node = tmpNode;
        }
    }

    class iterator
    {
    public:
        iterator(Node<T>* node) : m_node(node) {}

        iterator() : m_node(0) {}

        iterator& operator ++()
        {
            m_node = m_node->next;
            return *this;
        }

        T operator * ()
        {
            return m_node->data;
        }

        bool operator!=(const iterator& rhg) const
        {
            return m_node != rhg.m_node;
        }

    private:
        Node<T>* m_node;
    };

    iterator begin() const
    {
        return iterator(m_root);
    }

    iterator end() const
    {
        return nullptr;
    }

    template <typename ... Args >
    void append(Args&& ... args)
    {
        auto newNode = m_allocator.allocate(1);
        m_allocator.construct(newNode, std::forward <Args>(args) ... );

        if (m_root == nullptr)
        {
            m_root = newNode;
            m_head = m_root;
        }
        else
        {
            m_head->next = newNode;
            m_head = m_head->next;
        }
    }
};

unsigned int factorial(unsigned int n)
{
    unsigned int ret = 1;
    for(unsigned int i = 1; i <= n; ++i)
        ret *= i;
    return ret;
}

//ltrace -e malloc -e free allocator
int main(int, char *[])
{
    std::map<int, int> container1;
    for (int i = 0; i < COUNT; ++i)
        container1.insert(std::pair<int, int>(i, factorial(i)));

    std::map<int, int, std::less<int>, allocator<std::pair<const int, int>, COUNT> > container2;

    for (int i = 0; i < COUNT; ++i)
        container2.insert(std::pair<const int, int>(i, factorial(i)));

    for (auto it = container2.cbegin(); it != container2.cend(); ++it)
        std::cout << it->first << " " << it->second << std::endl;

    ForwardList<int> container3;
    for (int i = 0; i < COUNT; ++i)
        container3.append(i);

    ForwardList<int, allocator<int, COUNT> > container4;
    for (int i = 0; i < COUNT; ++i)
        container4.append(i);

    //move
    ForwardList<int, allocator<int, COUNT> > container5(std::move(container4));

    //copy
    ForwardList<int, allocator<int, COUNT> > container6(container5);

    for(const auto& it : container6)
        std::cout << it << std::endl;

    return 0;
}
