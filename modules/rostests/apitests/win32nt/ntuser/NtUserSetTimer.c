/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for NtUserSetTimer
 * PROGRAMMERS:
 */

#include "../win32nt.h"

UINT_PTR timerId;

void CALLBACK TimerProc(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime) {
    std::cout << "Timer event triggered: " << idEvent << std::endl;
    // Můžete přidat další logiku zde
}

bool Test_SetAndKillTimer() {
    timerId = SetTimer(NULL, 0, 1000, (TIMERPROC)TimerProc);
    if (timerId == 0) {
        std::cerr << "Failed to set timer." << std::endl;
        return false;
    } else {
        std::cout << "Timer set successfully: " << timerId << std::endl;
    }

    Sleep(2000);

    if (KillTimer(NULL, timerId) == 0) {
        std::cerr << "Failed to kill timer." << std::endl;
        return false;
    } else {
        std::cout << "Timer killed successfully: " << timerId << std::endl;
    }

    return true;
}

void MessageLoop() {
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

START_TEST(NtUserSetTimer)
{
	// Test 1: one timer
    UINT_PTR timerId = SetTimer(NULL, 0, 1000, (TIMERPROC)TimerProc); // Nastaví časovač na 1 sekundu
    if (timerId == 0) {
        std::cerr << "Failed to set timer." << std::endl;
        return 1;
    } else {
        std::cout << "Timer set successfully: " << timerId << std::endl;
    }
    Sleep(2000);
    if (KillTimer(NULL, timerId) == 0) {
        std::cerr << "Failed to kill timer." << std::endl;
        return 1;
    } else {
        std::cout << "Timer killed successfully: " << timerId << std::endl;
    }
	
	// Test 2:
	if (!Test_SetAndKillTimer()) {
        return 1;
    }

    // Hlavní smyčka zpráv
    std::cout << "Entering message loop. Press Ctrl+C to exit." << std::endl;
    MessageLoop();

    return 0;

}
