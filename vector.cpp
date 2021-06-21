#ifndef VECTOR_H_
#define VECTOR_H_

#include <algorithm>
#include <cassert>
#include <exception>
#include <memory>
#include <sstream>
#include <type_traits>

namespace data_structures {

template <typename T, typename Alloc = std::allocator<T>>
class vector {
    static_assert(std::is_nothrow_move_constructible_v<T>);
    static_assert(std::is_nothrow_move_assignable_v<T>);
    static_assert(std::is_nothrow_destructible_v<T>);

private:
    inline static std::size_t next_2_degree(std::size_t x) noexcept {
        if (x == 0) {
            return 0;
        }
        --x;
        for (std::size_t i = 1; i < sizeof(x); i *= 2) {
            x |= x >> i;
        }  
        return ++x;
    }

    inline static std::string error_message(std::size_t ind, std::size_t size) {
        std::stringstream s;
        s << "tried to use " << ind << " element, but " << ind
          << " is out of range [0, " << size << ")";
        return s.str();
    }

    struct DefaultInitializer {
        void operator()(T *ptr) {
            new (ptr) T();
        }
    };

    struct CopyInitializer {
    private:
        const T &value;

    public:
        explicit CopyInitializer(const T &value_) : value(value_) {
        }
        void operator()(T *ptr) {
            new (ptr) T(value);
        }
    };

    struct MoveInitializer {
    private:
        T &&value;

    public:
        explicit MoveInitializer(T &&value_) noexcept
            : value(std::move(value_)) {
        }
        void operator()(T *ptr) noexcept {
            new (ptr) T(std::move(value));
        }
    };

    struct CopyFromVectorInitializer {
    private:
        const vector &other;
        T *to_data;

    public:
        explicit CopyFromVectorInitializer(const vector &other_,
                                           T *to_data_) noexcept
            : other(other_), to_data(to_data_) {
        }
        void operator()(T *ptr) {
            std::size_t ind = ptr - to_data;
            new (ptr) T(other[ind]);
        }
    };

    struct MoveFromVectorInitializer {
    private:
        vector &other;
        T *to_data;

    public:
        explicit MoveFromVectorInitializer(vector &other_, T *to_data_) noexcept
            : other(other_), to_data(to_data_) {
        }
        void operator()(T *ptr) noexcept {
            std::size_t ind = ptr - to_data;
            new (ptr) T(std::move(other[ind]));
        }
    };

    struct DataHolder {
    private:
        std::size_t capacity = 0;
        T *data = nullptr;

        T *allocate_memory(std::size_t capacity) {
            if (capacity > 0) {
                return Alloc().allocate(capacity);
            } else {
                return nullptr;
            }
        }

        void delete_memory() noexcept {
            if (capacity > 0 && data) {
                Alloc().deallocate(data, capacity);
            }
            capacity = 0;
            data = nullptr;
        }

    public:
        [[nodiscard]] std::size_t get_capacity() const noexcept {
            return capacity;
        }

        [[nodiscard]] T *get_data()
            const noexcept {
            return data;
        }

        DataHolder() = default;

        explicit DataHolder(std::size_t min_capacity)
            : capacity(next_2_degree(min_capacity)),
              data(allocate_memory(capacity)) {
        }

        DataHolder(const DataHolder &) = delete;
        DataHolder &operator=(const DataHolder &) = delete;

        DataHolder(DataHolder &&other) noexcept
            : capacity(std::exchange(other.capacity, 0)),
              data(std::exchange(other.data, nullptr)) {
        }

        DataHolder &operator=(DataHolder &&other) noexcept {
            if (this == &other) {
                return *this;
            }
            std::swap(capacity, other.capacity);
            std::swap(data, other.data);
            return *this;
        }

        ~DataHolder() noexcept {
            delete_memory();
        }
    };

    T *data()
        const noexcept {
        return buffer.get_data();
    }

    DataHolder buffer;
    std::size_t size_ = 0;

    struct only_capacity_constructor_tag {};

    vector(std::size_t min_capacity, only_capacity_constructor_tag)
        : buffer(min_capacity) {
    }

    void destroy_half_interval(std::size_t begin, std::size_t end) noexcept {
        for (std::size_t i = begin; i < end; ++i) {
            (data() + i)->~T();
        }
    }

    template <typename F>
    void initialize_half_interval(F init, std::size_t begin, std::size_t end) {
        for (std::size_t i = begin; i < end; ++i) {
            try {
                init(data() + i);
            } catch (...) {
                destroy_half_interval(begin, i);
                throw;
            }
        }
    }

    template <typename F>
    void insert_back(F init, std::size_t count) & {
        initialize_half_interval(init, size_, size_ + count);
        size_ += count;
    }

