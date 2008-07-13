/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $URL$
 * $Id$
 *
 */

#if !defined(DISABLE_DEFAULT_EVENTMANAGER)

#include "common/system.h"
#include "common/config-manager.h"
#include "backends/events/default/default-events.h"

#include "engines/engine.h"
#include "gui/message.h"

#define RECORD_SIGNATURE 0x54455354
#define RECORD_VERSION 1

void readRecord(Common::InSaveFile *inFile, uint32 &diff, Common::Event &event) {
	diff = inFile->readUint32LE();

	event.type = (Common::EventType)inFile->readUint32LE();

	switch(event.type) {
	case Common::EVENT_KEYDOWN:
	case Common::EVENT_KEYUP:
		event.kbd.keycode = (Common::KeyCode)inFile->readSint32LE();
		event.kbd.ascii = inFile->readUint16LE();
		event.kbd.flags = inFile->readByte();
		break;
	case Common::EVENT_MOUSEMOVE:
	case Common::EVENT_LBUTTONDOWN:
	case Common::EVENT_LBUTTONUP:
	case Common::EVENT_RBUTTONDOWN:
	case Common::EVENT_RBUTTONUP:
	case Common::EVENT_WHEELUP:
	case Common::EVENT_WHEELDOWN:
		event.mouse.x = inFile->readSint16LE();
		event.mouse.y = inFile->readSint16LE();
		break;
	default:
		break;
	}
}

void writeRecord(Common::OutSaveFile *outFile, uint32 diff, Common::Event &event) {
	outFile->writeUint32LE(diff);

	outFile->writeUint32LE((uint32)event.type);

	switch(event.type) {
	case Common::EVENT_KEYDOWN:
	case Common::EVENT_KEYUP:
		outFile->writeSint32LE(event.kbd.keycode);
		outFile->writeUint16LE(event.kbd.ascii);
		outFile->writeByte(event.kbd.flags);
		break;
	case Common::EVENT_MOUSEMOVE:
	case Common::EVENT_LBUTTONDOWN:
	case Common::EVENT_LBUTTONUP:
	case Common::EVENT_RBUTTONDOWN:
	case Common::EVENT_RBUTTONUP:
	case Common::EVENT_WHEELUP:
	case Common::EVENT_WHEELDOWN:
		outFile->writeSint16LE(event.mouse.x);
		outFile->writeSint16LE(event.mouse.y);
		break;
	default:
		break;
	}
}

DefaultEventManager::DefaultEventManager(OSystem *boss) :
	_boss(boss),
	_buttonState(0),
	_modifierState(0),
	_shouldQuit(false),
	_shouldRTL(false) {

	assert(_boss);

	_recordFile = NULL;
	_recordTimeFile = NULL;
	_playbackFile = NULL;
	_playbackTimeFile = NULL;
	_timeMutex = _boss->createMutex();
	_recorderMutex = _boss->createMutex();

	_eventCount = 0;
	_lastEventCount = 0;
	_lastMillis = 0;

	Common::String recordModeString = ConfMan.get("record_mode");
	if (recordModeString.compareToIgnoreCase("record") == 0) {
		_recordMode = kRecorderRecord;
	} else {
		if (recordModeString.compareToIgnoreCase("playback") == 0) {
			_recordMode = kRecorderPlayback;
		} else {
			_recordMode = kPassthrough;
		}
	}

	_recordFileName = ConfMan.get("record_file_name");
	if (_recordFileName.empty()) {
		_recordFileName = "record.bin";
	}
	_recordTempFileName = ConfMan.get("record_temp_file_name");
	if (_recordTempFileName.empty()) {
		_recordTempFileName = "record.tmp";
	}
	_recordTimeFileName = ConfMan.get("record_time_file_name");
	if (_recordTimeFileName.empty()) {
		_recordTimeFileName = "record.time";
	}

	// Reset key repeat
	_currentKeyDown.keycode = 0;

	// recorder stuff
	if (_recordMode == kRecorderRecord) {
		_recordCount = 0;
		_recordTimeCount = 0;
		_recordFile = _boss->getSavefileManager()->openForSaving(_recordTempFileName.c_str());
		_recordTimeFile = _boss->getSavefileManager()->openForSaving(_recordTimeFileName.c_str());
		_recordSubtitles = ConfMan.getBool("subtitles");
	}

	uint32 sign;
	uint32 version;
	uint32 randomSourceCount;
	if (_recordMode == kRecorderPlayback) {
		_playbackCount = 0;
		_playbackTimeCount = 0;
		_playbackFile = _boss->getSavefileManager()->openForLoading(_recordFileName.c_str());
		_playbackTimeFile = _boss->getSavefileManager()->openForLoading(_recordTimeFileName.c_str());

		if (!_playbackFile) {
			warning("Cannot open playback file %s. Playback was switched off", _recordFileName.c_str());
			_recordMode = kPassthrough;
		}

		if (!_playbackTimeFile) {
			warning("Cannot open playback time file %s. Playback was switched off", _recordTimeFileName.c_str());
			_recordMode = kPassthrough;
		}
	}

	if (_recordMode == kRecorderPlayback) {
		sign = _playbackFile->readUint32LE();
		if (sign != RECORD_SIGNATURE) {
			error("Unknown record file signature");
		}
		version = _playbackFile->readUint32LE();

		// conf vars
		ConfMan.setBool("subtitles", _playbackFile->readByte() != 0);

		_recordCount = _playbackFile->readUint32LE();
		_recordTimeCount = _playbackFile->readUint32LE();
		randomSourceCount = _playbackFile->readUint32LE();
		for (uint i = 0; i < randomSourceCount; ++i) {
			RandomSourceRecord rec;
			rec.name = "";
			uint32 sLen = _playbackFile->readUint32LE();
			for (uint j = 0; j < sLen; ++j) {
				char c = _playbackFile->readSByte();
				rec.name += c;
			}
			rec.seed = _playbackFile->readUint32LE();
			_randomSourceRecords.push_back(rec);
		}

		_hasPlaybackEvent = false;
	}
}

