/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for GetDCEx
 * PROGRAMMERS:     Timo Kreuzer
 */

#include "precomp.h"

#define DCX_USESTYLE 0x00010000

#define DCX_MYFLAG 0x00200000

void Test_GetDCEx_Params()
{

}

static
LRESULT
CALLBACK
WndProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    return TRUE;
}

static
ATOM
RegisterClassHelper(
    PSTR pszClassName,
    UINT style,
    WNDPROC pfnWndProc)
{
    WNDCLASSA cls;

    cls.style = style;
    cls.lpfnWndProc = pfnWndProc;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(0);
    cls.hIcon = 0;
    cls.hCursor = LoadCursorA(0, IDC_ARROW);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = pszClassName;

    return RegisterClassA(&cls);
}

int indexW=-1;

static
HWND
CreateWindowHelper(
    PSZ pszClassName,
    PSZ pszTitle)
{
    indexW++;
    return CreateWindowA(pszClassName, pszTitle, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                         (indexW * 100),
                         (indexW * 100),
                         100,
                         100,
                         NULL,
                         NULL,
                         0,
                         NULL);
}

static
void
Test_GetDCEx_Cached()
{
    static const PSTR pszClassName = "TestClass_Cached";
    ATOM atomClass;
    HWND hwnd;
    HDC hdc1, hdc2;
    HRGN hrgn;

    atomClass = RegisterClassHelper(pszClassName, 0, WndProc);
    ok(atomClass != 0, "Failed to register class\n");

    hwnd = CreateWindowHelper(pszClassName, "Test Window1");
    ok(hwnd != NULL, "Failed to create hwnd\n");

    hdc1 = GetDCEx(hwnd, 0, 0);
    ok(hdc1 == NULL, "GetDCEx should fail\n");
    hrgn = CreateRectRgn(0, 0, 100, 100);
    hdc1 = GetDCEx(hwnd, hrgn, 0);
    ok(hdc1 == NULL, "GetDCEx should fail\n");

    hdc1 = GetDCEx(hwnd, 0, DCX_WINDOW);
    ok(hdc1 == NULL, "GetDCEx should fail\n");
    hdc1 = GetDCEx(hwnd, hrgn, DCX_WINDOW);
    ok(hdc1 == NULL, "GetDCEx should fail\n");

    hdc1 = GetDCEx(hwnd, hrgn, DCX_INTERSECTRGN);
    ok(hdc1 == NULL, "GetDCEx should fail\n");

    hdc1 = GetDCEx(hwnd, hrgn, DCX_PARENTCLIP);
    ok(hdc1 != NULL, "GetDCEx failed\n");
    ReleaseDC(hwnd, hdc1);

    hdc1 = GetDCEx(hwnd, hrgn, DCX_WINDOW | DCX_INTERSECTRGN | DCX_PARENTCLIP);
    ok(hdc1 != NULL, "GetDCEx failed\n");
    ReleaseDC(hwnd, hdc1);

    hdc1 = GetDCEx(hwnd, 0, DCX_CACHE);
    ok(hdc1 != NULL, "GetDCEx failed\n");
    ReleaseDC(hwnd, hdc1);

    hrgn = CreateRectRgn(0, 0, 100, 100);
    hdc2 = GetDCEx(hwnd, hrgn, DCX_CACHE);
    ok(hdc2 != NULL, "GetDCEx failed\n");
    ReleaseDC(hwnd, hdc2);
    ok(hdc2 == hdc1, "Expected the same DC\n");

    hdc1 = GetDCEx(hwnd, 0, DCX_CACHE);
    hdc2 = GetDCEx(hwnd, hrgn, DCX_CACHE);
    ok(hdc1 != NULL, "GetDCEx failed\n");
    ok(hdc2 != hdc1, "Expected a different DC\n");
    ReleaseDC(hwnd, hdc1);
    ReleaseDC(hwnd, hdc2);

    hdc1 = GetDCEx(NULL, NULL, 0);
    ok(hdc1 != NULL, "GetDCEx failed\n");
    hdc2 = GetDCEx(NULL, NULL, 0);
    ok(hdc2 != NULL, "GetDCEx failed\n");
    ok(hdc2 != hdc1, "Expected a different DC\n");
    ReleaseDC(hwnd, hdc1);
    ReleaseDC(hwnd, hdc2);

    ok(CombineRgn(hrgn, hrgn, hrgn, RGN_OR) == SIMPLEREGION, "region is not valid");

    DestroyWindow(hwnd);
    ok(UnregisterClass(pszClassName, GetModuleHandleA(0)) == TRUE,
       "UnregisterClass failed");
}

static
void
Test_GetDCEx_CS_OWNDC()
{
    static const PSTR pszClassName = "TestClass_CS_OWNDC";
    ATOM atomClass;
    HWND hwnd;
    HDC hdc1, hdc2;
    //HRGN hrgn;

    atomClass = RegisterClassHelper(pszClassName, CS_OWNDC, WndProc);
    ok(atomClass != 0, "Failed to register class\n");

    hwnd = CreateWindowHelper(pszClassName, "Test Window1");
    ok(hwnd != NULL, "Failed to create hwnd\n");

    hdc1 = GetDCEx(hwnd, NULL, DCX_MYFLAG);
    ok(hdc1 != NULL, "GetDCEx failed\n");
    hdc2 = GetDCEx(hwnd, NULL, DCX_MYFLAG);
    ok(hdc2 != NULL, "GetDCEx failed\n");
    ok(hdc2 == hdc1, "Expected the same DC\n");
    ok(ReleaseDC(hwnd, hdc1) == TRUE, "ReleaseDC failed\n");
    ok(ReleaseDC(hwnd, hdc2) == TRUE, "ReleaseDC failed\n");

    hdc2 = GetDCEx(hwnd, NULL, 0);
    ok(hdc2 == hdc1, "Expected the same DC\n");
    ok(ReleaseDC(hwnd, hdc2) == TRUE, "ReleaseDC failed\n");

    hdc2 = GetDCEx(hwnd, NULL, DCX_CACHE);
    ok(hdc2 != hdc1, "Expected a different DC\n");
    ok(ReleaseDC(hwnd, hdc2) == TRUE, "ReleaseDC failed\n");

    hdc2 = GetDCEx(hwnd, NULL, DCX_WINDOW);
    ok(hdc2 == hdc1, "Expected the same DC\n");
    ok(ReleaseDC(hwnd, hdc2) == TRUE, "ReleaseDC failed\n");

    /* Try after resetting CS_OWNDC in the class */
    ok(SetClassLongPtrA(hwnd, GCL_STYLE, 0) == CS_OWNDC, "class style wrong\n");
    hdc2 = GetDCEx(hwnd, NULL, 0);
    ok(hdc2 == hdc1, "Expected the same DC, got %p\n", hdc2);
    ok(ReleaseDC(hwnd, hdc2) == TRUE, "ReleaseDC failed\n");

    /* Try after setting CS_CLASSDC in the class */
    ok(SetClassLongPtrA(hwnd, GCL_STYLE, CS_CLASSDC) == 0, "class style not set\n");
    hdc2 = GetDCEx(hwnd, NULL, 0);
    ok(hdc2 == hdc1, "Expected the same DC, got %p\n", hdc2);
    ok(ReleaseDC(hwnd, hdc2) == TRUE, "ReleaseDC failed\n");

    /* CS_OWNDC and CS_CLASSDC? Is that even legal? */
    ok(SetClassLongPtrA(hwnd, GCL_STYLE, (CS_OWNDC | CS_CLASSDC)) == CS_CLASSDC, "class style not set\n");
    hdc2 = GetDCEx(hwnd, NULL, 0);
    ok(hdc2 == hdc1, "Expected the same DC, got %p\n", hdc2);
    ok(ReleaseDC(hwnd, hdc2) == TRUE, "ReleaseDC failed\n");

    SetClassLongPtrA(hwnd, GCL_STYLE, CS_OWNDC);

    DestroyWindow(hwnd);
    ok(UnregisterClass(pszClassName, GetModuleHandleA(0)) == TRUE,
       "UnregisterClass failed");
}

static
void
Test_GetDCEx_CS_CLASSDC()
{
    static const PSTR pszClassName = "TestClass_CS_CLASSDC";
    ATOM atomClass;
    HWND hwnd1, hwnd2;
    HDC hdc1, hdc2;
    //HRGN hrgn;

    atomClass = RegisterClassHelper(pszClassName, CS_CLASSDC, WndProc);
    ok(atomClass != 0, "Failed to register class\n");

    hwnd1 = CreateWindowHelper(pszClassName, "Test Window1");
    ok(hwnd1 != NULL, "Failed to create hwnd1\n");

    /* Looks legit, but this is not the DC you are looking for!
       In fact this is NOT the class dc, but an own DC, doh!
       When the first Window is created, the DC for that Window is both it's own
       AND the class DC. But we only get the class DC, when using DCX_USESTYLE */
    hdc1 = GetDCEx(hwnd1, NULL, 0);
    ok(hdc1 != NULL, "GetDCEx failed\n");
    hdc2 = GetDCEx(hwnd1, NULL, 0);
    ok(hdc2 == hdc1, "Expected the same DC, got %p\n", hdc2);
    ok(ReleaseDC(hwnd1, hdc1) == TRUE, "ReleaseDC failed\n");
    ok(ReleaseDC(hwnd1, hdc2) == TRUE, "ReleaseDC failed\n");

    /* Now with DCX_USESTYLE */
    hdc2 = GetDCEx(hwnd1, NULL, DCX_USESTYLE);
    ok(hdc2 == hdc1, "Expected the same DC, got %p\n", hdc2);
    ok(ReleaseDC(hwnd1, hdc2) == TRUE, "ReleaseDC failed\n");

    hwnd2 = CreateWindowHelper(pszClassName, "Test Window2");
    ok(hwnd2 != NULL, "Failed to create hwnd2\n");

    /* Yeah, this doesn't work anymore. Once the */
    hdc2 = GetDCEx(hwnd2, NULL, 0);
    ok(hdc2 == NULL, "Expected failure\n");

    /* Now with DCX_USESTYLE ... */
    hdc2 = GetDCEx(hwnd2, NULL, DCX_USESTYLE);
    ok(hdc2 == hdc1, "Expected the same DC, got %p\n", hdc2);
    ok(ReleaseDC(hwnd2, hdc2) == TRUE, "ReleaseDC failed\n");

    SendMessage(hwnd2, WM_USER, 0, 0);

    DestroyWindow(hwnd1);
    DestroyWindow(hwnd2);
    ok(UnregisterClass(pszClassName, GetModuleHandleA(0)) == TRUE,
       "UnregisterClass failed");
}

