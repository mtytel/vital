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

#include "full_interface_test.h"
#include "full_interface.h"
#include "synth_gui_interface.h"

void FullInterfaceTest::runTest() {
  createSynthEngine();
  SynthGuiData data(getSynthBase());
  MessageManager::getInstance();
  std::unique_ptr<MessageManagerLock> lock = std::make_unique<MessageManagerLock>();
  FullInterface* full_interface = new FullInterface(&data);
  lock.reset();

  full_interface->setOscilloscopeMemory(getSynthBase()->getOscilloscopeMemory());
  full_interface->setAudioMemory(getSynthBase()->getAudioMemory());

  runStressRandomTest(full_interface, full_interface);
  deleteSynthEngine();
}

static FullInterfaceTest full_interface_test;
