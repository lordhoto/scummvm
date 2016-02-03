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
 */

#ifndef AGI_SYSTEMUI_H
#define AGI_SYSTEMUI_H

namespace Agi {

#define SYSTEMUI_SAVEDGAME_MAXIMUM_SLOTS 100
#define SYSTEMUI_SAVEDGAME_SLOTS_ON_SCREEN 12
#define SYSTEMUI_SAVEDGAME_DESCRIPTION_LEN 30
#define SYSTEMUI_SAVEDGAME_DISPLAYTEXT_LEN 31
#define SYSTEMUI_SAVEDGAME_DISPLAYTEXT_PREFIX_LEN 3

struct SystemUISavedGameEntry {
	int16 slotId;
	bool  exists;
	bool  isValid;
	char description[SYSTEMUI_SAVEDGAME_DESCRIPTION_LEN + 1]; // actual description
	char displayText[SYSTEMUI_SAVEDGAME_DISPLAYTEXT_LEN + 1]; // modified description, meant for display purposes only
};
typedef Common::Array<SystemUISavedGameEntry> SystemUISavedGameArray;

class SystemUI {
public:
	SystemUI(AgiEngine *vm, GfxMgr *gfx, TextMgr *text);
	~SystemUI();

private:
	AgiEngine *_vm;
	GfxMgr *_gfx;
	TextMgr *_text;

public:
	const char *getStatusTextScore();
	const char *getStatusTextSoundOn();
	const char *getStatusTextSoundOff();

	void pauseDialog();
	bool restartDialog();
	bool quitDialog();

	const char *getInventoryTextNothing();
	const char *getInventoryTextYouAreCarrying();
	const char *getInventoryTextSelectItems();
	const char *getInventoryTextReturnToGame();

	int16 figureOutAutomaticSaveGameSlot(const char *automaticSaveDescription);
	int16 figureOutAutomaticRestoreGameSlot(const char *automaticSaveDescription);

	int16 askForSaveGameSlot();
	int16 askForRestoreGameSlot();
	bool  askForSaveGameDescription(int16 slotId, Common::String &newDescription);

	void  savedGameSlot_KeyPress(uint16 newKey);

private:
	int16 askForSavedGameSlot(const char *slotListText);
	bool  askForSavedGameVerification(const char *verifyText, const char *actualDescription, int16 slotId);

	void  createSavedGameDisplayText(char *destDisplayText, const char *actualDescription, int16 slotId, bool fillUpWithSpaces);
	void  clearSavedGameSlots();
	void  readSavedGameSlots(bool filterNonexistant, bool withAutoSaveSlot);
	void  figureOutAutomaticSavedGameSlot(const char *automaticSaveDescription, int16 &matchedGameSlotId, int16 &freshGameSlotId);

	void  drawSavedGameSlots();
	void  drawSavedGameSlotSelector(bool active);

	SystemUISavedGameArray _savedGameArray;
	int16 _savedGameUpmostSlotNr;
	int16 _savedGameSelectedSlotNr;

private:
	const char *_textStatusScore;
	const char *_textStatusSoundOn;
	const char *_textStatusSoundOff;

	const char *_textPause;
	const char *_textRestart;
	const char *_textQuit;

	const char *_textInventoryNothing;
	const char *_textInventoryYouAreCarrying;
	const char *_textInventorySelectItems;
	const char *_textInventoryReturnToGame;

	const char *_textSaveGameSelectSlot;
	const char *_textSaveGameEnterDescription;
	const char *_textSaveGameVerify;

	const char *_textRestoreGameNoSlots;
	const char *_textRestoreGameSelectSlot;
	const char *_textRestoreGameError;
	const char *_textRestoreGameVerify;
};

} // End of namespace Agi

#endif /* AGI_SYSTEMUI_H */