    template <typename F>
    void resize(std::size_t new_size, F init) & {
        if (new_size == size_) {
            return;
        }
        if (new_size < size_) {
            destroy_half_interval(new_size, size_);
            size_ = new_size;
        } else {
            if (new_size <= capacity()) {
                insert_back(init, new_size - size_);
            } else {
                vector new_vector(new_size, only_capacity_constructor_tag{});
                new_vector.initialize_half_interval(init, size_, new_size);
                new_vector.initialize_half_interval(
                    MoveFromVectorInitializer(*this, new_vector.data()), 0,
                    size_);
                new_vector.size_ = new_size;
                *this = std::move(new_vector);
            }
        }
    }

public:
    vector() noexcept = default;

    explicit vector(std::size_t size_) : buffer(size_) {
        insert_back(DefaultInitializer(), size_);
    }

    vector(std::size_t size_, const T &value) : buffer(size_) {
        insert_back(CopyInitializer(value), size_);
    }

    vector(const vector &other) : buffer(other.size()) {
        insert_back(CopyFromVectorInitializer(other, data()), other.size());
    }

    vector &operator=(const vector &other) {
        if (this == &other) {
            return *this;
        }
        if (other.empty()) {
            clear();
            return *this;
        }
        if (empty() && other.size() <= capacity()) {
            insert_back(CopyFromVectorInitializer(other, data()), other.size());
            return *this;
        }
        if constexpr (std::is_nothrow_copy_assignable_v<T>) {
            if (capacity() >= other.size()) {
                std::size_t old_size = size();
                if (size() < other.size()) {
                    insert_back(CopyFromVectorInitializer(other, data()),
                                other.size() - size());
                }
                for (std::size_t i = 0; i < std::min(old_size, other.size());
                     ++i) {
                    (*this)[i] = other[i];
                }
                if (size() > other.size()) {
                    destroy_half_interval(other.size(), size());
                    size_ = other.size();
                }
            } else {
                *this = vector(other);
            }
        } else {
            *this = vector(other);
        }
        return *this;
    }

    vector(vector &&other) noexcept
        : buffer(std::move(other.buffer)),
          size_(std::exchange(other.size_, 0)) {
    }

    vector &operator=(vector &&other) noexcept {
        clear();
        if (this == &other) {
            return *this;
        }
        buffer = std::move(other.buffer);
        std::swap(size_, other.size_);
        return *this;
    }

    [[nodiscard]] bool empty() const noexcept {
        return size_ == 0;
    }

    [[nodiscard]] std::size_t size() const noexcept {
        return size_;
    }

    [[nodiscard]] std::size_t capacity() const noexcept {
        return buffer.get_capacity();
    }

    T &at(std::size_t ind) & {
        if (ind >= size()) {
            throw std::out_of_range(error_message(ind, size()));
        }
        return *(data() + ind);
    }

    T &&at(std::size_t ind) && {
        if (ind >= size()) {
            throw std::out_of_range(error_message(ind, size()));
        }
        return std::move(*(data() + ind));
    }

    const T &at(std::size_t ind) const & {
        if (ind >= size()) {
            throw std::out_of_range(error_message(ind, size()));
        }
        return *(data() + ind);
    }

    T &operator[](std::size_t ind) &noexcept {
        return *(data() + ind);
    }

    T &&operator[](std::size_t ind) &&noexcept {
        return std::move(*(data() + ind));
    }

    const T &operator[](std::size_t ind) const &noexcept {
        return *(data() + ind);
    }

    void reserve(std::size_t min_capacity) & {
        if (min_capacity <= capacity()) {
            return;
        }
        vector new_vector(min_capacity, only_capacity_constructor_tag{});
        new_vector.insert_back(
            MoveFromVectorInitializer(*this, new_vector.data()), size());
        *this = std::move(new_vector);
    }

    void resize(std::size_t new_size) & {
        resize(new_size, DefaultInitializer());
    }

    void resize(std::size_t new_size, const T &value) & {
        resize(new_size, CopyInitializer(value));
    }

    void push_back(T &&value) & {
        reserve(size() + 1);
        MoveInitializer(std::move(value))(data() + size());
        ++size_;
    }

    void push_back(const T &value) & {
        resize(size() + 1, value);
    }

    void pop_back() &noexcept {
        assert(size() > 0);
        destroy_half_interval(size_ - 1, size_);
        --size_;
    }

    void clear() &noexcept {
        destroy_half_interval(0, size_);
        size_ = 0;
    }

    ~vector() noexcept {
        clear();
    };
};

}  // namespace data_structures

#endif  // VECTOR_H_