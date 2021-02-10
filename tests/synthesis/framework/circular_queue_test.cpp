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

#include "circular_queue_test.h"
#include "circular_queue.h"

namespace {
  constexpr int kAddNumber = 100;
  constexpr int kLoopNumber = 10;

  int compareAscend(int left, int right) {
    return right - left;
  }

  int compareDescend(int left, int right) {
    return left - right;
  }
} // namespace

void CircularQueueTest::runTest() {
   testAddingRemoving();
   testLongQueue();
   testCount();
   testPopping();
   testResizing();
   testIterator();
   testClearing();
   testSorting();
}

void CircularQueueTest::testAddingRemoving() {
  vital::CircularQueue<int> queue;
  queue.reserve(kAddNumber);

  beginTest("Adding and Removing");
  expect(queue.capacity() == kAddNumber);

  for (int j = 0; j < kLoopNumber; ++j) {
    expect(queue.size() == 0);

    for (int i = 0; i < kAddNumber; ++i) {
      queue.push_back(i);
      expect(queue.size() == i + 1);
      expect(queue[i] == i);
      expect(queue.count(i) == 1);
    }

    for (int i = 0; i < kAddNumber; ++i)
      expect(queue.contains(i));

    expect(!queue.contains(-1));
    expect(!queue.contains(kAddNumber));

    int remove = kAddNumber / 2;
    queue.remove(remove);
    expect(queue.size() == kAddNumber - 1);

    for (int i = 0; i < kAddNumber; ++i)
      expect(queue.contains(i) == (i != remove));

    for (int i = 0; i < kAddNumber; ++i) {
      queue.remove(i);
      expect(!queue.contains(i));

      if (i < remove)
        expect(queue.size() == kAddNumber - i - 2);
      else
        expect(queue.size() == kAddNumber - i - 1);
    }

    for (int i = 0; i < kAddNumber; ++i)
      expect(!queue.contains(i));
  }
  expect(queue.size() == 0);
  expect(queue.capacity() == kAddNumber);
}

void CircularQueueTest::testClearing() {
  vital::CircularQueue<float> queue;
  queue.reserve(kAddNumber);

  beginTest("Clearing");
  expect(queue.capacity() == kAddNumber);

  for (int j = 0; j < kLoopNumber; ++j) {
    expect(queue.size() == 0);

    for (int i = 0; i < kAddNumber; ++i) {
      queue.push_back(i);
      expect(queue.size() == i + 1);
      expect(queue[i] == i);
      expect(queue.count(i) == 1);
    }

    for (int i = 0; i < kAddNumber; ++i)
      expect(queue.contains(i));

    expect(!queue.contains(-1));
    expect(!queue.contains(kAddNumber));

    int remove = kAddNumber / 2;
    queue.remove(remove);
    expect(queue.size() == kAddNumber - 1);

    queue.clear();

    for (int i = 0; i < kAddNumber; ++i)
      expect(!queue.contains(i));
  }
  expect(queue.size() == 0);
  expect(queue.capacity() == kAddNumber);
}

void CircularQueueTest::testLongQueue() {
  vital::CircularQueue<float> queue;
  queue.reserve(kAddNumber);

  beginTest("Long Queue");

  for (int i = 0; i < kAddNumber; ++i) {
    int number = kAddNumber - i - 1;
    queue.push_back(number);
    expect(queue.size() == i + 1);
    expect(queue[i] == number);
    expect(queue.count(number) == 1);
  }

  int remove_number = kAddNumber / 2;

  for (int j = 0; j < kLoopNumber; ++j) {
    expect(queue.size() == kAddNumber);

    for (int i = 0; i < remove_number; ++i) {
      int number = i + j * remove_number;
      expect(queue.count(number) == 1);
      queue.remove(number);
      expect(queue.size() == kAddNumber - i - 1);
      expect(queue.count(number) == 0);
    }

    expect(queue.size() == kAddNumber - remove_number);

    for (int i = 0; i < remove_number; ++i) {
      int number = i + j * remove_number + kAddNumber;
      if (i % 2)
        queue.push_back(number);
      else
        queue.push_front(number);

      expect(queue.size() == kAddNumber - remove_number + i + 1);
      expect(queue.count(number) == 1);
    }

    for (int i = 0; i < kAddNumber; ++i) {
      int number = i + (j + 1) * remove_number;
      expect(queue.contains(number));
    }
  }
  expect(queue.size() == kAddNumber);
  expect(queue.capacity() == kAddNumber);
}

