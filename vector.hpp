#ifndef MY_VECTOR_HPP_
#define MY_VECTOR_HPP_

#include <memory>
#include <stdexcept>
#include <utility>

inline std::size_t round_up_to_the_power_of_two(std::size_t number) {
    if (number == 0) {
        return 0;
    }
    std::size_t result = 1;
    while (result < number) {
        result *= 2;
    }
    return result;
}

// call functor(index) with index owned [begin, end)
template <typename F>
inline void do_on_the_segment(std::size_t begin, std::size_t end, const F &functor) {
    for (std::size_t index = begin; index < end; index++) {
        functor(index);
    }
}

template <typename T, typename Alloc = std::allocator<T>>
class vector {
    //========//
    //==DATA==//
    //========//

    T *m_data = nullptr;
    std::size_t m_capacity = 0;
    std::size_t m_len = 0;

    //======================//
    //==ALLOCATION METHODS==//
    //======================//

    // разрушает данные в m_data
    void destroy_data() noexcept {
        do_on_the_segment(0, m_len, [&](std::size_t index) { m_data[index].~T(); });
    }

    // разрушает всю структуру
    void destroy() noexcept {
        destroy_data();
        if (m_capacity > 0) {
            Alloc().deallocate(m_data, m_capacity);
        }
    }

    static T *call_allocate(std::size_t capacity) {
        if (capacity == 0) {
            return nullptr;
        } else {
            return Alloc().allocate(capacity);
        }
    }

    // инициализирует структуру и выделяет память под size элементов
    void allocate(std::size_t size) {
        std::size_t new_capacity = round_up_to_the_power_of_two(size);
        m_data = call_allocate(new_capacity);
        m_capacity = new_capacity;
        m_len = size;
    }

    // копирует элементы из other
    void copy_from(const vector &other) noexcept {
        do_on_the_segment(0, m_len, [&](std::size_t index) { new (m_data + index) T(other[index]); });
    }

    // перемещает other в this
    // причем other получает во владение m_data, в котором могло что-то быть,
    // чтобы зря не терять память
    void move_from(vector &other) noexcept {
        m_len = std::exchange(other.m_len, 0);
        std::swap(m_capacity, other.m_capacity);
        std::swap(m_data, other.m_data);
    }

    // изменяет m_capacity на new_capacity и перемещает все элементы в новое
    // пространство
    void accept_new_capacity(std::size_t new_capacity) {
        T *new_data = call_allocate(new_capacity);

        do_on_the_segment(0, m_len, [&](std::size_t index) { new (new_data + index) T(std::move(m_data[index])); });

        destroy();

        m_data = new_data;
        m_capacity = new_capacity;
    }

public:
    //================//
    //==CONSTRUCTORS==//
    //================//

    ~vector() noexcept {
        destroy();
    }

    vector() noexcept = default;

    vector(const vector &other) {
        allocate(other.m_len);
        copy_from(other);
    }

    vector(vector &&other) noexcept {
        move_from(other);
    }

    vector(std::size_t size, const T &value) {
        allocate(size);
        do_on_the_segment(0, m_len, [&](std::size_t index) { new (m_data + index) T(value); });
    }

    vector(std::size_t size, T &&value) {
        allocate(size);
        T tmp = std::move(value);
        do_on_the_segment(0, m_len, [&](std::size_t index) { new (m_data + index) T(tmp); });
    }

    explicit vector(std::size_t size) {
        allocate(size);
        do_on_the_segment(0, m_len, [&](std::size_t index) { new (m_data + index) T(); });
    }

    //==========================//
    //==COPY AND MOVE OPERATOR==//
    //==========================//

    vector &operator=(const vector &other) {
        if (this == &other) {
            return *this;
        }

        if (m_capacity >= other.m_len) {
            // мне хватает памяти, чтобы скопировать элементы

            if (m_len < other.m_len) {
                do_on_the_segment(0, m_len, [&](std::size_t index) { m_data[index] = other[index]; });
                do_on_the_segment(m_len, other.m_len, [&](std::size_t index) { new (m_data + index) T(other[index]); });
            } else {
                do_on_the_segment(other.m_len, m_len, [&](std::size_t index) { m_data[index].~T(); });
                do_on_the_segment(0, other.m_len, [&](std::size_t index) { m_data[index] = other[index]; });
            }

            m_len = other.m_len;
        } else {
            std::size_t new_capacity = round_up_to_the_power_of_two(other.m_len);
            T *new_data = call_allocate(new_capacity);

            destroy();

            m_data = new_data;
            m_capacity = new_capacity;
            m_len = other.m_len;

            copy_from(other);
        }
        return *this;
    }