static
void
Test_GetDCEx_CS_Mixed()
{
    static const PSTR pszClassName = "TestClass_CS_Mixed";
    ATOM atomClass;
    HWND hwnd1,hwnd2, hwnd3;
    HDC hdc1, hdc2, hdc3;

    /* Register a class with CS_OWNDC *and* CS_CLASSDC */
    atomClass = RegisterClassHelper(pszClassName, CS_OWNDC | CS_CLASSDC, WndProc);
    ok(atomClass != 0, "Failed to register class\n");

    /* Create the first window, this should create a single own and class DC */
    hwnd1 = CreateWindowHelper(pszClassName, "Test Window1");
    ok(hwnd1 != NULL, "Failed to create hwnd1\n");

    /* Verify that we have the right style */
    ok(GetClassLongPtrA(hwnd1, GCL_STYLE) == (CS_OWNDC | CS_CLASSDC),
       "class style not set\n");

    /* This is now the class DC and the first windows own DC */
    hdc1 = GetDCEx(hwnd1, NULL, 0);
    ok(hdc1 != NULL, "GetDCEx failed\n");
    ok(ReleaseDC(hwnd1, hdc1) == TRUE, "ReleaseDC failed\n");

    /* This should get us the own/class DC again */
    hdc2 = GetDCEx(hwnd1, NULL, 0);
    ok(hdc2 == hdc1, "Expected the own/class DC, got %p\n", hdc2);
    ok(ReleaseDC(hwnd1, hdc2) == TRUE, "ReleaseDC failed\n");

    /* This should get us the class DC, but it's the same */
    hdc2 = GetDCEx(hwnd1, NULL, DCX_USESTYLE);
    ok(hdc2 == hdc1, "Expected the own/class DC, got %p\n", hdc2);
    ok(ReleaseDC(hwnd1, hdc2) == TRUE, "ReleaseDC failed\n");

    /* Create a second window */
    hwnd2 = CreateWindowHelper(pszClassName, "Test Window1");
    ok(hwnd1 != NULL, "Failed to create hwnd1\n");

    /* This should get us the own DC of the new window */
    hdc2 = GetDCEx(hwnd2, NULL, 0);
    ok(hdc2 != NULL, "GetDCEx failed\n");
    ok(hdc2 != hdc1, "Expected different DC\n");
    ok(ReleaseDC(hwnd2, hdc2) == TRUE, "ReleaseDC failed\n");

    /* This gets us the own DC again, CS_OWNDC has priority! */
    hdc3 = GetDCEx(hwnd2, NULL, DCX_USESTYLE);
    ok(hdc3 == hdc2, "Expected the own DC, got %p\n", hdc3);
    ok(ReleaseDC(hwnd2, hdc3) == TRUE, "ReleaseDC failed\n");

    /* Disable CS_OWNDC */
    ok(SetClassLongPtrA(hwnd1, GCL_STYLE, CS_CLASSDC) == (CS_OWNDC | CS_CLASSDC), "unexpected style\n");
    ok(GetClassLongPtrA(hwnd1, GCL_STYLE) == CS_CLASSDC, "class style not set\n");

    /* Since the window already has an own DC, we get it again! */
    hdc3 = GetDCEx(hwnd2, NULL, DCX_USESTYLE);
    ok(hdc3 == hdc2, "Expected the own DC, got %p\n", hdc3);
    ok(ReleaseDC(hwnd2, hdc3) == TRUE, "ReleaseDC failed\n");

    /* Disable CS_CLASSDC, too */
    ok(SetClassLongPtrA(hwnd1, GCL_STYLE, 0) == CS_CLASSDC, "unexpected style\n");
    ok(GetClassLongPtrA(hwnd1, GCL_STYLE) == 0, "class style not set\n");

    /* With DCX_USESTYLE we only get a cached DC */
    hdc3 = GetDCEx(hwnd2, NULL, DCX_USESTYLE);
    ok(hdc3 != NULL, "GetDCEx failed\n");
    ok(hdc3 != hdc1, "Expected different DC, got class DC\n");
    ok(hdc3 != hdc2, "Expected different DC, got own DC\n");
    ok(ReleaseDC(hwnd2, hdc3) == TRUE, "ReleaseDC failed\n");

    /* Without DCX_USESTYLE we get the own DC */
    hdc3 = GetDCEx(hwnd2, NULL, 0);
    ok(hdc3 != NULL, "GetDCEx failed\n");
    ok(hdc3 != hdc1, "Expected different DC, got class DC\n");
    ok(hdc3 == hdc2, "Expected the own DC, got %p\n", hdc3);
    ok(ReleaseDC(hwnd2, hdc3) == TRUE, "ReleaseDC failed\n");

    /* Set only CS_OWNDC */
    ok(SetClassLongPtrA(hwnd1, GCL_STYLE, CS_OWNDC) == 0, "unexpected style\n");
    ok(GetClassLongPtrA(hwnd1, GCL_STYLE) == CS_OWNDC, "class style not set\n");

    hwnd3 = CreateWindowHelper(pszClassName, "Test Window1");
    ok(hwnd3 != NULL, "Failed to create hwnd1\n");

    /* This should get a new own DC */
    hdc2 = GetDCEx(hwnd3, NULL, 0);
    ok(hdc2 != hdc1, "Expected different DC\n");
    ok(ReleaseDC(hwnd3, hdc2) == TRUE, "ReleaseDC failed\n");

    /* Re-enable CS_CLASSDC */
    ok(SetClassLongPtrA(hwnd1, GCL_STYLE, (CS_OWNDC | CS_CLASSDC)) == CS_OWNDC, "unexpected style\n");
    ok(GetClassLongPtrA(hwnd1, GCL_STYLE) == (CS_OWNDC | CS_CLASSDC), "class style not set\n");

    /* This should get us the own DC */
    hdc3 = GetDCEx(hwnd3, NULL, 0);
    ok(hdc3 == hdc2, "Expected the same DC, got %p\n", hdc3);
    ok(ReleaseDC(hwnd3, hdc3) == TRUE, "ReleaseDC failed\n");

    /* This should still get us the new own DC */
    hdc3 = GetDCEx(hwnd3, NULL, DCX_USESTYLE);
    ok(hdc3 == hdc2, "Expected the same DC, got %p\n", hdc3);
    ok(ReleaseDC(hwnd3, hdc3) == TRUE, "ReleaseDC failed\n");

    /* Disable CS_OWNDC */
    ok(SetClassLongPtrA(hwnd1, GCL_STYLE, CS_CLASSDC) == (CS_OWNDC | CS_CLASSDC), "unexpected style\n");
    ok(GetClassLongPtrA(hwnd1, GCL_STYLE) == CS_CLASSDC, "class style not set\n");

    /* This should get us the own DC */
    hdc3 = GetDCEx(hwnd3, NULL, 0);
    ok(hdc3 == hdc2, "Expected the same DC, got %p\n", hdc3);
    ok(ReleaseDC(hwnd3, hdc3) == TRUE, "ReleaseDC failed\n");

    /* This should still get us the new own DC */
    hdc3 = GetDCEx(hwnd3, NULL, DCX_USESTYLE);
    ok(hdc3 == hdc2, "Expected the same DC, got %p\n", hdc3);
    ok(ReleaseDC(hwnd3, hdc3) == TRUE, "ReleaseDC failed\n");

    /* cleanup for a second run */
    DestroyWindow(hwnd1);
    DestroyWindow(hwnd2);
    DestroyWindow(hwnd3);
    ok(UnregisterClass(pszClassName, GetModuleHandleA(0)) == TRUE,
       "UnregisterClass failed\n");

    /* Create class again with CS_OWNDC */
    atomClass = RegisterClassHelper(pszClassName, CS_OWNDC, WndProc);
    ok(atomClass != 0, "Failed to register class\n");

    hwnd1 = CreateWindowHelper(pszClassName, "Test Window1");
    ok(hwnd1 != NULL, "Failed to create hwnd1\n");

    /* This is the windows own DC, the class does not have a class DC yet */
    hdc1 = GetDCEx(hwnd1, NULL, 0);
    ok(hdc1 != NULL, "GetDCEx failed\n");
    ok(ReleaseDC(hwnd1, hdc1) == TRUE, "ReleaseDC failed\n");

    /* Enable only CS_CLASSDC */
    ok(SetClassLongPtrA(hwnd1, GCL_STYLE, CS_CLASSDC) == CS_OWNDC, "unexpected style\n");
    ok(GetClassLongPtrA(hwnd1, GCL_STYLE) == CS_CLASSDC, "class style not set\n");

    /* Create a second window. Now we should create a class DC! */
    hwnd2 = CreateWindowHelper(pszClassName, "Test Window2");
    ok(hwnd2 != NULL, "Failed to create hwnd1\n");

    /* We expect a new DCE (the class DCE) */
    hdc2 = GetDCEx(hwnd2, NULL, DCX_USESTYLE);
    ok(hdc2 != NULL, "GetDCEx failed\n");
    ok(hdc2 != hdc1, "Expected different DCs\n");
    ok(ReleaseDC(hwnd2, hdc2) == TRUE, "ReleaseDC failed\n");

    /* cleanup */
    DestroyWindow(hwnd1);
    DestroyWindow(hwnd2);
    ok(UnregisterClass(pszClassName, GetModuleHandleA(0)) == TRUE,
       "UnregisterClass failed\n");
}

