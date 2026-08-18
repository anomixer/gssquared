#include "util/MenuInterface.h"
#include "gs2.hpp"
#include "NClock.hpp"
#include "videosystem.hpp"
#include "platform-specific/menu.h"
#include "debugger/debugwindow.hpp"
#include "computer.hpp"
#include "util/AudioSystem.hpp"
#include "util/mount.hpp"
#include "util/SystemSettings.hpp"
#include "devices/game/gamecontroller.hpp"
#include "Module_ID.hpp"
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

static void pushMenuEvent(Sint32 code) {
	SDL_Event event = {};
	event.type = gs2_app_values.menu_event_type;
	event.user.code = code;
	SDL_PushEvent(&event);
}

void MenuInterface::machineReset()       { pushMenuEvent(MENU_MACHINE_RESET); }
void MenuInterface::machineRestart()     { pushMenuEvent(MENU_MACHINE_RESTART); }
void MenuInterface::machinePauseResume() { pushMenuEvent(MENU_MACHINE_PAUSE_RESUME); }
void MenuInterface::machineCaptureMouse(){ pushMenuEvent(MENU_MACHINE_CAPTURE_MOUSE); }

void MenuInterface::setSpeed(int speed_id) {
	switch (speed_id) {
		case SPEED_1_0:  pushMenuEvent(MENU_SPEED_1_0); break;
		case SPEED_2_8:  pushMenuEvent(MENU_SPEED_2_8); break;
		case SPEED_7_1:  pushMenuEvent(MENU_SPEED_7_1); break;
		case SPEED_14_3: pushMenuEvent(MENU_SPEED_14_3); break;
	}
}

void MenuInterface::setMonitor(int monitor_id) {
	pushMenuEvent(monitor_id);
}

void MenuInterface::openDebugWindow() { pushMenuEvent(MENU_OPEN_DEBUG_WINDOW); }

void MenuInterface::diskToggle(storage_key_t key) {
	SDL_Event event = {};
	event.type = gs2_app_values.menu_event_type;
	event.user.code = MENU_DISK_TOGGLE;
	// Do not cast storage_key_t::key directly to a pointer. On wasm32 the
	// high 32 bits (including the slot) are truncated, so File -> Drives sends
	// the wrong storage key while the in-emulator drive drawer still works.
	const uintptr_t packed = (static_cast<uintptr_t>(key.slot) << 16)
	                       | static_cast<uintptr_t>(key.drive);
	event.user.data1 = reinterpret_cast<void*>(packed);
	SDL_PushEvent(&event);
}

void MenuInterface::openSystemConfig() { pushMenuEvent(MENU_OPEN_CONFIG); }

void MenuInterface::displayFullScreen() { pushMenuEvent(MENU_DISPLAY_FULLSCREEN); }
void MenuInterface::editCopyScreen()   { pushMenuEvent(MENU_EDIT_COPY_SCREEN); }
void MenuInterface::editPasteText()    { pushMenuEvent(MENU_EDIT_PASTE_TEXT); }
void MenuInterface::fileSaveScreenshot() { pushMenuEvent(MENU_FILE_SAVE_SCREENSHOT); }
void MenuInterface::toggleMountDrivers() { pushMenuEvent(MENU_FILE_MOUNT_DRIVERS); }

void MenuInterface::toggleSleepMode() {
	gs2_app_values.sleep_mode = !gs2_app_values.sleep_mode;
}

void MenuInterface::toggleAudioDecorrelation() {
	if (computer_ && computer_->audio_system) computer_->audio_system->toggle_decorrelation();
}

void MenuInterface::toggleRightMouseAccel() {
	gs2_app_values.right_mouse_accelerate = !gs2_app_values.right_mouse_accelerate;
}

void MenuInterface::toggleCrtShader() {
	if (computer_ && computer_->video_system) computer_->video_system->toggle_crt_shader();
}

void MenuInterface::toggleHudStats() {
	SystemSettings::instance().toggle_hud_stats();
}

void MenuInterface::toggleHudDrives() {
	SystemSettings::instance().toggle_hud_drives();
}

void MenuInterface::toggleDisconnectedWhenNoGamepad() {
	SystemSettings::instance().toggle_disconnected_when_no_gamepad();
}

void MenuInterface::toggleSsTextMode() {
	SystemSettings::instance().toggle_ss_text_mode();
	syncSsTextCanvasAspect();
}

