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

#include "comb_filter_test.h"
#include "comb_filter.h"

void CombFilterTest::runTest() {
  vital::CombFilter comb_filter(5000);

  std::set<int> ignored_inputs;
  ignored_inputs.insert(vital::CombFilter::kStyle);
  vital::Value style;
  comb_filter.plug(&style, vital::CombFilter::kStyle);
  
  for (int i = 0; i < vital::CombFilter::kNumFilterTypes; ++i) {
    style.set(i);
    runInputBoundsTest(&comb_filter, ignored_inputs, std::set<int>());
  }
}

static CombFilterTest comb_filter_test;