static
void
Test_myTests()
{
    static const PSTR pszClassName = "TestClass_CS_SwitchedStyle";
    ATOM atomClass;
    HWND hwnd1, hwnd2;
    HDC hdc1, hdc2 , hdcClass;

	///////////////my tests
	//1/
    atomClass = RegisterClassHelper(pszClassName, CS_CLASSDC, WndProc);
    hwnd1 = CreateWindowHelper(pszClassName, "Test Window1");
    //SetWindowPos(hwnd2, NULL, 100, 100, 800, 600, SWP_NOZORDER | SWP_NOMOVE);
    hwnd2 = CreateWindowHelper(pszClassName, "Test Window2");
    //SetWindowPos(hwnd1, NULL, 20, 20, 400, 300, SWP_NOZORDER | SWP_NOMOVE);
    
    ShowWindow(hwnd1, SW_SHOW);
    UpdateWindow(hwnd1);
    ShowWindow(hwnd2, SW_SHOW);
    UpdateWindow(hwnd2);

    /* MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }*/

	hdc1 = GetDCEx(hwnd1, NULL, DCX_USESTYLE | DCX_MYFLAG);//1
	hdc2 = GetDCEx(hwnd2, NULL, DCX_USESTYLE | DCX_MYFLAG);//2
    if (WindowFromDC(hdc1) == hwnd1)
    {
        printf("1-DC1 patri k oknu hwnd1. %p\n", hwnd1);
    }
    else if (WindowFromDC(hdc1) == hwnd2)
    {
        printf("1-DC1 patri k oknu hwnd2. %p\n", hwnd2);
    }
    else
    {
        printf("1-DC1 nepatri ani k jednomu oknu.\n");
    }
    if (WindowFromDC(hdc2) == hwnd1)
    {
        printf("DC2 patri k oknu hwnd1 %p\n", hwnd1);
    }
    else if (WindowFromDC(hdc2) == hwnd2)
    {
        printf("DC2 patri k oknu hwnd2 %p\n", hwnd2);
    }
    else
    {
        printf("DC2 nepatri ani k jednomu oknu.\n");
    }
    ReleaseDC(hwnd2, hdc2);

    if (WindowFromDC(hdc1) == hwnd1)
    {
        printf("DC1 patri k oknu hwnd1 %p\n", hwnd1);
    }
    else if (WindowFromDC(hdc1) == hwnd2)
    {
        printf("DC1 patri k oknu hwnd2 %p\n", hwnd2);
    }
    else
    {
        printf("DC1 nepatri ani k jednomu oknu.\n");
    }
    if (WindowFromDC(hdc2) == hwnd1)
    {
        printf("DC2 patri k oknu hwnd1 %p\n", hwnd1);
    }
    else if (WindowFromDC(hdc2) == hwnd2)
    {
        printf("DC2 patri k oknu hwnd2 %p\n", hwnd2);
    }
    else
    {
        printf("DC2 nepatri ani k jednomu oknu.\n");
    }

    /*
    if (IsDCForWindow(hdc1, hwnd1))
    {
        printf("DC1 patri k oknu hwnd1.\n");
    }
    else if (IsDCForWindow(hdc1, hwnd2))
    {
        printf("DC1 patri k oknu hwnd2.\n");
    }
    else
    {
        printf("DC1 nepatri ani k jednomu oknu.\n");
    }
    if (IsDCForWindow(hdc2, hwnd1))
    {
        printf("DC2 patri k oknu hwnd1.\n");
    }
    else if (IsDCForWindow(hdc2, hwnd2))
    {
        printf("DC2 patri k oknu hwnd2.\n");
    }
    else
    {
        printf("DC2 nepatri ani k jednomu oknu.\n");
    }
    Sleep(3000);
    */

    printf("DC1: %lx DC2: %lx\n", (long)hdc1, (long)hdc2);
	SetClassLongPtrA(hwnd1, GCL_STYLE, CS_OWNDC);
    printf("DC1: %lx DC2: %lx\n", (long)hdc1, (long)hdc2);
	//ReleaseDC(hwnd1, hdc1);
	//SetClassLongPtrA(hwnd1, GCL_STYLE, CS_OWNDC);
    hdcClass = GetDCEx(hwnd1, NULL, DCX_USESTYLE | DCX_MYFLAG);
    ok(hdcClass == NULL, "GetDCEx must be NULL\n");

    ShowWindow(hwnd1, SW_SHOW);
    UpdateWindow(hwnd1);
    ShowWindow(hwnd2, SW_SHOW);
    UpdateWindow(hwnd2);

    if (WindowFromDC(hdc1)== hwnd1)
    {
        printf("DC1 patri k oknu hwnd1.\n");
    }
    else if (WindowFromDC(hdc1) == hwnd2)
    {
        printf("DC1 patri k oknu hwnd2.\n");
    }
    else
    {
        printf("DC1 nepatri ani k jednomu oknu.\n");
    }
    if (WindowFromDC(hdc2) == hwnd1)
    {
        printf("DC2 patri k oknu hwnd1.\n");
    }
    else if (WindowFromDC(hdc2) == hwnd2)
    {
        printf("DC2 patri k oknu hwnd2.\n");
    }
    else
    {
        printf("DC2 nepatri ani k jednomu oknu.\n");
    }
    Sleep(1000);
    

    ShowWindow(hwnd1, SW_SHOW);
    UpdateWindow(hwnd1);
    ShowWindow(hwnd2, SW_SHOW);
    UpdateWindow(hwnd2);

    if (WindowFromDC(hdc1) == hwnd1)
    {
        printf("DC1 patri k oknu hwnd1.\n");
    }
    else if (WindowFromDC(hdc1) == hwnd2)
    {
        printf("DC1 patri k oknu hwnd2.\n");
    }
    else
    {
        printf("DC1 nepatri ani k jednomu oknu.\n");
    }
    if (WindowFromDC(hdc2) == hwnd1)
    {
        printf("DC2 patri k oknu hwnd1.\n");
    }
    else if (WindowFromDC(hdc2) == hwnd2)
    {
        printf("DC2 patri k oknu hwnd2.\n");
    }
    else
    {
        printf("DC2 nepatri ani k jednomu oknu.\n");
    }
    Sleep(1000);

	DestroyWindow(hwnd1);
	DestroyWindow(hwnd2);
	UnregisterClass(pszClassName, GetModuleHandleA(0));

    //2/
    atomClass = RegisterClassHelper(pszClassName, CS_CLASSDC, WndProc);
    hwnd2 = CreateWindowHelper(pszClassName, "Test Window2");
    hwnd1 = CreateWindowHelper(pszClassName, "Test Window1");
    hdc2 = GetDCEx(hwnd2, NULL, DCX_USESTYLE | DCX_MYFLAG); // 1
    hdc1 = GetDCEx(hwnd1, NULL, DCX_USESTYLE | DCX_MYFLAG); // 2
    SetClassLongPtrA(hwnd1, GCL_STYLE, CS_OWNDC);
    // ReleaseDC(hwnd1, hdc1);
    // SetClassLongPtrA(hwnd1, GCL_STYLE, CS_OWNDC);
    hdc1 = GetDCEx(hwnd1, NULL, DCX_USESTYLE | DCX_MYFLAG);
    ok(hdc1 != NULL, "GetDCEx must be not NULL\n");

    DestroyWindow(hwnd1);
    DestroyWindow(hwnd2);
    UnregisterClass(pszClassName, GetModuleHandleA(0));
	
	//3/
	atomClass = RegisterClassHelper(pszClassName, CS_CLASSDC, WndProc);
	hwnd1 = CreateWindowHelper(pszClassName, "Test Window1");
	hwnd2 = CreateWindowHelper(pszClassName, "Test Window2");
	hdc1 = GetDCEx(hwnd1, NULL, DCX_USESTYLE | DCX_MYFLAG);//4
	SetClassLongPtrA(hwnd1, GCL_STYLE, CS_OWNDC);
	hdc1=GetDCEx(hwnd1, NULL, DCX_USESTYLE | DCX_MYFLAG);
	ok(hdc1 != NULL, "GetDCEx must be not NULL\n");
	
	DestroyWindow(hwnd1);
	DestroyWindow(hwnd2);
	UnregisterClass(pszClassName, GetModuleHandleA(0));	
	
	//4/
	atomClass = RegisterClassHelper(pszClassName, CS_CLASSDC, WndProc);
	hwnd1 = CreateWindowHelper(pszClassName, "Test Window1");
	hwnd2 = CreateWindowHelper(pszClassName, "Test Window2");	
	hdc1 = GetDCEx(hwnd1, NULL, DCX_USESTYLE | DCX_MYFLAG);//1
	hdc2 = GetDCEx(hwnd2, NULL, DCX_USESTYLE | DCX_MYFLAG);//2
	SetClassLongPtrA(hwnd1, GCL_STYLE, CS_OWNDC);
	//ReleaseDC(hwnd1, hdc1);
	//SetClassLongPtrA(hwnd1, GCL_STYLE, CS_OWNDC);
    DestroyWindow(hwnd2);
    UnregisterClass(pszClassName, GetModuleHandleA(0));
	hdc1=GetDCEx(hwnd1, NULL, DCX_USESTYLE | DCX_MYFLAG);
	ok(hdc1 == NULL, "GetDCEx must be NULL\n");
	
	DestroyWindow(hwnd1);
	//DestroyWindow(hwnd2);
	//UnregisterClass(pszClassName, GetModuleHandleA(0));
	///////////////my tests

    if (hdc2 == 0)
        hdc2 = 0;
    if (atomClass == 0)
        atomClass = 0;
}

