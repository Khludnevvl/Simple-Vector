#pragma once
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <memory>
#include <new>
#include <utility>
#include <initializer_list>

namespace simple_vector
{
template <typename T>
class RawMemory
{
public:
    RawMemory() = default;

    explicit RawMemory(size_t capacity)
        : buffer_(Allocate(capacity)), capacity_(capacity)
    {
    }

    ~RawMemory()
    {
        Deallocate(buffer_);
    }

    T *operator+(size_t offset) noexcept
    {
        assert(offset <= capacity_);
        return buffer_ + offset;
    }

    const T *operator+(size_t offset) const noexcept
    {
        return const_cast<RawMemory &>(*this) + offset;
    }

    const T &operator[](size_t index) const noexcept
    {
        return const_cast<RawMemory &>(*this)[index];
    }

    T &operator[](size_t index) noexcept
    {
        assert(index < capacity_);
        return buffer_[index];
    }

    void Swap(RawMemory &other) noexcept
    {
        std::swap(buffer_, other.buffer_);
        std::swap(capacity_, other.capacity_);
    }

    const T *GetAddress() const noexcept
    {
        return buffer_;
    }

    T *GetAddress() noexcept
    {
        return buffer_;
    }

    size_t Capacity() const
    {
        return capacity_;
    }

private:
    static T *Allocate(size_t n)
    {
        return n != 0 ? static_cast<T *>(operator new(n * sizeof(T))) : nullptr;
    }

    // Освобождает сырую память, выделенную ранее по адресу buf при помощи Allocate
    static void Deallocate(T *buf) noexcept
    {
        operator delete(buf);
    }

    T *buffer_ = nullptr;
    size_t capacity_ = 0;
};

template <typename T>
class Vector
{
public:
    using iterator = T *;
    using const_iterator = const T *;

    explicit Vector() = default;

    explicit Vector(size_t size)
        : data_(size), size_(size)
    {
        std::uninitialized_value_construct_n(data_.GetAddress(), size_);
    }

    explicit Vector(const Vector &other)
        : data_(other.size_), size_(other.size_)
    {
        std::uninitialized_copy_n(other.data_.GetAddress(), size_, data_.GetAddress());
    }

    explicit Vector(Vector &&other) noexcept
    {
        Swap(other);
    }

    iterator begin() noexcept
    {
        return data_.GetAddress();
    }
    iterator end() noexcept
    {
        return data_.GetAddress() + size_;
    }
    const_iterator begin() const noexcept
    {
        return data_.GetAddress();
    }
    const_iterator end() const noexcept
    {
        return data_.GetAddress() + size_;
    }
    const_iterator cbegin() const noexcept
    {
        return begin();
    }
    const_iterator cend() const noexcept
    {
        return end();
    }

    Vector &operator=(const Vector &rhs)
    {
        if (this != &rhs)
        {
            if (rhs.size_ > this->Capacity())
            {
                Vector rhs_copy(rhs);
                Swap(rhs_copy);
            }
            else
            {
                if (rhs.size_ >= this->size_)
                {
                    std::copy(rhs.data_.GetAddress(), rhs.data_ + this->size_, data_.GetAddress());
                    std::uninitialized_copy_n(rhs.data_ + this->size_, rhs.size_ - this->size_, data_.GetAddress() + this->size_);
                }
                if (rhs.size_ < this->size_)
                {
                    std::copy(rhs.data_.GetAddress(), rhs.data_ + rhs.size_, data_.GetAddress());
                    std::destroy_n(data_ + rhs.size_, this->size_ - rhs.size_);
                }
                this->size_ = rhs.size_;
            }
        }
        return *this;
    }
    Vector &operator=(Vector &&rhs) noexcept
    {
        Swap(rhs);
        return *this;
    }

    void Swap(Vector &rhs) noexcept
    {
        data_.Swap(rhs.data_);
        std::swap(size_, rhs.size_);
    }

