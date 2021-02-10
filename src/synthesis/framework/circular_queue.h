/* Copyright 2013-2019 Matt Tytel
 *
 * vital is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * vital is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with vital.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "common.h"
#include <cstddef>
#include <memory>

namespace vital {

  template<class T>
  class CircularQueue {
    public:

      class iterator {
        public:
          iterator(T* pointer, T* front, T* end) : pointer_(pointer), front_(front), end_(end) { }

          force_inline void increment() {
            if (pointer_ == end_)
              pointer_ = front_;
            else
              pointer_++;
          }

          force_inline void decrement() {
            if (pointer_ == front_)
              pointer_ = end_;
            else
              pointer_--;
          }

          force_inline iterator operator++() {
            iterator iter = *this;
            increment();
            return iter;
          }

          force_inline const iterator operator++(int i) {
            iterator iter = *this;
            increment();
            return iter;
          }

          force_inline iterator operator--() {
            iterator iter = *this;
            decrement();
            return iter;
          }

          force_inline const iterator operator--(int i) {
            iterator iter = *this;
            decrement();
            return iter;
          }

          force_inline T& operator*() {
            return *pointer_;
          }

          force_inline T* operator->() {
            return pointer_;
          }

          force_inline T* get() {
            return pointer_;
          }

          force_inline bool operator==(const iterator& rhs) {
            return pointer_ == rhs.pointer_;
          }

          force_inline bool operator!=(const iterator& rhs) {
            return pointer_ != rhs.pointer_;
          }

        protected:
          T* pointer_;
          T* front_;
          T* end_;
      };

      CircularQueue(int capacity) : capacity_(capacity + 1), start_(0), end_(0) {
        data_ = std::make_unique<T[]>(capacity_);
      }

      CircularQueue(const CircularQueue& other) {
        capacity_ = other.capacity_;
        data_ = std::make_unique<T[]>(capacity_);
        memcpy(data_.get(), other.data_.get(), capacity_ * sizeof(T));
        start_ = other.start_;
        end_ = other.end_;
      }

      CircularQueue() : data_(nullptr), capacity_(0), start_(0), end_(0) { }

      void reserve(int capacity) {
        int new_capacity = capacity + 1;
        if (new_capacity < capacity_)
          return;

        std::unique_ptr<T[]> tmp = std::make_unique<T[]>(new_capacity);

        if (capacity_) {
          end_ = size();
          for (int i = 0; i < end_; ++i)
            tmp[i] = std::move(at(i));
        }

        data_ = std::move(tmp);
        capacity_ = new_capacity;
        start_ = 0;
      }

      force_inline void assign(int num, T value) {
        if (num > capacity_)
          reserve(num);

        for (int i = 0; i < num; ++i)
          data_[i] = value;

        start_ = 0;
        end_ = num;
      }

      force_inline T& at(std::size_t index) {
        VITAL_ASSERT(index >= 0 && index < size());
        return data_[(start_ + static_cast<int>(index)) % capacity_];
      }

      force_inline T& operator[](std::size_t index) {
        return at(index);
      }

      force_inline const T& operator[](std::size_t index) const {
        return at(index);
      }

      force_inline void push_back(T entry) {
        data_[end_] = std::move(entry);
        end_ = (end_ + 1) % capacity_;
        VITAL_ASSERT(end_ != start_);
      }

      force_inline T pop_back() {
        VITAL_ASSERT(end_ != start_);
        end_ = (end_ - 1 + capacity_) % capacity_;
        return data_[end_];
      }

      force_inline void push_front(T entry) {
        start_ = (start_ - 1 + capacity_) % capacity_;
        data_[start_] = entry;
        VITAL_ASSERT(end_ != start_);
      }

      force_inline T pop_front() {
        VITAL_ASSERT(end_ != start_);
        int last = start_;
        start_ = (start_ + 1) % capacity_;
        return data_[last];
      }

      force_inline void removeAt(int index) {
        VITAL_ASSERT(end_ != start_);
        int i = (index + start_) % capacity_;
        end_ = (end_ - 1 + capacity_) % capacity_;
        while (i != end_) {
          int next = (i + 1) % capacity_;
          data_[i] = data_[next];
          i = next;
        }
      }

      force_inline void remove(T entry) {
        for (int i = start_; i != end_; i = (i + 1) % capacity_) {
          if (data_[i] == entry) {
            removeAt((i - start_ + capacity_) % capacity_);
            return;
          }
        }
      }

      void removeAll(T entry) {
        for (int i = start_; i != end_; i = (i + 1) % capacity_) {
          if (data_[i] == entry) {
            removeAt((i - start_ + capacity_) % capacity_);
            i--;
          }
        }
      }

      template<int(*compare)(T, T)>
      void sort() {
        int total = size();
        for (int i = 1; i < total; ++i) {
          T value = at(i);
          int j = i - 1;
          for (; j >= 0 && compare(at(j), value) < 0; --j)
            at(j + 1) = at(j);

          at(j + 1) = value;
        }
      }

      void ensureSpace(int space = 2) {
        if (size() + space >= capacity())
          reserve(capacity_ + std::max(capacity_, space));
      }

      void ensureCapacity(int min_capacity) {
        if (min_capacity >= capacity())
          reserve(capacity_ + std::max(capacity_, min_capacity));
      }

      force_inline iterator erase(iterator& iter) {
        int index = static_cast<int>(iter.get() - data_.get());
        removeAt((index - start_ + capacity_) % capacity_);
        return iter;
      }

      int count(T entry) const {
        int number = 0;
        for (int i = start_; i != end_; i = (i + 1) % capacity_) {
          if (data_[i] == entry)
            number++;
        }
        return number;
      }

      bool contains(T entry) const {
        for (int i = start_; i != end_; i = (i + 1) % capacity_) {
          if (data_[i] == entry)
            return true;
        }
        return false;
      }

      force_inline void clear() {
        start_ = 0;
        end_ = 0;
      }

      force_inline T front() const {
        return data_[start_];
      }

      force_inline T back() const {
        return data_[(end_ - 1 + capacity_) % capacity_];
      }

      force_inline int size() const {
        VITAL_ASSERT(capacity_ > 0);
        return (end_ - start_ + capacity_) % capacity_;
      }

      force_inline int capacity() const {
        return capacity_ - 1;
      }

      force_inline iterator begin() const {
        return iterator(data_.get() + start_, data_.get(), data_.get() + (capacity_ - 1));
      }

      force_inline iterator end() const {
        return iterator(data_.get() + end_, data_.get(), data_.get() + (capacity_ - 1));
      }

    private:
      std::unique_ptr<T[]> data_;
      int capacity_;
      int start_;
      int end_;
  };
} // namespace vital