void MenuInterface::syncSsTextCanvasAspect() {
	const bool on = SystemSettings::instance().ss_text_mode();
#ifdef __EMSCRIPTEN__
	// SS mode keeps the emulator canvas at 1288x928. The native 720x400 VGA
	// image is fitted inside that surface by the renderer; explicitly setting
	// both dimensions avoids a large viewport stretching only one axis.
	EM_ASM({
		var c = document.querySelector('#canvas');
		if (!c) { return; }
		if ($0) {
			c.style.aspectRatio = '1288 / 928';
			window.gssquaredSsTextMode = true;
			if (window.gssquaredResizeSsCanvas) window.gssquaredResizeSsCanvas();
		} else {
			window.gssquaredSsTextMode = false;
			c.style.aspectRatio = '1288 / 928';
			c.style.removeProperty('width');
			c.style.removeProperty('height');
			c.style.removeProperty('max-width');
			c.style.removeProperty('max-height');
		}
		// Force layout and let SDL re-measure the new CSS size.
		var _forceReflow = c.offsetWidth;
		window.dispatchEvent(new Event('resize'));
		setTimeout(function() { window.dispatchEvent(new Event('resize')); }, 16);
		setTimeout(function() { window.dispatchEvent(new Event('resize')); }, 50);
		setTimeout(function() { window.dispatchEvent(new Event('resize')); }, 100);
		setTimeout(function() { window.dispatchEvent(new Event('resize')); }, 200);
	}, on ? 1 : 0);

	video_system_t *vs = computer_ ? computer_->video_system : nullptr;
	if (vs) {
		// Keep the SS canvas at the normal 1288x928 emulator surface. The
		// 720x400 source is fitted inside it by SecondSight::frame().
		vs->set_target_aspect(0.0f);
	}
#else
	(void)on;
#endif
}

int MenuInterface::getCurrentSpeed() {
	if (!computer_ || !computer_->clock) return -1;
	return (int)computer_->clock->get_clock_mode();
}

int MenuInterface::getCurrentMonitor() {
	if (!computer_) return -1;
	video_system_t *vs = computer_->video_system;
	if (!vs) return -1;

	if (vs->display_color_engine == DM_ENGINE_NTSC) return MONITOR_COMPOSITE;
	if (vs->display_color_engine == DM_ENGINE_RGB)  return MONITOR_GS_RGB;

	switch (vs->display_mono_color) {
		case DM_MONO_GREEN: return MONITOR_MONO_GREEN;
		case DM_MONO_AMBER: return MONITOR_MONO_AMBER;
		default:            return MONITOR_MONO_WHITE;
	}
}

bool MenuInterface::getSleepMode() {
	return gs2_app_values.sleep_mode;
}

bool MenuInterface::getAudioDecorrelation() {
	return computer_ && computer_->audio_system ? computer_->audio_system->get_decorrelation() : false;
}

bool MenuInterface::getRightMouseAccel() {
	return gs2_app_values.right_mouse_accelerate;
}

bool MenuInterface::getCrtShader() {
	return computer_ && computer_->video_system ? computer_->video_system->get_crt_shader_enabled() : false;
}

bool MenuInterface::getCrtShaderAvailable() {
	return computer_ && computer_->video_system ? computer_->video_system->crt_shader_available() : false;
}

bool MenuInterface::getHudStats() {
	return SystemSettings::instance().hud_stats();
}

bool MenuInterface::getHudDrives() {
	return SystemSettings::instance().hud_drives();
}

bool MenuInterface::getDisconnectedWhenNoGamepad() {
	return SystemSettings::instance().disconnected_when_no_gamepad();
}

bool MenuInterface::getSsTextMode() {
	return SystemSettings::instance().ss_text_mode();
}

bool MenuInterface::isEmulationRunning() {
	return computer_ != nullptr;
}

bool MenuInterface::hasBazFast() {
	return computer_ && computer_->has_bazfast();
}

bool MenuInterface::hasSecondSight() {
	return computer_ && computer_->has_second_sight();
}

bool MenuInterface::getMountDrivers() {
	return computer_ && computer_->is_drivers_mounted();
}

bool MenuInterface::isPaused() {
	return computer_ && computer_->execution_mode == EXEC_PAUSED;
}

bool MenuInterface::isMouseCaptured() {
	return SDL_GetWindowRelativeMouseMode(computer_->video_system->window);
}

void MenuInterface::setControllerMode(int mode) {
	switch (mode) {
		case JOYSTICK_APPLE_GAMEPAD: pushMenuEvent(MENU_CONTROLLER_GAMEPAD); break;
		case JOYSTICK_APPLE_MOUSE:   pushMenuEvent(MENU_CONTROLLER_MOUSE);   break;
		case JOYSTICK_ATARI_DPAD:    pushMenuEvent(MENU_CONTROLLER_JOYPORT); break;
	}
}

int MenuInterface::getCurrentControllerMode() {
	if (!computer_) return JOYSTICK_APPLE_GAMEPAD;
	gamec_state_t *gc = (gamec_state_t *)computer_->get_module_state(MODULE_GAMECONTROLLER);
	if (!gc) return JOYSTICK_APPLE_GAMEPAD;
	return (int)get_joystick_mode(gc);
}

std::vector<MenuDriveInfo> MenuInterface::getDriveList() {
	std::vector<MenuDriveInfo> result;
	if (!computer_ || !computer_->mounts) return result;

	for (const drive_info_t &info : computer_->mounts->get_all_drives()) {
		MenuDriveInfo mdi;
		mdi.key               = info.key;
		mdi.is_mounted        = info.status.is_mounted;
		mdi.is_modified       = info.status.is_modified;
		mdi.is_write_protected = info.status.is_write_protected;
		mdi.filename          = info.status.filename;
		result.push_back(mdi);
	}
	return result;
}

static MenuInterface sInstance;

MenuInterface *getMenuInterface() {
	return &sInstance;
}
