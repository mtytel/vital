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

#include "random_lfo_test.h"
#include "random_lfo.h"

void RandomLfoTest::runTest() {
  vital::RandomLfo random_lfo;
  std::set<int> ignored_inputs;
  std::set<int> ignored_outputs;
  runInputBoundsTest(&random_lfo, ignored_inputs, ignored_outputs);
}

static RandomLfoTest random_lfo_test;