static
void
Test_GetDCEx_CS_SwitchedStyle()
{
    static const PSTR pszClassName = "TestClass_CS_SwitchedStyle";
    ATOM atomClass;
    HWND hwnd1, hwnd2;
    HDC hdc1, hdc2, hdcClass;

    /* Create a class with CS_CLASSDC */
    atomClass = RegisterClassHelper(pszClassName, CS_CLASSDC, WndProc);
    ok(atomClass != 0, "Failed to register class\n");

    /* Create the 2 windows */
    hwnd1 = CreateWindowHelper(pszClassName, "Test Window1");
    ok(hwnd1 != NULL, "Failed to create hwnd1\n");
    hwnd2 = CreateWindowHelper(pszClassName, "Test Window2");
    ok(hwnd2 != NULL, "Failed to create hwnd2\n");

    /* Get the class DC from the Windows */
    hdc1 = GetDCEx(hwnd1, NULL, DCX_USESTYLE);
    hdc2 = GetDCEx(hwnd2, NULL, DCX_USESTYLE);
    hdcClass = hdc1;
    ok(hdc1 == hdc2, "Expected same DC\n");
    ok(ReleaseDC(hwnd2, hdc2) == TRUE, "ReleaseDC failed\n");

    /* Switch the class to CS_OWNDC */
    ok(SetClassLongPtrA(hwnd1, GCL_STYLE, CS_OWNDC) == CS_CLASSDC, "unexpected style\n");
    ok(GetClassLongPtrA(hwnd1, GCL_STYLE) == CS_OWNDC, "class style not set\n");

    /* Release the DC and try to get another one, this should fail now */
    ok(ReleaseDC(hwnd1, hdc1) == TRUE, "ReleaseDC failed\n");
    hdc1 = GetDCEx(hwnd1, NULL, DCX_USESTYLE);
    ok(hdc1 == NULL, "GetDCEx should fail\n");

    /* Destroy the 1st window, this should move it's own DC to the cache,
       but not the class DC, but they are the same, so... */
    DestroyWindow(hwnd1);

    /* Create another window, this time it should have it's own DC */
    hwnd1 = CreateWindowHelper(pszClassName, "Test Window1");
    ok(hwnd1 != NULL, "Failed to create hwnd1\n");
    hdc1 = GetDCEx(hwnd1, NULL, DCX_USESTYLE);
    ok(hdc1 != NULL, "GetDXEx failed\n");
    ok(hdc1 != hdc2, "Should get different DC\n");

    /* Switch the class back to CS_CLASSDC */
    ok(SetClassLongPtrA(hwnd2, GCL_STYLE, CS_CLASSDC) == CS_OWNDC, "unexpected style\n");
    ok(GetClassLongPtrA(hwnd2, GCL_STYLE) == CS_CLASSDC, "class style not set\n");

    /* Get the 2nd window's DC, this should still be the class DC */
    hdc2 = GetDCEx(hwnd2, NULL, DCX_USESTYLE);
    ok(hdc2 != hdc1, "Expected different DC\n");
    ok(hdc2 == hdcClass, "Expected class DC\n");

    DestroyWindow(hwnd1);
    DestroyWindow(hwnd2);
    ok(UnregisterClass(pszClassName, GetModuleHandleA(0)) == TRUE,
       "UnregisterClass failed\n");
}

/*
 * Unit tests for DCE support
 *
 * Copyright 2005 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "winbase.h"
#include "windef.h"
#include "wingdi.h"
#include "winuser.h"

#include "wine/test.h"

#ifndef DCX_USESTYLE
#define DCX_USESTYLE 0x00010000
#endif

static HWND hwnd_cache, hwnd_owndc, hwnd_classdc, hwnd_classdc2, hwnd_parent, hwnd_parentdc;

void
MyOutputDebugString(const char *format, ...)
{
    char strbuffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(strbuffer, sizeof(strbuffer), format, args);
    va_end(args);
    OutputDebugString(strbuffer);
}

/* test behavior of DC attributes with various GetDC/ReleaseDC combinations */
static void
test_dc_attributes(void)
{
    HDC hdc, old_hdc;
    //HDC hdcs[20];
    //INT i, rop, def_rop, caps;
    INT rop;
    //BOOL found_dc;

    HWND testx;
    //HWND testy;

    /* test cache DC */

    /*
    hdc = GetDC(hwnd_cache);
    def_rop = GetROP2(hdc);

    SetROP2(hdc, R2_WHITE);
    rop = GetROP2(hdc);
    ok(rop == R2_WHITE, "wrong ROP2 %d\n", rop);

    ok(WindowFromDC(hdc) == hwnd_cache, "wrong window\n");
    ReleaseDC(hwnd_cache, hdc);
    ok(WindowFromDC(hdc) == 0, "wrong window\n");//FAIL!!!!!!!!!!!!!!!!!!!
    hdc = GetDC(hwnd_cache);
    rop = GetROP2(hdc);
    ok(rop == def_rop, "wrong ROP2 %d after release\n", rop);
    SetROP2(hdc, R2_WHITE);
    ok(WindowFromDC(hdc) == hwnd_cache, "wrong window\n");
    ReleaseDC(hwnd_cache, hdc);
    old_hdc = hdc;

    found_dc = FALSE;
    for (i = 0; i < 20; i++)
    {
        hdc = hdcs[i] = GetDCEx(hwnd_cache, 0, DCX_USESTYLE | DCX_NORESETATTRS);
        if (!hdc)
            break;
        rop = GetROP2(hdc);
        ok(rop == def_rop, "wrong ROP2 %d after release %p/%p\n", rop, old_hdc, hdc);
        if (hdc == old_hdc)
        {
            found_dc = TRUE;
            SetROP2(hdc, R2_WHITE);
        }
    }
    if (!found_dc)
    {
        trace("hdc %p not found in cache using %p\n", old_hdc, hdcs[0]);
        old_hdc = hdcs[0];
        SetROP2(old_hdc, R2_WHITE);
    }
    while (i > 0)
        ReleaseDC(hwnd_cache, hdcs[--i]);

    for (i = 0; i < 20; i++)
    {
        hdc = hdcs[i] = GetDCEx(hwnd_cache, 0, DCX_USESTYLE | DCX_NORESETATTRS);
        if (!hdc)
            break;
        rop = GetROP2(hdc);
        if (hdc == old_hdc)
            ok(rop == R2_WHITE || broken(rop == def_rop), // win9x doesn't support DCX_NORESETATTRS
               "wrong ROP2 %d after release %p/%p\n", rop, old_hdc, hdc);
        else
            ok(rop == def_rop, "wrong ROP2 %d after release %p/%p\n", rop, old_hdc, hdc);
    }
    while (i > 0)
        ReleaseDC(hwnd_cache, hdcs[--i]);

    for (i = 0; i < 20; i++)
    {
        hdc = hdcs[i] = GetDCEx(hwnd_cache, 0, DCX_USESTYLE);
        if (!hdc)
            break;
        rop = GetROP2(hdc);
        if (hdc == old_hdc)
        {
            ok(rop == R2_WHITE || broken(rop == def_rop), "wrong ROP2 %d after release %p/%p\n", rop, old_hdc, hdc);
            SetROP2(old_hdc, def_rop);
        }
        else
            ok(rop == def_rop, "wrong ROP2 %d after release %p/%p\n", rop, old_hdc, hdc);
    }
    while (i > 0)
        ReleaseDC(hwnd_cache, hdcs[--i]);*/

    /* Released cache DCs are 'disabled' */
    /*
    rop = SetROP2(old_hdc, R2_BLACK);
    ok(rop == 0, "got %d\n", rop);
    rop = GetROP2(old_hdc);
    ok(rop == 0, "got %d\n", rop);
    caps = GetDeviceCaps(old_hdc, HORZRES);
    ok(caps == 0, "got %d\n", caps);
    caps = GetDeviceCaps(old_hdc, VERTRES);
    ok(caps == 0, "got %d\n", caps);
    caps = GetDeviceCaps(old_hdc, NUMCOLORS);
    ok(caps == 0, "got %d\n", caps);
    ok(WindowFromDC(old_hdc) == 0, "wrong window\n");//FAIL!!!!!!!!!!!!!!!!!!!

    hdc = GetDC(0);
    caps = GetDeviceCaps(hdc, HORZRES);
    ok(caps != 0, "got %d\n", caps);
    caps = GetDeviceCaps(hdc, VERTRES);
    ok(caps != 0, "got %d\n", caps);
    caps = GetDeviceCaps(hdc, NUMCOLORS);
    ok(caps != 0, "got %d\n", caps);
    ReleaseDC(0, hdc);
    caps = GetDeviceCaps(hdc, HORZRES);
    ok(caps == 0, "got %d\n", caps);
    caps = GetDeviceCaps(hdc, VERTRES);
    ok(caps == 0, "got %d\n", caps);
    caps = GetDeviceCaps(hdc, NUMCOLORS);
    ok(caps == 0, "got %d\n", caps);
    */

    /* test own DC */
    /*
    hdc = GetDC(hwnd_owndc);
    SetROP2(hdc, R2_WHITE);
    rop = GetROP2(hdc);
    ok(rop == R2_WHITE, "wrong ROP2 %d\n", rop);

    old_hdc = hdc;
    ok(WindowFromDC(hdc) == hwnd_owndc, "wrong window\n");
    ReleaseDC(hwnd_owndc, hdc);
    ok(WindowFromDC(hdc) == hwnd_owndc, "wrong window\n");
    hdc = GetDC(hwnd_owndc);
    ok(old_hdc == hdc, "didn't get same DC %p/%p\n", old_hdc, hdc);
    rop = GetROP2(hdc);
    ok(rop == R2_WHITE, "wrong ROP2 %d after release\n", rop);
    ok(WindowFromDC(hdc) == hwnd_owndc, "wrong window\n");
    ReleaseDC(hwnd_owndc, hdc);
    rop = GetROP2(hdc);
    ok(rop == R2_WHITE, "wrong ROP2 %d after second release\n", rop);
    */

    /* test class DC */
    /*
    hdc = GetDC(hwnd_classdc);
    SetROP2(hdc, R2_WHITE);
    rop = GetROP2(hdc);
    ok(rop == R2_WHITE, "wrong ROP2 %d\n", rop);

    old_hdc = hdc;
    ok(WindowFromDC(hdc) == hwnd_classdc, "wrong window\n");
    ReleaseDC(hwnd_classdc, hdc);
    ok(WindowFromDC(hdc) == hwnd_classdc, "wrong window\n");
    hdc = GetDC(hwnd_classdc);
    ok(old_hdc == hdc, "didn't get same DC %p/%p\n", old_hdc, hdc);
    rop = GetROP2(hdc);
    ok(rop == R2_WHITE, "wrong ROP2 %d after release\n", rop);
    ok(WindowFromDC(hdc) == hwnd_classdc, "wrong window\n");
    ReleaseDC(hwnd_classdc, hdc);
    rop = GetROP2(hdc);
    ok(rop == R2_WHITE, "wrong ROP2 %d after second release\n", rop);
    */
    /* test class DC with 2 windows */
    MyOutputDebugString("GetDC-1\n");
    old_hdc = GetDC(hwnd_classdc);

    printf("GetDC-1 %x\n", (unsigned int)old_hdc);

    MyOutputDebugString("GetDC-1 %x\n", old_hdc);
    SetROP2(old_hdc, R2_BLACK);
    testx = WindowFromDC(old_hdc);
    MyOutputDebugString("testx-1 %x %x %d\n", testx, hwnd_classdc, (testx == hwnd_classdc));
    printf("testx-1 %x %x\n", (unsigned int)testx, (unsigned int)hwnd_classdc);
    ok(testx == hwnd_classdc, "wrong window\n");
    MyOutputDebugString("GetDC-2\n");

    //testy = WindowFromDC(old_hdc);// myTest
    //printf("testy %x %x %x\n", (unsigned int)testy, (unsigned int)hwnd_classdc, (unsigned int)hwnd_classdc2); // myTest

    hdc = GetDC(hwnd_classdc2);//ZDE JE PROBLÉM, tím by se mělo změnit poslední hwnd

    //testy = WindowFromDC(old_hdc);//myTest
    //printf("testy %x %x %x\n", (unsigned int)testy, (unsigned int)hwnd_classdc, (unsigned int)hwnd_classdc2);//myTest

    printf("GetDC-2 %x\n", (unsigned int)hdc);

    MyOutputDebugString("GetDC-2 %x\n", hdc);
    ok(old_hdc == hdc, "didn't get same DC %p/%p\n", old_hdc, hdc);
    rop = GetROP2(hdc);
    ok(rop == R2_BLACK, "wrong ROP2 %d for other window\n", rop);
    testx = WindowFromDC(hdc);
    MyOutputDebugString("testx-2 %x %x %d\n", testx, hwnd_classdc2, (testx == hwnd_classdc2));
    printf("testx-2 %x %x\n", (unsigned int)testx, (unsigned int)hwnd_classdc2);
    ok(testx == hwnd_classdc2, "wrong window\n"); // FAIL!!!!!!!!!!!!!!!!!!!
    ReleaseDC(hwnd_classdc, old_hdc);
    ReleaseDC(hwnd_classdc, hdc);
    ok(WindowFromDC(hdc) == hwnd_classdc2, "wrong window\n");//FAIL!!!!!!!!!!!!!!!!!!!
    rop = GetROP2(hdc);
    ok(rop == R2_BLACK, "wrong ROP2 %d after release\n", rop);
}