void CircularQueueTest::testCount() {
  vital::CircularQueue<float> queue;
  queue.reserve(kAddNumber * kLoopNumber);

  beginTest("Count");

  for (int j = 0; j < kLoopNumber; ++j) {
    for (int i = 0; i < kAddNumber; ++i) {
      if ((i + j) % 2)
        queue.push_back(i + j);
      else
        queue.push_front(i + j);
    }
  }

  for (int i = 0; i < kLoopNumber + kAddNumber; ++i) {
    int count = std::min(std::min(kLoopNumber, i + 1), kLoopNumber + kAddNumber - i - 1);
    expect(queue.count(i) == count);
  }

  queue.clear();

  for (int i = 0; i < kLoopNumber + kAddNumber; ++i) {
    expect(queue.count(i) == 0);
    expect(!queue.contains(i));
  }
}

void CircularQueueTest::testResizing() {
  vital::CircularQueue<float> queue_ensure;
  vital::CircularQueue<float> queue_reserve;
  queue_ensure.reserve(kAddNumber);
  queue_reserve.reserve(kAddNumber);
  beginTest("Resizing");

  for (int j = 0; j < kLoopNumber; ++j) {

    for (int i = 0; i < kAddNumber; ++i) {
      int number = j * kAddNumber + i;
      queue_ensure.push_back(number);
      expect(queue_ensure.size() == number + 1);
      expect(queue_ensure[number] == number);
      expect(queue_ensure.count(number) == 1);

      queue_reserve.push_back(number);
      expect(queue_reserve.size() == number + 1);
      expect(queue_reserve[number] == number);
      expect(queue_reserve.count(number) == 1);
    }

    queue_reserve.reserve((j + 2) * kAddNumber);
    queue_ensure.ensureSpace(kAddNumber);

    for (int i = 0; i < (j + 1) * kAddNumber; ++i) {
      expect(queue_reserve[i] == i);
      expect(queue_reserve.count(i) == 1);
      expect(queue_ensure[i] == i);
      expect(queue_ensure.count(i) == 1);
    }
  }
}

void CircularQueueTest::testIterator() {
  vital::CircularQueue<float> queue;
  queue.reserve(kAddNumber);
  beginTest("Iterator");

  for (int i = 0; i < kAddNumber; ++i) {
    queue.push_back(i);
    expect(queue.size() == i + 1);
    expect(queue[i] == i);
    expect(queue.count(i) == 1);
  }

  int i = 0;
  for (auto iter : queue) {
    expect(iter == i);
    i++;
  }
}

void CircularQueueTest::testPopping() {
  vital::CircularQueue<float> queue;
  queue.reserve(kAddNumber * kLoopNumber);
  beginTest("Popping");

  for (int j = 0; j < kLoopNumber; ++j) {
    for (int i = 0; i < kAddNumber; ++i) {
      if ((i + j) % 2) {
        queue.push_back(i + j);
        expect(queue[queue.size() - 1] == i + j);
      }
      else {
        queue.push_front(i + j);
        expect(queue[0] == i + j);
      }
    }
  }

  int i = 0;
  while (queue.size()) {
    if (i % 3 == 0) {
      int front = queue[0];
      int count = queue.count(front);
      expect(count > 0);
      queue.pop_front();
      expect(count - 1 == queue.count(front));
      expect(count != 1 || !queue.contains(front));
    }
    else {
      int back = queue[queue.size() - 1];
      int count = queue.count(back);
      expect(count > 0);
      queue.pop_back();
      expect(count - 1 == queue.count(back));
      expect(count != 1 || !queue.contains(back));
    }
    i++;
  }

  expect(queue.size() == 0);
  for (int i = 0; i < kLoopNumber + kAddNumber; ++i) {
    expect(queue.count(i) == 0);
    expect(!queue.contains(i));
  }
}

void CircularQueueTest::testSorting() {
  vital::CircularQueue<int> queue;
  queue.reserve(kAddNumber);
  beginTest("Sorting");

  queue.push_back(5);
  queue.push_back(-2);
  queue.push_back(2);
  queue.push_back(9);
  queue.push_back(1);
  queue.push_back(0);
  queue.sort<compareAscend>();
  expect(queue[0] == -2);
  expect(queue[1] == 0);
  expect(queue[2] == 1);
  expect(queue[3] == 2);
  expect(queue[4] == 5);
  expect(queue[5] == 9);
  queue.sort<compareDescend>();
  expect(queue[0] == 9);
  expect(queue[1] == 5);
  expect(queue[2] == 2);
  expect(queue[3] == 1);
  expect(queue[4] == 0);
  expect(queue[5] == -2);

  queue.clear();

  for (int i = 0; i < kAddNumber; ++i)
    queue.push_back((i + kAddNumber / 2) % kAddNumber);

  queue.sort<compareAscend>();
  for (int i = 0; i < kAddNumber; ++i)
    expect(queue[i] == i);

  queue.sort<compareDescend>();
  for (int i = 0; i < kAddNumber; ++i)
    expect(queue[i] == kAddNumber - i - 1);
}

static CircularQueueTest circular_queue_test;
