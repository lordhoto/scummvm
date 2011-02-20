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

#ifndef KYRA_SOUND_INTERN_H
#define KYRA_SOUND_INTERN_H

#include "kyra/sound.h"
#include "kyra/sound_adlib.h"

#include "common/mutex.h"

#include "audio/softsynth/fmtowns_pc98/towns_pc98_driver.h"
#include "audio/softsynth/fmtowns_pc98/towns_euphony.h"

#include "audio/softsynth/emumidi.h"
#include "audio/midiparser.h"

namespace Audio {
class PCSpeaker;
class MaxTrax;
} // End of namespace Audio

namespace Common {
class MacResManager;
} // End of namespace Common

namespace Kyra {
class MidiOutput;

/**
 * MIDI output device.
 *
 * This device supports both MT-32 MIDI, as used in
 * Kyrandia 1 and 2, and GM MIDI, as used in Kyrandia 2.
 */
class SoundMidiPC : public Sound {
public:
	SoundMidiPC(KyraEngine_v1 *vm, Audio::Mixer *mixer, MidiDriver *driver, kType type);
	~SoundMidiPC();

	kType getMusicType() const { return _type; }

	bool init();

	void updateVolumeSettings();

	void loadSoundFile(uint file);
	void loadSoundFile(Common::String file);
	void loadSfxFile(Common::String file);

	void playTrack(uint8 track);
	void haltTrack();
	bool isPlaying() const;

	void playSoundEffect(uint8 track);
	void stopAllSoundEffects();

	void beginFadeOut();
private:
	static void onTimer(void *data);

	// Our channel handling
	int _musicVolume, _sfxVolume;

	uint32 _fadeStartTime;
	bool _fadeMusicOut;

	// Midi file related
	Common::String _mFileName, _sFileName;
	byte *_musicFile, *_sfxFile;

	MidiParser *_music;
	MidiParser *_sfx[3];

	// misc
	kType _type;
	Common::String getFileName(const Common::String &str);

	bool _nativeMT32;
	MidiDriver *_driver;
	MidiOutput *_output;

	Common::Mutex _mutex;
};

class SoundTowns : public Sound {
public:
	SoundTowns(KyraEngine_v1 *vm, Audio::Mixer *mixer);
	~SoundTowns();

	kType getMusicType() const { return kTowns; }

	bool init();
	void process();

	void loadSoundFile(uint file);
	void loadSoundFile(Common::String) {}

	void playTrack(uint8 track);
	void haltTrack();

	void playSoundEffect(uint8);
	void stopAllSoundEffects();

	void beginFadeOut();

	void updateVolumeSettings();

private:
	bool loadInstruments();
	void playEuphonyTrack(uint32 offset, int loop);

	void fadeOutSoundEffects();

	int _lastTrack;
	Audio::AudioStream *_currentSFX;
	Audio::SoundHandle _sfxHandle;

	uint8 *_musicTrackData;

	uint _sfxFileIndex;
	uint8 *_sfxFileData;
	uint8 _sfxChannel;

	TownsEuphonyDriver *_driver;
	
	Common::Mutex _mutex;

	bool _cdaPlaying;

	const uint8 *_musicFadeTable;
	const uint8 *_sfxBTTable;
	const uint8 *_sfxWDTable;
};

class SoundPC98 : public Sound {
public:
	SoundPC98(KyraEngine_v1 *vm, Audio::Mixer *mixer);
	~SoundPC98();

	virtual kType getMusicType() const { return kPC98; }

	bool init();

	void process() {}
	void loadSoundFile(uint file);
	void loadSoundFile(Common::String file);

	void playTrack(uint8 track);
	void haltTrack();
	void beginFadeOut();

	int32 voicePlay(const char *file, Audio::SoundHandle *handle, uint8 volume, bool isSfx) { return -1; }
	void playSoundEffect(uint8);

	void updateVolumeSettings();

protected:
	int _lastTrack;
	uint8 *_musicTrackData;
	uint8 *_sfxTrackData;
	TownsPC98_AudioDriver *_driver;
};

class SoundTownsPC98_v2 : public Sound {
public:
	SoundTownsPC98_v2(KyraEngine_v1 *vm, Audio::Mixer *mixer);
	~SoundTownsPC98_v2();

	kType getMusicType() const { return _vm->gameFlags().platform == Common::kPlatformFMTowns ? kTowns : kPC98; }

	bool init();
	void process();

	void loadSoundFile(uint file) {}
	void loadSoundFile(Common::String file);

	void playTrack(uint8 track);
	void haltTrack();
	void beginFadeOut();