/* test behavior with various invalid parameters */
static void
test_parameters(void)
{
    HDC hdc;

    hdc = GetDC(hwnd_cache);
    ok(ReleaseDC(hwnd_owndc, hdc), "ReleaseDC with wrong window should succeed\n");

    hdc = GetDC(hwnd_cache);
    ok(!ReleaseDC(hwnd_cache, 0), "ReleaseDC with wrong HDC should fail\n");
    ok(ReleaseDC(hwnd_cache, hdc), "correct ReleaseDC should succeed\n");
    ok(!ReleaseDC(hwnd_cache, hdc), "second ReleaseDC should fail\n");

    hdc = GetDC(hwnd_owndc);
    ok(ReleaseDC(hwnd_cache, hdc), "ReleaseDC with wrong window should succeed\n");
    hdc = GetDC(hwnd_owndc);
    ok(ReleaseDC(hwnd_owndc, hdc), "correct ReleaseDC should succeed\n");
    ok(ReleaseDC(hwnd_owndc, hdc), "second ReleaseDC should succeed\n");

    hdc = GetDC(hwnd_classdc);
    ok(ReleaseDC(hwnd_cache, hdc), "ReleaseDC with wrong window should succeed\n");
    hdc = GetDC(hwnd_classdc);
    ok(ReleaseDC(hwnd_classdc, hdc), "correct ReleaseDC should succeed\n");
    ok(ReleaseDC(hwnd_classdc, hdc), "second ReleaseDC should succeed\n");
}