    void Reserve(size_t new_capacity)
    {
        if (new_capacity <= data_.Capacity())
        {
            return;
        }
        RawMemory<T> new_data(new_capacity);

        if constexpr (!std::is_copy_constructible_v<T> || std::is_nothrow_move_constructible_v<T>)
        {
            std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        else
        {
            std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        std::destroy_n(data_.GetAddress(), size_);
        data_.Swap(new_data);
    }

    void Resize(size_t new_size)
    {
        Reserve(new_size);

        if (new_size <= this->size_)
        {
            std::destroy_n(data_ + new_size, this->size_ - new_size);
        }
        else
        {
            std::uninitialized_value_construct_n(data_ + this->size_, new_size - this->size_);
        }
        size_ = new_size;
    }

    void PushBack(const T &value)
    {
        EmplaceBack(value);
    }

    void PushBack(T &&value)
    {
        EmplaceBack(std::move(value));
    }

    template <typename... Args>
    T &EmplaceBack(Args &&...args)
    {
        if (size_ == Capacity())
        {

            const size_t new_capacity = (size_ == 0 ? 1 : size_ * 2);
            RawMemory<T> new_data(new_capacity);

            new (new_data + this->size_) T(std::forward<Args>(args)...);
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>)
            {
                std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            else if (std::is_copy_constructible_v<T>)
            {
                std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            data_.Swap(new_data);
            std::destroy_n(new_data.GetAddress(), size_);
        }
        else
        {
            new (data_ + size_) T(std::forward<Args>(args)...);
        }
        size_++;
        return *(end() - 1);
    }

    template <typename... Args>
    iterator Emplace(const_iterator pos, Args &&...args)
    {
        if (pos == end())
        {
            EmplaceBack(std::forward<Args>(args)...);
            return end() - 1;
        }
        const size_t elem_index = std::distance(cbegin(), pos);

        if (size_ == Capacity())
        {
            const size_t new_capacity = (size_ == 0 ? 1 : size_ * 2);
            RawMemory<T> new_data(new_capacity);
            new (new_data + elem_index) T(std::forward<Args>(args)...);
            if constexpr (!std::is_copy_constructible_v<T> || std::is_nothrow_move_constructible_v<T>)
            {
                std::uninitialized_move_n(begin(), elem_index, new_data.GetAddress());
                std::uninitialized_move_n(begin() + elem_index, size_ - elem_index, new_data + elem_index + 1);
            }
            else
            {
                std::uninitialized_copy_n(begin(), elem_index, new_data.GetAddress());
                std::uninitialized_copy_n(begin() + elem_index + 1, size_ - elem_index, new_data + elem_index + 1);
            }
            data_.Swap(new_data);
            std::destroy_n(new_data.GetAddress(), size_);
        }
        else if (size_ < Capacity())
        {
            T new_value = T(std::forward<Args>(args)...);
            T *last_value = std::prev(end());
            new (data_ + size_) T(std::move(*last_value));
            std::move_backward(begin() + elem_index, end() - 1, end());
            data_[elem_index] = std::move(new_value);
        }
        size_++;
        return begin() + elem_index;
    }

    iterator Erase(const_iterator pos) /*noexcept(std::is_nothrow_move_assignable_v<T>)*/
    {
        const size_t elem_index = std::distance(cbegin(), pos);
        std::move(begin() + elem_index + 1, end(), begin() + elem_index);
        std::destroy_at(end() - 1);
        size_--;

        return begin() + elem_index;
    }

    iterator Insert(const_iterator pos, const T &value)
    {
        return Emplace(pos, value);
    }

    iterator Insert(const_iterator pos, T &&value)
    {
        return Emplace(pos, std::move(value));
    }

    void PopBack() /* noexcept */
    {
        assert(size_ > 0);
        std::destroy_at(data_ + size_ - 1);
        --size_;
    }

    size_t Size() const noexcept
    {
        return size_;
    }

    size_t Capacity() const noexcept
    {
        return data_.Capacity();
    }

    const T &operator[](size_t index) const noexcept
    {
        return const_cast<Vector &>(*this)[index];
    }

    T &operator[](size_t index) noexcept
    {
        assert(index < size_);
        return data_[index];
    }

    ~Vector()
    {
        std::destroy_n(data_.GetAddress(), size_);
    }

    bool operator==(const Vector &rhs) const
    {
        return std::equal(begin(), end(), rhs.begin());
    }
    bool operator!=(const Vector &rhs) const
    {
        return !(*this == rhs);
    }

private:

    static void Destroy(T *buf) noexcept
    {
        buf->~T();
    }

    RawMemory<T> data_;
    size_t size_ = 0;
};
}//namespace simple_vector