DefaultEventManager::~DefaultEventManager() {
	_boss->lockMutex(_timeMutex);
	_boss->lockMutex(_recorderMutex);
	_recordMode = kPassthrough;
	_boss->unlockMutex(_timeMutex);
	_boss->unlockMutex(_recorderMutex);

	if (!artificialEventQueue.empty())
		artificialEventQueue.clear();

	if (_playbackFile != NULL) {
		delete _playbackFile;
	}
	if (_playbackTimeFile != NULL) {
		delete _playbackTimeFile;
	}

	if (_recordFile != NULL) {
		_recordFile->finalize();
		delete _recordFile;
		_recordTimeFile->finalize();
		delete _recordTimeFile;

		_playbackFile = _boss->getSavefileManager()->openForLoading(_recordTempFileName.c_str());

		_recordFile = _boss->getSavefileManager()->openForSaving(_recordFileName.c_str());
		_recordFile->writeUint32LE(RECORD_SIGNATURE);
		_recordFile->writeUint32LE(RECORD_VERSION);

		// conf vars
		_recordFile->writeByte(_recordSubtitles ? 1 : 0);

		_recordFile->writeUint32LE(_recordCount);
		_recordFile->writeUint32LE(_recordTimeCount);

		_recordFile->writeUint32LE(_randomSourceRecords.size());
		for (uint i = 0; i < _randomSourceRecords.size(); ++i) {
			_recordFile->writeUint32LE(_randomSourceRecords[i].name.size());
			_recordFile->writeString(_randomSourceRecords[i].name);
			_recordFile->writeUint32LE(_randomSourceRecords[i].seed);
		}

		for (uint i = 0; i < _recordCount; ++i) {
			uint32 tempDiff;
			Common::Event tempEvent;
			readRecord(_playbackFile, tempDiff, tempEvent);
			writeRecord(_recordFile, tempDiff, tempEvent);
		}

		_recordFile->finalize();
		delete _recordFile;
		delete _playbackFile;

		//TODO: remove recordTempFileName'ed file
	}
	_boss->deleteMutex(_timeMutex);
	_boss->deleteMutex(_recorderMutex);
}

bool DefaultEventManager::playback(Common::Event &event) {

	if (!_hasPlaybackEvent) {
		if (_recordCount > _playbackCount) {
			readRecord(_playbackFile, const_cast<uint32&>(_playbackDiff), _playbackEvent);
			_playbackCount++;
			_hasPlaybackEvent = true;
		}
	}

	if (_hasPlaybackEvent) {
		if (_playbackDiff <= (_eventCount - _lastEventCount)) {
			switch(_playbackEvent.type) {
			case Common::EVENT_MOUSEMOVE:
			case Common::EVENT_LBUTTONDOWN:
			case Common::EVENT_LBUTTONUP:
			case Common::EVENT_RBUTTONDOWN:
			case Common::EVENT_RBUTTONUP:
			case Common::EVENT_WHEELUP:
			case Common::EVENT_WHEELDOWN:
				_boss->warpMouse(_playbackEvent.mouse.x, _playbackEvent.mouse.y);
				break;
			default:
				break;
			}
			event = _playbackEvent;
			_hasPlaybackEvent = false;
			_lastEventCount = _eventCount;
			return true;
		}
	}

	return false;
}

void DefaultEventManager::record(Common::Event &event) {
	writeRecord(_recordFile, _eventCount - _lastEventCount, event);

	_recordCount++;
	_lastEventCount = _eventCount;
}

void DefaultEventManager::registerRandomSource(Common::RandomSource &rnd, const char *name) {

	if (_recordMode == kRecorderRecord) {
		RandomSourceRecord rec;
		rec.name = name;
		rec.seed = rnd.getSeed();
		_randomSourceRecords.push_back(rec);
	}

	if (_recordMode == kRecorderPlayback) {
		for (uint i = 0; i < _randomSourceRecords.size(); ++i) {
			if (_randomSourceRecords[i].name == name) {
				rnd.setSeed(_randomSourceRecords[i].seed);
				_randomSourceRecords.remove_at(i);
				break;
			}
		}
	}
}