static void
test_dc_visrgn(void)
{
    HDC old_hdc, hdc;
    HRGN hrgn, hrgn2;
    RECT rect, parent_rect;

    /* cache DC */

    SetRect(&rect, 10, 10, 20, 20);
    MapWindowPoints(hwnd_cache, 0, (POINT *)&rect, 2);
    hrgn = CreateRectRgnIndirect(&rect);
    hdc = GetDCEx(hwnd_cache, hrgn, DCX_INTERSECTRGN | DCX_USESTYLE);
    SetRectEmpty(&rect);
    GetClipBox(hdc, &rect);
    ok(rect.left >= 10 && rect.top >= 10 && rect.right <= 20 && rect.bottom <= 20, "invalid clip box %s\n",
       wine_dbgstr_rect(&rect));
    ok(GetRgnBox(hrgn, &rect) != ERROR, "region must still be valid\n");
    ReleaseDC(hwnd_cache, hdc);
    ok(GetRgnBox(hrgn, &rect) == ERROR, "region must no longer be valid\n");

    /* cache DC with NORESETATTRS */

    SetRect(&rect, 10, 10, 20, 20);
    MapWindowPoints(hwnd_cache, 0, (POINT *)&rect, 2);
    hrgn = CreateRectRgnIndirect(&rect);
    hdc = GetDCEx(hwnd_cache, hrgn, DCX_INTERSECTRGN | DCX_USESTYLE | DCX_NORESETATTRS);
    SetRectEmpty(&rect);
    GetClipBox(hdc, &rect);
    ok(rect.left >= 10 && rect.top >= 10 && rect.right <= 20 && rect.bottom <= 20, "invalid clip box %s\n",
       wine_dbgstr_rect(&rect));
    ok(GetRgnBox(hrgn, &rect) != ERROR, "region must still be valid\n");
    ReleaseDC(hwnd_cache, hdc);
    ok(GetRgnBox(hrgn, &rect) == ERROR, "region must no longer be valid\n");
    hdc = GetDCEx(hwnd_cache, 0, DCX_USESTYLE | DCX_NORESETATTRS);
    SetRectEmpty(&rect);
    GetClipBox(hdc, &rect);
    ok(!(rect.left >= 10 && rect.top >= 10 && rect.right <= 20 && rect.bottom <= 20),
       "clip box should have been reset %s\n", wine_dbgstr_rect(&rect));
    ReleaseDC(hwnd_cache, hdc);

    /* window DC */

    SetRect(&rect, 10, 10, 20, 20);
    MapWindowPoints(hwnd_owndc, 0, (POINT *)&rect, 2);
    hrgn = CreateRectRgnIndirect(&rect);
    hdc = GetDCEx(hwnd_owndc, hrgn, DCX_INTERSECTRGN | DCX_USESTYLE);
    SetRectEmpty(&rect);
    GetClipBox(hdc, &rect);
    ok(rect.left >= 10 && rect.top >= 10 && rect.right <= 20 && rect.bottom <= 20, "invalid clip box %s\n",
       wine_dbgstr_rect(&rect));
    ok(GetRgnBox(hrgn, &rect) != ERROR, "region must still be valid\n");
    ReleaseDC(hwnd_owndc, hdc);
    ok(GetRgnBox(hrgn, &rect) != ERROR, "region must still be valid\n");
    SetRectEmpty(&rect);
    GetClipBox(hdc, &rect);
    ok(rect.left >= 10 && rect.top >= 10 && rect.right <= 20 && rect.bottom <= 20, "invalid clip box %s\n",
       wine_dbgstr_rect(&rect));
    hdc = GetDCEx(hwnd_owndc, 0, DCX_USESTYLE);
    SetRectEmpty(&rect);
    GetClipBox(hdc, &rect);
    ok(rect.left >= 10 && rect.top >= 10 && rect.right <= 20 && rect.bottom <= 20, "invalid clip box %s\n",
       wine_dbgstr_rect(&rect));
    ok(GetRgnBox(hrgn, &rect) != ERROR, "region must still be valid\n");
    ReleaseDC(hwnd_owndc, hdc);
    ok(GetRgnBox(hrgn, &rect) != ERROR, "region must still be valid\n");

    SetRect(&rect, 20, 20, 30, 30);
    MapWindowPoints(hwnd_owndc, 0, (POINT *)&rect, 2);
    hrgn2 = CreateRectRgnIndirect(&rect);
    hdc = GetDCEx(hwnd_owndc, hrgn2, DCX_INTERSECTRGN | DCX_USESTYLE);
    ok(GetRgnBox(hrgn, &rect) == ERROR, "region must no longer be valid\n");
    SetRectEmpty(&rect);
    GetClipBox(hdc, &rect);
    ok(rect.left >= 20 && rect.top >= 20 && rect.right <= 30 && rect.bottom <= 30, "invalid clip box %s\n",
       wine_dbgstr_rect(&rect));
    ok(GetRgnBox(hrgn2, &rect) != ERROR, "region2 must still be valid\n");
    ReleaseDC(hwnd_owndc, hdc);
    ok(GetRgnBox(hrgn2, &rect) != ERROR, "region2 must still be valid\n");
    hdc = GetDCEx(hwnd_owndc, 0, DCX_EXCLUDERGN | DCX_USESTYLE);
    ok(GetRgnBox(hrgn2, &rect) == ERROR, "region must no longer be valid\n");
    SetRectEmpty(&rect);
    GetClipBox(hdc, &rect);
    ok(!(rect.left >= 20 && rect.top >= 20 && rect.right <= 30 && rect.bottom <= 30),
       "clip box should have been reset %s\n", wine_dbgstr_rect(&rect));
    ReleaseDC(hwnd_owndc, hdc);

    /* class DC */

    SetRect(&rect, 10, 10, 20, 20);
    MapWindowPoints(hwnd_classdc, 0, (POINT *)&rect, 2);
    hrgn = CreateRectRgnIndirect(&rect);
    hdc = GetDCEx(hwnd_classdc, hrgn, DCX_INTERSECTRGN | DCX_USESTYLE);
    SetRectEmpty(&rect);
    GetClipBox(hdc, &rect);
    ok(rect.left >= 10 && rect.top >= 10 && rect.right <= 20 && rect.bottom <= 20, "invalid clip box %s\n",
       wine_dbgstr_rect(&rect));
    ok(GetRgnBox(hrgn, &rect) != ERROR, "region must still be valid\n");
    ReleaseDC(hwnd_classdc, hdc);
    ok(GetRgnBox(hrgn, &rect) != ERROR, "region must still be valid\n");
    SetRectEmpty(&rect);
    GetClipBox(hdc, &rect);
    ok(rect.left >= 10 && rect.top >= 10 && rect.right <= 20 && rect.bottom <= 20, "invalid clip box %s\n",
       wine_dbgstr_rect(&rect));

    hdc = GetDCEx(hwnd_classdc, 0, DCX_USESTYLE);
    SetRectEmpty(&rect);
    GetClipBox(hdc, &rect);
    ok(rect.left >= 10 && rect.top >= 10 && rect.right <= 20 && rect.bottom <= 20, "invalid clip box %s\n",
       wine_dbgstr_rect(&rect));
    ok(GetRgnBox(hrgn, &rect) != ERROR, "region must still be valid\n");
    ReleaseDC(hwnd_classdc, hdc);
    ok(GetRgnBox(hrgn, &rect) != ERROR, "region must still be valid\n");

    SetRect(&rect, 20, 20, 30, 30);
    MapWindowPoints(hwnd_classdc, 0, (POINT *)&rect, 2);
    hrgn2 = CreateRectRgnIndirect(&rect);
    hdc = GetDCEx(hwnd_classdc, hrgn2, DCX_INTERSECTRGN | DCX_USESTYLE);
    ok(GetRgnBox(hrgn, &rect) == ERROR, "region must no longer be valid\n");
    SetRectEmpty(&rect);
    GetClipBox(hdc, &rect);
    ok(rect.left >= 20 && rect.top >= 20 && rect.right <= 30 && rect.bottom <= 30, "invalid clip box %s\n",
       wine_dbgstr_rect(&rect));
    ok(GetRgnBox(hrgn2, &rect) != ERROR, "region2 must still be valid\n");

    old_hdc = hdc;
    hdc = GetDCEx(hwnd_classdc2, 0, DCX_USESTYLE);
    ok(old_hdc == hdc, "did not get the same hdc %p/%p\n", old_hdc, hdc);
    ok(GetRgnBox(hrgn2, &rect) != ERROR, "region2 must still be valid\n");
    SetRectEmpty(&rect);
    GetClipBox(hdc, &rect);

    ok(!(rect.left >= 20 && rect.top >= 20 && rect.right <= 30 && rect.bottom <= 30),
       "clip box should have been reset %s\n", wine_dbgstr_rect(&rect));
    ReleaseDC(hwnd_classdc2, hdc);
    ok(GetRgnBox(hrgn2, &rect) != ERROR, "region2 must still be valid\n");
    hdc = GetDCEx(hwnd_classdc2, 0, DCX_EXCLUDERGN | DCX_USESTYLE);
    ok(GetRgnBox(hrgn2, &rect) != ERROR, "region2 must still be valid\n");
    ok(!(rect.left >= 20 && rect.top >= 20 && rect.right <= 30 && rect.bottom <= 30),
       "clip box must have been reset %s\n", wine_dbgstr_rect(&rect));
    ReleaseDC(hwnd_classdc2, hdc);

    /* parent DC */
    hdc = GetDC(hwnd_parentdc);
    GetClipBox(hdc, &rect);
    ReleaseDC(hwnd_parentdc, hdc);

    hdc = GetDC(hwnd_parent);
    GetClipBox(hdc, &parent_rect);
    ReleaseDC(hwnd_parent, hdc);

    ok(EqualRect(&rect, &parent_rect), "rect = %s, expected %s\n", wine_dbgstr_rect(&rect),
       wine_dbgstr_rect(&parent_rect));
}

