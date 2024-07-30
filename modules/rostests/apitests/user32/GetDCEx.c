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

    hdc1 = GetDCEx(hwnd, NULL, 0);
    ok(hdc1 != NULL, "GetDCEx failed\n");
    hdc2 = GetDCEx(hwnd, NULL, 0);
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

BOOL
IsDCForWindow(HDC hdc, HWND hwnd)
{
    // Nakreslíme unikátní barvu na pozadí okna a zkontrolujeme, zda je viditelná v DC
    COLORREF uniqueColor = RGB(123, 123, 123); // Volíme barvu, která se pravděpodobně nepoužívá v jiném okně
    HBRUSH brush = CreateSolidBrush(uniqueColor);
    HDC tempDC = GetDC(hwnd);

    RECT rect;
    GetClientRect(hwnd, &rect);
    FillRect(tempDC, &rect, brush);
    ReleaseDC(hwnd, tempDC);
    DeleteObject(brush);

    // Zkontrolujeme, zda DC obsahuje tuto unikátní barvu
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBitmap = CreateCompatibleBitmap(hdc, 1, 1);
    SelectObject(memDC, memBitmap);

    // Zkopírujeme jeden pixel z levého horního rohu
    BitBlt(memDC, 0, 0, 1, 1, hdc, 0, 0, SRCCOPY);

    COLORREF color = GetPixel(memDC, 0, 0);
    DeleteDC(memDC);
    DeleteObject(memBitmap);

    return color == uniqueColor;
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
    hwnd2 = CreateWindowHelper(pszClassName, "Test Window2");
    //SetWindowPos(hwnd2, NULL, 100, 100, 800, 600, SWP_NOZORDER | SWP_NOMOVE);
    hwnd1 = CreateWindowHelper(pszClassName, "Test Window1");
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
    printf("DC1: %lx DC2: %lx\n", (long)hdc1, (long)hdc2);
	SetClassLongPtrA(hwnd1, GCL_STYLE, CS_OWNDC);
    printf("DC1: %lx DC2: %lx\n", (long)hdc1, (long)hdc2);
	//ReleaseDC(hwnd1, hdc1);
	//SetClassLongPtrA(hwnd1, GCL_STYLE, CS_OWNDC);
    hdcClass = GetDCEx(hwnd1, NULL, DCX_USESTYLE | DCX_MYFLAG);
    ok(hdcClass == NULL, "GetDCEx must be NULL\n");

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
    Sleep(5000);

    ShowWindow(hwnd1, SW_SHOW);
    UpdateWindow(hwnd1);
    ShowWindow(hwnd2, SW_SHOW);
    UpdateWindow(hwnd2);

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
    Sleep(5000);
    if (IsDCForWindow(hdcClass, hwnd1))
    {
        printf("hdcClass patri k oknu hwnd1.\n");
    }
    else if (IsDCForWindow(hdcClass, hwnd2))
    {
        printf("hdcClass patri k oknu hwnd2.\n");
    }
    else
    {
        printf("hdcClass nepatri ani k jednomu oknu.\n");
    }
    Sleep(5000);
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
	Test_myTests();
}
