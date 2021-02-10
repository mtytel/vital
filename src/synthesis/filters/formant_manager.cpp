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

#include "formant_manager.h"

#include "digital_svf.h"
#include "operators.h"

namespace vital {

  FormantManager::FormantManager(int num_formants) : ProcessorRouter(0, 1) {
    for (int i = 0; i < num_formants; ++i) {
      DigitalSvf* formant = new DigitalSvf();
      formant->setResonanceBounds(kMinResonance, kMaxResonance);
      formants_.push_back(formant);
      addProcessor(formant);
    }
  }

  void FormantManager::init() {
    int num_formants = static_cast<int>(formants_.size());
    VariableAdd* total = new VariableAdd(num_formants);
    for (DigitalSvf* formant : formants_)
      total->plugNext(formant);

    addProcessor(total);
    total->useOutput(output(0), 0);

    ProcessorRouter::init();
  }

  void FormantManager::reset(poly_mask reset_mask) {
    for (DigitalSvf* formant : formants_)
      getLocalProcessor(formant)->reset(reset_mask);
  }

  void FormantManager::hardReset() {
    for (DigitalSvf* formant : formants_)
      getLocalProcessor(formant)->hardReset();
  }
} // namespace vital