/* test various BeginPaint/EndPaint behaviors */
static void
test_begin_paint(void)
{
    HDC old_hdc, hdc;
    RECT rect, parent_rect;
    PAINTSTRUCT ps;
    COLORREF cr;

    /* cache DC */

    /* clear update region */
    RedrawWindow(hwnd_cache, NULL, 0, RDW_VALIDATE | RDW_NOFRAME | RDW_NOERASE);
    SetRect(&rect, 10, 10, 20, 20);
    RedrawWindow(hwnd_cache, &rect, 0, RDW_INVALIDATE);
    hdc = BeginPaint(hwnd_cache, &ps);
    SetRectEmpty(&rect);
    GetClipBox(hdc, &rect);
    ok(rect.left >= 10 && rect.top >= 10 && rect.right <= 20 && rect.bottom <= 20, "invalid clip box %s\n",
       wine_dbgstr_rect(&rect));
    EndPaint(hwnd_cache, &ps);

    /* window DC */

    RedrawWindow(hwnd_owndc, NULL, 0, RDW_VALIDATE | RDW_NOFRAME | RDW_NOERASE);
    SetRect(&rect, 10, 10, 20, 20);
    RedrawWindow(hwnd_owndc, &rect, 0, RDW_INVALIDATE);
    hdc = BeginPaint(hwnd_owndc, &ps);
    SetRectEmpty(&rect);
    GetClipBox(hdc, &rect);
    ok(rect.left >= 10 && rect.top >= 10 && rect.right <= 20 && rect.bottom <= 20, "invalid clip box %s\n",
       wine_dbgstr_rect(&rect));
    ReleaseDC(hwnd_owndc, hdc);
    SetRectEmpty(&rect);
    GetClipBox(hdc, &rect);
    ok(rect.left >= 10 && rect.top >= 10 && rect.right <= 20 && rect.bottom <= 20, "invalid clip box %s\n",
       wine_dbgstr_rect(&rect));
    ok(GetDC(hwnd_owndc) == hdc, "got different hdc\n");
    SetRectEmpty(&rect);
    GetClipBox(hdc, &rect);
    ok(rect.left >= 10 && rect.top >= 10 && rect.right <= 20 && rect.bottom <= 20, "invalid clip box %s\n",
       wine_dbgstr_rect(&rect));
    EndPaint(hwnd_owndc, &ps);
    SetRectEmpty(&rect);
    GetClipBox(hdc, &rect);
    ok(!(rect.left >= 10 && rect.top >= 10 && rect.right <= 20 && rect.bottom <= 20),
       "clip box should have been reset %s\n", wine_dbgstr_rect(&rect));
    RedrawWindow(hwnd_owndc, NULL, 0, RDW_VALIDATE | RDW_NOFRAME | RDW_NOERASE);
    SetRect(&rect, 10, 10, 20, 20);
    RedrawWindow(hwnd_owndc, &rect, 0, RDW_INVALIDATE | RDW_ERASE);
    ok(GetDC(hwnd_owndc) == hdc, "got different hdc\n");
    SetRectEmpty(&rect);
    GetClipBox(hdc, &rect);
    ok(!(rect.left >= 10 && rect.top >= 10 && rect.right <= 20 && rect.bottom <= 20),
       "clip box should be the whole window %s\n", wine_dbgstr_rect(&rect));
    RedrawWindow(hwnd_owndc, NULL, 0, RDW_ERASENOW);
    SetRectEmpty(&rect);
    GetClipBox(hdc, &rect);
    ok(!(rect.left >= 10 && rect.top >= 10 && rect.right <= 20 && rect.bottom <= 20),
       "clip box should still be the whole window %s\n", wine_dbgstr_rect(&rect));

    /* class DC */

    RedrawWindow(hwnd_classdc, NULL, 0, RDW_VALIDATE | RDW_NOFRAME | RDW_NOERASE);
    SetRect(&rect, 10, 10, 20, 20);
    RedrawWindow(hwnd_classdc, &rect, 0, RDW_INVALIDATE);
    hdc = BeginPaint(hwnd_classdc, &ps);
    SetRectEmpty(&rect);
    GetClipBox(hdc, &rect);

    printf("rect: %ld %ld %ld %ld\n", rect.left, rect.top, rect.right, rect.bottom);

    ok(rect.left >= 10 && rect.top >= 10 && rect.right <= 20 && rect.bottom <= 20, "invalid clip box %s\n",
       wine_dbgstr_rect(&rect));

    old_hdc = hdc;

    //GetDCEx(NULL, (HANDLE)0x1234, 1);
    GetDCEx(NULL, (HANDLE)0x1234, 3);
    GetDCEx(NULL, (HANDLE)0x1234, 5);
    hdc = GetDC(hwnd_classdc2);
    GetDCEx(NULL, (HANDLE)0x1234, 4);
    //GetDCEx(NULL, (HANDLE)0x1234, 0);
    //GetDCEx(NULL, (HANDLE)0x1234, 2);

    ok(old_hdc == hdc, "did not get the same hdc %p/%p\n", old_hdc, hdc);
    SetRectEmpty(&rect);
    GetClipBox(hdc, &rect);

    printf("rect: %ld %ld %ld %ld\n", rect.left, rect.top, rect.right, rect.bottom);

    ok(!(rect.left >= 10 && rect.top >= 10 && rect.right <= 20 && rect.bottom <= 20),//problem
       "clip box should have been reset %s\n", wine_dbgstr_rect(&rect));
    ReleaseDC(hwnd_classdc2, hdc);
    EndPaint(hwnd_classdc, &ps);

    /* parent DC */
    RedrawWindow(hwnd_parent, NULL, 0, RDW_VALIDATE | RDW_NOFRAME | RDW_NOERASE);
    RedrawWindow(hwnd_parentdc, NULL, 0, RDW_INVALIDATE);
    hdc = BeginPaint(hwnd_parentdc, &ps);
    GetClipBox(hdc, &rect);
    cr = SetPixel(hdc, 10, 10, RGB(255, 0, 0));
    ok(cr != -1, "error drawing outside of window client area\n");
    EndPaint(hwnd_parentdc, &ps);
    GetClientRect(hwnd_parent, &parent_rect);

    ok(rect.left == parent_rect.left, "rect.left = %ld, expected %ld\n", rect.left, parent_rect.left);
    ok(rect.top == parent_rect.top, "rect.top = %ld, expected %ld\n", rect.top, parent_rect.top);
    todo_wine ok(rect.right == parent_rect.right, "rect.right = %ld, expected %ld\n", rect.right, parent_rect.right);//FAIL!!!!!!!!!!!!!!!!!!!
    todo_wine ok(rect.bottom == parent_rect.bottom, "rect.bottom = %ld, expected %ld\n", rect.bottom, parent_rect.bottom);//FAIL!!!!!!!!!!!!!!!!!!!

    hdc = GetDC(hwnd_parent);
    todo_wine ok(GetPixel(hdc, 10, 10) == cr, "error drawing outside of window client area\n");//FAIL!!!!!!!!!!!!!!!!!!!
    ReleaseDC(hwnd_parent, hdc);
}

/* test ScrollWindow with window DCs */
static void
test_scroll_window(void)
{
    PAINTSTRUCT ps;
    HDC hdc;
    RECT clip, rect;

    /* ScrollWindow uses the window DC, ScrollWindowEx doesn't */

    UpdateWindow(hwnd_owndc);
    SetRect(&clip, 25, 25, 50, 50);
    ScrollWindow(hwnd_owndc, -5, -10, NULL, &clip);
    hdc = BeginPaint(hwnd_owndc, &ps);
    SetRectEmpty(&rect);
    GetClipBox(hdc, &rect);
    ok(rect.left >= 25 && rect.top >= 25 && rect.right <= 50 && rect.bottom <= 50, "invalid clip box %s\n",
       wine_dbgstr_rect(&rect));
    EndPaint(hwnd_owndc, &ps);

    SetViewportExtEx(hdc, 2, 3, NULL);
    SetViewportOrgEx(hdc, 30, 20, NULL);

    ScrollWindow(hwnd_owndc, -5, -10, NULL, &clip);
    hdc = BeginPaint(hwnd_owndc, &ps);
    SetRectEmpty(&rect);
    GetClipBox(hdc, &rect);
    ok(rect.left >= 25 && rect.top >= 25 && rect.right <= 50 && rect.bottom <= 50, "invalid clip box %s\n",
       wine_dbgstr_rect(&rect));//FAIL!!!!!!!!!!!!!!!!!!!
    EndPaint(hwnd_owndc, &ps);

    ScrollWindowEx(hwnd_owndc, -5, -10, NULL, &clip, 0, NULL, SW_INVALIDATE | SW_ERASE);
    hdc = BeginPaint(hwnd_owndc, &ps);
    SetRectEmpty(&rect);
    GetClipBox(hdc, &rect);
    ok(rect.left >= -5 && rect.top >= 5 && rect.right <= 20 && rect.bottom <= 30, "invalid clip box %s\n",
       wine_dbgstr_rect(&rect));
    EndPaint(hwnd_owndc, &ps);

    SetViewportExtEx(hdc, 1, 1, NULL);
    SetViewportOrgEx(hdc, 0, 0, NULL);

    ScrollWindowEx(hwnd_owndc, -5, -10, NULL, &clip, 0, NULL, SW_INVALIDATE | SW_ERASE);
    hdc = BeginPaint(hwnd_owndc, &ps);
    SetRectEmpty(&rect);
    GetClipBox(hdc, &rect);
    ok(rect.left >= 25 && rect.top >= 25 && rect.right <= 50 && rect.bottom <= 50, "invalid clip box %s\n",
       wine_dbgstr_rect(&rect));
    EndPaint(hwnd_owndc, &ps);
}

static void
test_invisible_create(void)
{
    HWND hwnd_owndc =
        CreateWindowA("owndc_class", NULL, WS_OVERLAPPED, 0, 200, 100, 100, 0, 0, GetModuleHandleA(0), NULL);
    HDC dc1, dc2;

    dc1 = GetDC(hwnd_owndc);
    dc2 = GetDC(hwnd_owndc);

    ok(dc1 == dc2, "expected owndc dcs to match\n");

    ReleaseDC(hwnd_owndc, dc2);
    ReleaseDC(hwnd_owndc, dc1);
    DestroyWindow(hwnd_owndc);
}

static void
test_dc_layout(void)
{
    DWORD(WINAPI * pSetLayout)(HDC hdc, DWORD layout);
    DWORD(WINAPI * pGetLayout)(HDC hdc);
    HWND hwnd_cache_rtl, hwnd_owndc_rtl, hwnd_classdc_rtl, hwnd_classdc2_rtl;
    HDC hdc;
    DWORD layout;
    HMODULE mod = GetModuleHandleA("gdi32.dll");

    pGetLayout = (void *)GetProcAddress(mod, "GetLayout");
    pSetLayout = (void *)GetProcAddress(mod, "SetLayout");
    if (!pGetLayout || !pSetLayout)
    {
        win_skip("Don't have SetLayout\n");
        return;
    }

    hdc = GetDC(hwnd_cache);
    pSetLayout(hdc, LAYOUT_RTL);
    layout = pGetLayout(hdc);
    ReleaseDC(hwnd_cache, hdc);
    if (!layout)
    {
        win_skip("SetLayout not supported\n");
        return;
    }

    hwnd_cache_rtl = CreateWindowExA(
        WS_EX_LAYOUTRTL, "cache_class", NULL, WS_OVERLAPPED | WS_VISIBLE, 0, 0, 100, 100, 0, 0, GetModuleHandleA(0),
        NULL);
    hwnd_owndc_rtl = CreateWindowExA(
        WS_EX_LAYOUTRTL, "owndc_class", NULL, WS_OVERLAPPED | WS_VISIBLE, 0, 200, 100, 100, 0, 0, GetModuleHandleA(0),
        NULL);
    hwnd_classdc_rtl = CreateWindowExA(
        WS_EX_LAYOUTRTL, "classdc_class", NULL, WS_OVERLAPPED | WS_VISIBLE, 200, 0, 100, 100, 0, 0, GetModuleHandleA(0),
        NULL);
    hwnd_classdc2_rtl = CreateWindowExA(
        WS_EX_LAYOUTRTL, "classdc_class", NULL, WS_OVERLAPPED | WS_VISIBLE, 200, 200, 100, 100, 0, 0,
        GetModuleHandleA(0), NULL);
    hdc = GetDC(hwnd_cache_rtl);
    layout = pGetLayout(hdc);

    ok(layout == LAYOUT_RTL, "wrong layout %lx\n", layout);
    pSetLayout(hdc, 0);
    ReleaseDC(hwnd_cache_rtl, hdc);
    hdc = GetDC(hwnd_owndc_rtl);
    layout = pGetLayout(hdc);
    ok(layout == LAYOUT_RTL, "wrong layout %lx\n", layout);
    ReleaseDC(hwnd_cache_rtl, hdc);

    hdc = GetDC(hwnd_cache);
    layout = pGetLayout(hdc);
    ok(layout == 0, "wrong layout %lx\n", layout);
    ReleaseDC(hwnd_cache, hdc);

    hdc = GetDC(hwnd_owndc_rtl);
    layout = pGetLayout(hdc);
    ok(layout == LAYOUT_RTL, "wrong layout %lx\n", layout);
    pSetLayout(hdc, 0);
    ReleaseDC(hwnd_owndc_rtl, hdc);
    hdc = GetDC(hwnd_owndc_rtl);
    layout = pGetLayout(hdc);
    ok(layout == LAYOUT_RTL, "wrong layout %lx\n", layout);
    ReleaseDC(hwnd_owndc_rtl, hdc);

    hdc = GetDC(hwnd_classdc_rtl);
    layout = pGetLayout(hdc);
    ok(layout == LAYOUT_RTL, "wrong layout %lx\n", layout);
    pSetLayout(hdc, 0);
    ReleaseDC(hwnd_classdc_rtl, hdc);
    hdc = GetDC(hwnd_classdc2_rtl);
    layout = pGetLayout(hdc);
    ok(layout == LAYOUT_RTL, "wrong layout %lx\n", layout);
    ReleaseDC(hwnd_classdc2_rtl, hdc);
    hdc = GetDC(hwnd_classdc);
    layout = pGetLayout(hdc);
    ok(layout == LAYOUT_RTL, "wrong layout %lx\n", layout);
    ReleaseDC(hwnd_classdc_rtl, hdc);

    DestroyWindow(hwnd_classdc2_rtl);
    DestroyWindow(hwnd_classdc_rtl);
    DestroyWindow(hwnd_owndc_rtl);
    DestroyWindow(hwnd_cache_rtl);
}