	int32 voicePlay(const char *file, Audio::SoundHandle *handle, uint8 volume, bool isSfx);
	void playSoundEffect(uint8 track);

	void updateVolumeSettings();

protected:
	Audio::AudioStream *_currentSFX;
	int _lastTrack;
	bool _useFmSfx;

	uint8 *_musicTrackData;
	uint8 *_sfxTrackData;
	TownsPC98_AudioDriver *_driver;
};

// PC Speaker MIDI driver
class MidiDriver_PCSpeaker : public MidiDriver_Emulated {
public:
	MidiDriver_PCSpeaker(Audio::Mixer *mixer);
	~MidiDriver_PCSpeaker();

	// MidiDriver interface
	virtual void close() {}

	virtual void send(uint32 data);

	virtual MidiChannel *allocateChannel() { return 0; }
	virtual MidiChannel *getPercussionChannel() { return 0; }

	// MidiDriver_Emulated interface
	void generateSamples(int16 *buffer, int numSamples);

	// AudioStream interface
	bool isStereo() const { return false; }
	int getRate() const { return _rate; }
private:
	Common::Mutex _mutex;
	Audio::PCSpeaker *_speaker;
	int _rate;

	struct Channel {
		uint8 pitchBendLow, pitchBendHigh;
		uint8 hold;
		uint8 modulation;
		uint8 voiceProtect;
		uint8 noteCount;
	} _channel[2];

	void resetController(int channel);

	struct Note {
		bool enabled;
		uint8 hardwareChannel;
		uint8 midiChannel;
		uint8 note;
		bool processHold;
		uint8 flags;
		uint8 hardwareFlags;
		uint16 priority;
		int16 modulation;
		uint16 precedence;
	} _note[2];

	void noteOn(int channel, int note);
	void noteOff(int channel, int note);

	void turnNoteOn(int note);
	void overwriteNote(int note);
	void turnNoteOff(int note);

	void setupTone(int note);

	uint16 _countdown;
	uint8 _hardwareChannel[1];
	bool _modulationFlag;

	uint8 _timerValue;
	void onTimer();

	static const uint8 _noteTable1[];
	static const uint8 _noteTable2[];
};

// for StaticResource (maybe we can find a nicer way to handle it)
struct AmigaSfxTable {
	uint8 note;
	uint8 patch;
	uint16 duration;
	uint8 volume;
	uint8 pan;
};

class SoundAmiga : public Sound {
public:
	SoundAmiga(KyraEngine_v1 *vm, Audio::Mixer *mixer);
	~SoundAmiga();

	kType getMusicType() const { return kAmiga; }

	bool init();

	void process() {}
	void loadSoundFile(uint file);
	void loadSoundFile(Common::String) {}

	void playTrack(uint8 track);
	void haltTrack();
	void beginFadeOut();

	int32 voicePlay(const char *file, Audio::SoundHandle *handle, uint8 volume, bool isSfx) { return -1; }
	void playSoundEffect(uint8);

protected:
	Audio::MaxTrax *_driver;
	Audio::SoundHandle _musicHandle;
	enum FileType { kFileNone = -1, kFileIntro = 0, kFileGame = 1, kFileFinal = 2 } _fileLoaded;

	const AmigaSfxTable *_tableSfxIntro;
	int _tableSfxIntro_Size;

	const AmigaSfxTable *_tableSfxGame;
	int _tableSfxGame_Size;
};

class SoundMac : public Sound {
public:
	SoundMac(KyraEngine_v1 *vm, Audio::Mixer *mixer, MidiDriver *driver);
	~SoundMac();

	kType getMusicType() const { return kMidiGM; }

	bool init();

	void loadSoundFile(uint file);
	void loadSoundFile(Common::String file) {}

	void playTrack(uint8 track);
	void haltTrack();

	void playSoundEffect(uint8 track);

	void beginFadeOut();

private:
	Common::SeekableReadStream *queryFile(const uint32 type, const uint16 id);

	MidiDriver *_driver;
	MidiParser *_musicFile;

	static void dumpSONGInfo(Common::SeekableReadStream *file);

	Common::MacResManager *_sources[2];

	static const uint16 _introEffectMap[];
	static const int _introEffectMapSize;

	static const uint16 _gameEffectMap[];
	static const int _gameEffectMapSize;

	const uint16 *_currentEffectMap;
	int _currentEffectMapSize;

	static const uint16 _scoreMap[];
	static const int _scoreMapSize;

	static const bool _ingameScoreLoopFlag[];
	static const int _ingameScoreLoopFlagSize;

	uint _setupFile;

	const uint16 *_currentScoreMap;
	int _currentScoreMapSize;
};

} // End of namespace Kyra

#endif
