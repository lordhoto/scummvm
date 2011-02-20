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
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $URL$
 * $Id$
 *
 */

#include "kyra/sound_intern.h"

#include "common/macresman.h"

#include "audio/midiparser.h"

namespace Kyra {

SoundMac::SoundMac(KyraEngine_v1 *vm, Audio::Mixer *mixer, MidiDriver *driver)
    : Sound(vm, mixer), _driver(driver), _musicFile(0), _sources(),
      _currentEffectMap(0), _currentEffectMapSize(0), _setupFile(0),
      _currentScoreMap(0), _currentScoreMapSize(0) {
	assert(_driver);
	memset(_sources, 0, sizeof(_sources));
}

SoundMac::~SoundMac() {
	_driver->setTimerCallback(0, 0);
	_driver->close();
	delete _driver;

	delete _sources[0];
	delete _sources[1];
}

bool SoundMac::init() {
	int ret = _driver->open();
	if (ret != MidiDriver::MERR_ALREADY_OPEN && ret != 0)
		error("Couldn't open midi driver");

	_sources[0] = new Common::MacResManager();
	assert(_sources[0]);
	if (!_sources[0]->open("Legend of Kyrandia")) {
		warning("Could not load \"Legend of Kyrandia\"");
		return false;
	}

	_sources[1] = new Common::MacResManager();
	assert(_sources[1]);
	if (!_sources[1]->open("HQ_Music.res")) {
		warning("Could not load \"HQ_Music.res\"");
		return false;
	}

	_musicFile = MidiParser::createParser_SMF();
	assert(_musicFile);

	_musicFile->setMidiDriver(_driver);
	_musicFile->setTempo(_driver->getBaseTempo());
	_musicFile->setTimerRate(_driver->getBaseTempo());

	_driver->setTimerCallback(_musicFile, MidiParser::timerCallback);

	return true;
}

void SoundMac::loadSoundFile(uint file) {
	_setupFile = file;

	if (file == kMusicIntro) {
		_currentEffectMap = _introEffectMap;
		_currentEffectMapSize = _introEffectMapSize;

		_currentScoreMap = _scoreMap;
		_currentScoreMapSize = _scoreMapSize;
	} else if (file == kMusicIngame) {
		_currentEffectMap = _gameEffectMap;
		_currentEffectMapSize = _gameEffectMapSize;

		_currentScoreMap = _scoreMap + 4;
		_currentScoreMapSize = _scoreMapSize - 4;
	} else {
		warning("SoundMac::loadSoundFile: Called for unknown file %d", file);

		_currentEffectMap = 0;
		_currentEffectMapSize = 0;

		_currentScoreMap = 0;
		_currentScoreMapSize = 0;
	}
}

void SoundMac::playTrack(uint8 track) {
	if (track == 0xFF)
		return;

	if (track == 0) {
		warning("STUB SoundMac::playTrack(%d): Stop music", track);
	} else if (track == 1) {
		beginFadeOut();
	} else if (track != 3) {
		const int realTrack = MAX<int>(track - (_setupFile == 1 ? 11 : 0), 0);

		if (realTrack >= _currentScoreMapSize) {
			warning("SoundMac::playTrack(%d): track id %d exceeds track map size %d", track, realTrack, _currentScoreMapSize);
		} else {
			bool loopFlag = false;

			if (_setupFile == kMusicIngame && realTrack < _ingameScoreLoopFlagSize)
				loopFlag = _ingameScoreLoopFlag[realTrack];

			const uint16 songFileID = _currentScoreMap[realTrack];
			Common::SeekableReadStream *songFile = queryFile(MKTAG('S', 'O', 'N', 'G'), songFileID);

			if (!songFile) {
				warning("Could not find SONG file %.3X", songFileID);
				return;
			} else if (songFile->size() < 18) {
				warning("SONG file %.3X has incorrect size %d", songFileID, songFile->size());
				delete songFile;
				return;
			}

			// TODO: Proper SONG handling
			dumpSONGInfo(songFile);

			const uint16 midiFileID = songFile->readUint16BE();
			delete songFile;

			Common::SeekableReadStream *midiFile = queryFile(MKTAG('M', 'I', 'D', 'I'), midiFileID);

			if (!midiFile) {
				warning("Could not find MIDI file %.3X", midiFileID);
				return;
			}

			const uint32 midiFileSize = midiFile->size();

			uint8 *file = new uint8[midiFileSize];
			assert(file);
			midiFile->read(file, midiFileSize);
			delete midiFile;

			_musicFile->loadMusic(file, midiFileSize);
			_musicFile->property(MidiParser::mpAutoLoop, loopFlag ? 1 : 0);
			_musicFile->setTrack(0);
		}
	}
}

void SoundMac::haltTrack() {
	_musicFile->stopPlaying();
}

void SoundMac::playSoundEffect(uint8 track) {
	warning("STUB SoundMac::playSoundEffect(%d)", track);
}

void SoundMac::beginFadeOut() {
	warning("STUB SoundMac::beginFadeOut()");
}

Common::SeekableReadStream *SoundMac::queryFile(const uint32 type, const uint16 id) {
	Common::SeekableReadStream *result = 0;

	for (int i = 0; i < ARRAYSIZE(_sources) && !result; ++i)
		result = _sources[i]->getResource(type, id);

	return result;
}

void SoundMac::dumpSONGInfo(Common::SeekableReadStream *file) {
	const uint16 midiID = file->readUint16BE();
	const byte leadInstrNr = file->readByte();
	const byte bufferAhead = file->readByte();
	const uint16 tempo = file->readUint16BE();
	const int16 songPitchShift = file->readUint16BE();
	const byte sfxExtraChannels = file->readByte();
	const byte maxNotes = file->readByte();
	const uint16 maxNormNotes = file->readUint16BE();
	const byte flags1 = file->readByte();
	const byte noteDecayExt = file->readByte();
	const byte songEcho = file->readByte();
	const byte flags2 = file->readByte();
	const uint16 instrRemaps = file->readUint16BE();

	debug("MIDI ID: 0x%.4X", midiID);
	debug("Lead instrument \"INST\" ID: 0x%.2X", leadInstrNr);
	debug("Buffer ahead (half-seconds): %d", bufferAhead);
	debug("Tempo (or 0, default 16667) < slower, > faster: %d", tempo);
	debug("Song pitch shift (12 is up an octave, -12 is down an octave): %d", songPitchShift);
	debug("Extra channels for sound effects: %d", sfxExtraChannels);
	debug("Max Notes: %d", maxNotes);
	debug("Max Norm Notes: %d", maxNormNotes);

	debug("Flags 1 (0x%.2X):", flags1);
	if (flags1 & 0x80)
		debug("\tDebug song?");
	if (flags1 & 0x40)
		debug("\tTerminate decaying notes early when exceeding Max Norm Notes?");
	if (flags1 & 0x20)
		debug("\tNote interpolate whole song?");
	if (flags1 & 0x10)
		debug("\tNote interpolate lead instrument?");
	if (flags1 & 0x08)
		debug("\tUse file's track #s instead of MIDI channel #s for default INST settings?");
	if (flags1 & 0x04)
		debug("\tEnable MIDI Program Change for INST settings?");
	if (flags1 & 0x02)
		debug("\tDisable note click removal?");
	if (flags1 & 0x01)
		debug("\tUse Lead INST # for all voices?");

	debug("Note decay extension (in 1/60ths): %d", noteDecayExt);
	debug("Song echo in 1/60ths of a second (0 for no echo at all) * 22 KHz mono option only: %d", songEcho);

	debug("Flags 2 (0x%.2X):", flags2);
	if (flags2 & 0x80)
		debug("\tReduce echo to 1/2?");
	if (flags2 & 0x40)
		debug("\tReduce echo to 1/4?");
	if (flags2 & 0x20)
		debug("\tInterpolate output buffer when using 11 KHz driver?");
	if (flags2 & 0x10)
		debug("\tMaster enable: inst. pitch randomness");
	if (flags2 & 0x08)
		debug("\tScale lead INST when amplitude scaling enabled?");
	if (flags2 & 0x04)
		debug("\tForce all INSTs to use amplitude scaling if Master enable set?");
	if (flags2 & 0x02)
		debug("\tMaster enable: allow note amplitude scaling?");
	if (flags2 & 0x01)
		debug("\tStereo performance?");

	debug("INST Remaps: %d", instrRemaps);

	for (int i = 0; i < instrRemaps; ++i) {
		const uint16 instrument = file->readUint16BE();
		const uint16 instFile = file->readUint16BE();

		debug("%d -> INST 0x%.4X", instrument, instFile);
	}

	file->seek(0, SEEK_SET);
}

// Static data

const uint16 SoundMac::_introEffectMap[] = {
	0x1B58, 0x1B59, 0x1B5A, 0x1B5B, 0x1B5C, 0x1B5D, 0x1B5E, 0x1B5F,
	0x1B60, 0x1B61, 0x1B62, 0x1B63, 0x1B64, 0x1B65, 0x1B66, 0x1B67,
	0x1B68, 0x1B69, 0x1B6A, 0x1B6D, 0x1B6C, 0x1B7A, 0x1BBC, 0x1BBD,
	0x1BBE, 0x1B71, 0x1B72, 0x1B73, 0x1B74, 0x1B75, 0x1B76, 0x1B77,
	0x1B78, 0x1B79, 0x1B7A, 0x1B7B, 0x1B7C, 0x1B7D, 0x1B7E
};

const int SoundMac::_introEffectMapSize = ARRAYSIZE(SoundMac::_introEffectMap);

const uint16 SoundMac::_gameEffectMap[] = {
	0x1B58, 0x1B59, 0x1B5A, 0x1B5B, 0x1B5C, 0x1B5D, 0x1B5E, 0x1B5F,
	0x1B60, 0x1B61, 0x1B62, 0x1B63, 0x1B64, 0x1B65, 0x1B66, 0x1B67,
	0x1B68, 0x1B69, 0x1B6A, 0x1B6B, 0x1B6C, 0x1B6D, 0x1B6E, 0x1B6F,
	0x1B70, 0x1B71, 0x1B72, 0x1B73, 0x1B74, 0x1B75, 0x1B76, 0x1B77,
	0x1B78, 0x1B8A, 0x1B7A, 0x1B7B, 0x1B7C, 0x1B7D, 0x1B7E
};

const int SoundMac::_gameEffectMapSize = ARRAYSIZE(SoundMac::_gameEffectMap);

const uint16 SoundMac::_scoreMap[] = {
	0x0C8, 0x0C9, 0x0CA, 0x0CB,
	
	0x064, 0x065, 0x066, 0x067, 0x068, 0x069, 0x06A, 0x06B,
	0x06C, 0x06D, 0x06E, 0x06F, 0x070, 0x071, 0x072, 0x073,
	0x074, 0x075, 0x076, 0x077, 0x078, 0x079, 0x07A, 0x1F4,
	0x1F5, 0x1F6, 0x1F7, 0x1F8, 0x1F9, 0x1FA, 0x1FB, 0x1FC,
	0x1FD, 0x1FE, 0x1FF
};

const int SoundMac::_scoreMapSize = ARRAYSIZE(SoundMac::_scoreMap);

const bool SoundMac::_ingameScoreLoopFlag[] = {
	false, false, false, false, false, false, false, false,
	false, false,  true,  true,  true, false,  true,  true,
	false, false, false,  true, false,  true, false,  true,
	 true,  true,  true,  true,  true,  true,  true,  true,
	 true, false, false
};

const int SoundMac::_ingameScoreLoopFlagSize = ARRAYSIZE(SoundMac::_ingameScoreLoopFlag);

} // End of namespace Kyra