static void
test_destroyed_window(void)
{
    HDC dc, old_dc;
    HDC hdcs[30];
    int i, rop;

    dc = GetDC(hwnd_cache);
    SetROP2(dc, R2_WHITE);
    rop = GetROP2(dc);
    ok(rop == R2_WHITE, "wrong ROP2 %d\n", rop);
    ok(WindowFromDC(dc) == hwnd_cache, "wrong window\n");
    old_dc = dc;

    DestroyWindow(hwnd_cache);
    rop = GetROP2(dc);
    ok(rop == 0, "wrong ROP2 %d\n", rop);
    ok(WindowFromDC(dc) == 0, "wrong window\n");
    ok(!ReleaseDC(hwnd_cache, dc), "ReleaseDC succeeded\n");
    dc = GetDC(hwnd_cache);
    ok(!dc, "Got a non-NULL DC (%p) for a destroyed window\n", dc);

    for (i = 0; i < 30; i++)
    {
        dc = hdcs[i] = GetDCEx(hwnd_parent, 0, DCX_CACHE | DCX_USESTYLE);
        if (dc == old_dc)
            break;
    }
    ok(i < 30, "DC for destroyed window not reused\n");
    while (i > 0)
        ReleaseDC(hwnd_parent, hdcs[--i]);

    dc = GetDC(hwnd_classdc);
    SetROP2(dc, R2_WHITE);
    rop = GetROP2(dc);
    ok(rop == R2_WHITE, "wrong ROP2 %d\n", rop);
    ok(WindowFromDC(dc) == hwnd_classdc, "wrong window\n");
    old_dc = dc;

    dc = GetDC(hwnd_classdc2);
    ok(old_dc == dc, "wrong DC\n");
    rop = GetROP2(dc);
    ok(rop == R2_WHITE, "wrong ROP2 %d\n", rop);
    ok(WindowFromDC(dc) == hwnd_classdc2, "wrong window\n");//FAIL!!!!!!!!!!!!!!!!!!!
    DestroyWindow(hwnd_classdc2);

    rop = GetROP2(dc);
    ok(rop == R2_WHITE, "wrong ROP2 %d\n", rop);
    ok(WindowFromDC(dc) == 0, "wrong window\n");//FAIL!!!!!!!!!!!!!!!!!!!
    ok(!ReleaseDC(hwnd_classdc2, dc), "ReleaseDC succeeded\n");//FAIL!!!!!!!!!!!!!!!!!!!
    dc = GetDC(hwnd_classdc2);
    ok(!dc, "Got a non-NULL DC (%p) for a destroyed window\n", dc);

    dc = GetDC(hwnd_classdc);
    ok(dc != 0, "Got NULL DC\n");
    rop = GetROP2(dc);
    ok(rop == R2_WHITE, "wrong ROP2 %d\n", rop);
    ok(WindowFromDC(dc) == hwnd_classdc, "wrong window\n");
    DestroyWindow(hwnd_classdc);

    rop = GetROP2(dc);
    ok(rop == R2_WHITE, "wrong ROP2 %d\n", rop);//FAIL!!!!!!!!!!!!!!!!!!!
    ok(WindowFromDC(dc) == 0, "wrong window\n");
    ok(!ReleaseDC(hwnd_classdc, dc), "ReleaseDC succeeded\n");
    dc = GetDC(hwnd_classdc);
    ok(!dc, "Got a non-NULL DC (%p) for a destroyed window\n", dc);

    dc = GetDC(hwnd_owndc);
    ok(dc != 0, "Got NULL DC\n");
    rop = GetROP2(dc);
    ok(rop == R2_WHITE, "wrong ROP2 %d\n", rop);
    ok(WindowFromDC(dc) == hwnd_owndc, "wrong window\n");
    DestroyWindow(hwnd_owndc);

    rop = GetROP2(dc);
    ok(rop == 0, "wrong ROP2 %d\n", rop);
    ok(WindowFromDC(dc) == 0, "wrong window\n");
    ok(!ReleaseDC(hwnd_owndc, dc), "ReleaseDC succeeded\n");
    dc = GetDC(hwnd_owndc);
    ok(!dc, "Got a non-NULL DC (%p) for a destroyed window\n", dc);

    DestroyWindow(hwnd_parent);
}

void START_TEST2(/*dce*/)
{
    WNDCLASSA cls;

    cls.style = CS_DBLCLKS;
    cls.lpfnWndProc = DefWindowProcA;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = 0;
    cls.hInstance = GetModuleHandleA(0);
    cls.hIcon = 0;
    cls.hCursor = LoadCursorA(0, (LPCSTR)IDC_ARROW);
    cls.hbrBackground = GetStockObject(WHITE_BRUSH);
    cls.lpszMenuName = NULL;
    cls.lpszClassName = "cache_class";
    RegisterClassA(&cls);
    cls.style = CS_DBLCLKS | CS_OWNDC;
    cls.lpszClassName = "owndc_class";
    RegisterClassA(&cls);
    cls.style = CS_DBLCLKS | CS_CLASSDC;
    cls.lpszClassName = "classdc_class";
    RegisterClassA(&cls);
    cls.style = CS_PARENTDC;
    cls.lpszClassName = "parentdc_class";
    RegisterClassA(&cls);

    MyOutputDebugString("CreateWindowA-1\n");
    hwnd_cache =
        CreateWindowA("cache_class", NULL, WS_OVERLAPPED | WS_VISIBLE, 0, 0, 100, 100, 0, 0, GetModuleHandleA(0), NULL);
    MyOutputDebugString("CreateWindowA-1 %x\n", hwnd_cache);
    MyOutputDebugString("CreateWindowA-2\n");
    hwnd_owndc =
        CreateWindowA("owndc_class", NULL, WS_OVERLAPPED | WS_VISIBLE, 0, 200, 100, 100, 0, 0, GetModuleHandleA(0), NULL);
    MyOutputDebugString("CreateWindowA-2 %x\n", hwnd_owndc);
    MyOutputDebugString("CreateWindowA-3\n");
    hwnd_classdc =
        CreateWindowA("classdc_class", NULL, WS_OVERLAPPED | WS_VISIBLE, 200, 0, 100, 100, 0, 0, GetModuleHandleA(0), NULL);
    MyOutputDebugString("CreateWindowA-3 %x\n", hwnd_classdc);
    MyOutputDebugString("CreateWindowA-4\n");
    hwnd_classdc2 =
        CreateWindowA("classdc_class", NULL, WS_OVERLAPPED | WS_VISIBLE, 200, 200, 100, 100, 0, 0, GetModuleHandleA(0), NULL);
    MyOutputDebugString("CreateWindowA-4 %x\n", hwnd_classdc2);
    MyOutputDebugString("CreateWindowA-5\n");
    hwnd_parent =
        CreateWindowA("static", NULL, WS_OVERLAPPED | WS_VISIBLE, 400, 0, 100, 100, 0, 0, 0, NULL);
    MyOutputDebugString("CreateWindowA-5 %x\n", hwnd_parent);
    MyOutputDebugString("CreateWindowA-6\n");
    hwnd_parentdc =
        CreateWindowA("parentdc_class", NULL, WS_CHILD | WS_VISIBLE, 0, 0, 1, 1, hwnd_parent, 0, 0, NULL);
    MyOutputDebugString("CreateWindowA-6 %x\n", hwnd_parentdc);

    if(0)test_dc_attributes();    
    if (0)
        test_parameters();
    test_dc_visrgn();    

    test_begin_paint();
    test_scroll_window();
    test_invisible_create();
    test_dc_layout();
    /* this should be last */
    test_destroyed_window();

    //GetDCEx(NULL, (HANDLE)0x1234, 0);
}

START_TEST(GetDCEx)
{
	if(0)
	{
    Test_GetDCEx_Params();
    Test_GetDCEx_Cached();
    Test_GetDCEx_CS_OWNDC();
    Test_GetDCEx_CS_CLASSDC();
    Test_GetDCEx_CS_Mixed();
    Test_GetDCEx_CS_SwitchedStyle();
	}
    if (0)
        Test_myTests();
    if (1)
        START_TEST2();
}