    vector &operator=(vector &&other) noexcept {
        if (this != &other) {
            destroy_data();
            move_from(other);
        } else {
            clear();
        }
        return *this;
    }

    //===================//
    //==TRIVIAL METHODS==//
    //===================//

    [[nodiscard]] std::size_t size() const noexcept {
        return m_len;
    }

    [[nodiscard]] std::size_t capacity() const noexcept {
        return m_capacity;
    }

    [[nodiscard]] bool empty() const noexcept {
        return m_len == 0;
    }

private:
    void verify_bound(size_t index) const {
        if (index >= m_len) {
            throw std::out_of_range("my vector failed bound");
        }
    }

public:
    //===========================//
    //==RANDOM ACCESS OPERATORS==//
    //===========================//

    T &at(std::size_t index) & {
        verify_bound(index);
        return m_data[index];
    }

    const T &at(std::size_t index) const & {
        verify_bound(index);
        return m_data[index];
    }

    T &&at(std::size_t index) && {
        verify_bound(index);
        return std::move(m_data[index]);
    }

    T &operator[](std::size_t index) &noexcept {
        return m_data[index];
    }

    const T &operator[](std::size_t index) const &noexcept {
        return m_data[index];
    }

    T &&operator[](std::size_t index) &&noexcept {
        return std::move(m_data[index]);
    }

    //==================//
    //==CHANGE METHODS==//
    //==================//

    void pop_back() &noexcept {
        m_len--;
        m_data[m_len].~T();
    }

private:
    template <typename F>
    void push_back_impl(F functor) {
        if (m_len == m_capacity) {
            std::size_t new_capacity = round_up_to_the_power_of_two(m_capacity + 1);
            T *new_data = call_allocate(new_capacity);
            functor(new_data);
            do_on_the_segment(0, m_len, [&](std::size_t index) { new (new_data + index) T(std::move(m_data[index])); });
            destroy();
            m_len++;
            m_data = new_data;
            m_capacity = new_capacity;
        } else {
            functor(m_data);
            m_len++;
        }
    }

public:
    void push_back(const T &value) & {
        push_back_impl([&](T *data) { new (data + m_len) T(value); });
    }

    void push_back(T &&value) & {
        push_back_impl([&](T *data) { new (data + m_len) T(std::move(value)); });
    }

    void clear() &noexcept {
        destroy_data();
        m_len = 0;
    }

    void reserve(std::size_t size) & {
        std::size_t need_capacity = round_up_to_the_power_of_two(size);
        if (m_capacity < need_capacity) {
            accept_new_capacity(need_capacity);
        }
    }

private:
    template <typename F>
    void increase_size(std::size_t size, F functor) {
        if (size <= m_len) {
            return;
        }
        std::size_t need_capacity = round_up_to_the_power_of_two(size);
        if (need_capacity > m_capacity) {
            // need new buffer
            T *new_data = call_allocate(need_capacity);
            // костыль, чтобы functor вызывался на new_data
            std::swap(new_data, m_data);
            do_on_the_segment(m_len, size, functor);
            std::swap(new_data, m_data);
            do_on_the_segment(0, m_len, [&](std::size_t index) { new (new_data + index) T(std::move(m_data[index])); });
            destroy();
            m_data = new_data;
            m_capacity = need_capacity;
        } else {
            // no new buffer
            do_on_the_segment(m_len, size, functor);
        }
        m_len = size;
    }

    void reduce_size(std::size_t size) {
        if (size >= m_len) {
            return;
        }
        do_on_the_segment(size, m_len, [&](std::size_t index) { m_data[index].~T(); });
        m_len = size;
    }

public:
    void resize(std::size_t size) & {
        increase_size(size, [&](std::size_t index) { new (m_data + index) T(); });
        reduce_size(size);
    }

    void resize(std::size_t size, T &&value) & {
        T tmp = std::move(value);
        increase_size(size, [&](std::size_t index) { new (m_data + index) T(tmp); });
        reduce_size(size);
    }

    void resize(std::size_t size, const T &value) & {
        increase_size(size, [&](std::size_t index) { new (m_data + index) T(value); });
        reduce_size(size);
    }
};

#endif  // MY_VECTOR_HPP_