void DefaultEventManager::processMillis(uint32 &millis) {
	uint32 d;
	if (_recordMode == kPassthrough) {
		return;
	}

	_boss->lockMutex(_timeMutex);
	if (_recordMode == kRecorderRecord) {
		//Simple RLE compression
		d = millis - _lastMillis;
		if (d >= 0xff) {
			_recordTimeFile->writeByte(0xff);
			_recordTimeFile->writeUint32LE(d);
		} else {
			_recordTimeFile->writeByte(d);
		}
		_recordTimeCount++;
	}

	if (_recordMode == kRecorderPlayback) {
		if (_recordTimeCount > _playbackTimeCount) {
			d = _playbackTimeFile->readByte();
			if (d == 0xff) {
				d = _playbackTimeFile->readUint32LE();
			}
			millis = _lastMillis + d;
			_playbackTimeCount++;
		}
	}

	_lastMillis = millis;
	_boss->unlockMutex(_timeMutex);
}

bool DefaultEventManager::pollEvent(Common::Event &event) {
	uint32 time = _boss->getMillis();
	bool result;

	if (!artificialEventQueue.empty()) {
		event = artificialEventQueue.pop();
		result = true;
	} else 	
		result = _boss->pollEvent(event);

	if (_recordMode != kPassthrough)  {

		_boss->lockMutex(_recorderMutex);
		_eventCount++;

		if (_recordMode == kRecorderPlayback)  {
			if (event.type != Common::EVENT_QUIT) {
				result = playback(event);
			}
		} else {
			if (_recordMode == kRecorderRecord) {
				if (result) {
					record(event);
				}
			}
		}
		_boss->unlockMutex(_recorderMutex);
	}

	if (result) {
		event.synthetic = false;
		switch (event.type) {
		case Common::EVENT_KEYDOWN:
			_modifierState = event.kbd.flags;
			// init continuous event stream
			// not done on PalmOS because keyboard is emulated and keyup is not generated
#if !defined(PALMOS_MODE)
			_currentKeyDown.ascii = event.kbd.ascii;
			_currentKeyDown.keycode = event.kbd.keycode;
			_currentKeyDown.flags = event.kbd.flags;
			_keyRepeatTime = time + kKeyRepeatInitialDelay;
#endif
			// Global Main Menu
			if (event.kbd.keycode == Common::KEYCODE_F11)
				if (g_engine && !g_engine->isPaused()) {
					Common::Event menuEvent;
					menuEvent.type = Common::EVENT_MAINMENU;
					pushEvent(menuEvent);
				}
			break;

		case Common::EVENT_KEYUP:
			_modifierState = event.kbd.flags;
			if (event.kbd.keycode == _currentKeyDown.keycode) {
				// Only stop firing events if it's the current key
				_currentKeyDown.keycode = 0;
			}
			break;

		case Common::EVENT_MOUSEMOVE:
			_mousePos = event.mouse;
			break;

		case Common::EVENT_LBUTTONDOWN:
			_mousePos = event.mouse;
			_buttonState |= LBUTTON;
			break;

		case Common::EVENT_LBUTTONUP:
			_mousePos = event.mouse;
			_buttonState &= ~LBUTTON;
			break;

		case Common::EVENT_RBUTTONDOWN:
			_mousePos = event.mouse;
			_buttonState |= RBUTTON;
			break;

		case Common::EVENT_RBUTTONUP:
			_mousePos = event.mouse;
			_buttonState &= ~RBUTTON;
			break;

		case Common::EVENT_MAINMENU:
			if (g_engine && !g_engine->isPaused())
				g_engine->mainMenuDialog();
			if (_shouldQuit)
				event.type = Common::EVENT_QUIT;
			break;

		case Common::EVENT_RTL:
			_shouldRTL = true;
			_shouldQuit = true;
			break;

		case Common::EVENT_QUIT:
			if (ConfMan.getBool("confirm_exit")) {
				if (g_engine)
					g_engine->pauseEngine(true);
				GUI::MessageDialog alert("Do you really want to quit?", "Yes", "No");
				result = _shouldQuit = (alert.runModal() == GUI::kMessageOK);
				if (g_engine)
					g_engine->pauseEngine(false);
			} else
				_shouldQuit = true;

			break;

		default:
			break;
		}
	} else {
		// Check if event should be sent again (keydown)
		if (_currentKeyDown.keycode != 0 && _keyRepeatTime < time) {
			// fire event
			event.type = Common::EVENT_KEYDOWN;
			event.synthetic = true;
			event.kbd.ascii = _currentKeyDown.ascii;
			event.kbd.keycode = (Common::KeyCode)_currentKeyDown.keycode;
			event.kbd.flags = _currentKeyDown.flags;
			_keyRepeatTime = time + kKeyRepeatSustainDelay;
			result = true;
		}
	}

	return result;
}

void DefaultEventManager::pushEvent(Common::Event event) {

	// If already received an EVENT_QUIT, don't add another one
	if (event.type == Common::EVENT_QUIT) {
		if (!_shouldQuit)
			artificialEventQueue.push(event);
	} else 
		artificialEventQueue.push(event);	
}

#endif // !defined(DISABLE_DEFAULT_EVENTMANAGER)
