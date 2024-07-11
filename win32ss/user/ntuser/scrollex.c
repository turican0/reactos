/*
 *  COPYRIGHT:        See COPYING in the top level directory
 *  PROJECT:          ReactOS Win32k subsystem
 *  PURPOSE:          Window scrolling function
 *  FILE:             win32ss/user/ntuser/scrollex.c
 *  PROGRAMER:        Filip Navara (xnavara@volny.cz)
 */

#include <win32k.h>

#include <process.h>

/* NtUserScrollWindowEx flag */
#define SW_NODCCACHE  0x8000

DBG_DEFAULT_CHANNEL(UserPainting);

static
HWND FASTCALL
co_IntFixCaret(PWND Window, RECTL *lprc, UINT flags)
{
   PTHRDCARETINFO CaretInfo;
   PTHREADINFO pti;
   PUSER_MESSAGE_QUEUE ThreadQueue;
   HWND hWndCaret;
   PWND WndCaret;

   ASSERT_REFS_CO(Window);

   pti = PsGetCurrentThreadWin32Thread();
   ThreadQueue = pti->MessageQueue;
   CaretInfo = &ThreadQueue->CaretInfo;
   hWndCaret = CaretInfo->hWnd;

   WndCaret = ValidateHwndNoErr(hWndCaret);

   // FIXME: Check for WndCaret can be NULL
   if (WndCaret == Window ||
         ((flags & SW_SCROLLCHILDREN) && IntIsChildWindow(Window, WndCaret)))
   {
      POINT pt, FromOffset, ToOffset;
      RECTL rcCaret;

      pt.x = CaretInfo->Pos.x;
      pt.y = CaretInfo->Pos.y;
      IntGetClientOrigin(WndCaret, &FromOffset);
      IntGetClientOrigin(Window, &ToOffset);
      rcCaret.left = pt.x;
      rcCaret.top = pt.y;
      rcCaret.right = pt.x + CaretInfo->Size.cx;
      rcCaret.bottom = pt.y + CaretInfo->Size.cy;
      if (RECTL_bIntersectRect(lprc, lprc, &rcCaret))
      {
         co_UserHideCaret(0);
         lprc->left = pt.x;
         lprc->top = pt.y;
         return hWndCaret;
      }
   }

   return 0;
}

/*
    Old GetUpdateRgn, for scrolls, see above note.
 */
INT FASTCALL
co_IntGetUpdateRgn(PWND Window, PREGION Rgn, BOOL bErase)
{
    int RegionType;
    RECTL Rect;
    PREGION UpdateRgn;

    ASSERT_REFS_CO(Window);

    if (bErase)
    {
       co_IntPaintWindows(Window, RDW_NOCHILDREN, FALSE);
    }

    Window->state &= ~WNDS_UPDATEDIRTY;

    if (Window->hrgnUpdate == NULL)
    {
        REGION_SetRectRgn(Rgn, 0, 0, 0, 0);
        return NULLREGION;
    }

    UpdateRgn = REGION_LockRgn(Window->hrgnUpdate);
    if (!UpdateRgn)
       return ERROR;

    Rect = Window->rcClient;
    IntIntersectWithParents(Window, &Rect);
    REGION_SetRectRgn(Rgn, Rect.left, Rect.top, Rect.right, Rect.bottom);
    RegionType = IntGdiCombineRgn(Rgn, Rgn, UpdateRgn, RGN_AND);
    REGION_bOffsetRgn(Rgn, -Window->rcClient.left, -Window->rcClient.top);
    REGION_UnlockRgn(UpdateRgn);

    return RegionType;
}

static
INT FASTCALL
UserScrollDC(
   HDC hDC,
   INT dx,
   INT dy,
   const RECTL *prcScroll,
   const RECTL *prcClip,
   HRGN hrgnUpdate,
   PREGION RgnUpdate,
   RECTL *prcUpdate)
{
   PDC pDC;
   RECTL rcScroll, rcClip, rcSrc, rcDst;
   INT Result;

   if (GdiGetClipBox(hDC, &rcClip) == ERROR)
   {
       ERR("GdiGetClipBox failed for HDC %p\n", hDC);
       return ERROR;
   }

   rcScroll = rcClip;
   if (prcClip)
   {
      RECTL_bIntersectRect(&rcClip, &rcClip, prcClip);
   }

   if (prcScroll)
   {
      rcScroll = *prcScroll;
      RECTL_bIntersectRect(&rcSrc, &rcClip, prcScroll);
   }
   else
   {
      rcSrc = rcClip;
   }

   rcDst = rcSrc;
   RECTL_vOffsetRect(&rcDst, dx, dy);
   RECTL_bIntersectRect(&rcDst, &rcDst, &rcClip);

   if (!NtGdiBitBlt( hDC,
                     rcDst.left,
                     rcDst.top,
                     rcDst.right - rcDst.left,
                     rcDst.bottom - rcDst.top,
                     hDC,
                     rcDst.left - dx,
                     rcDst.top - dy,
                     SRCCOPY,
                     0,
                     0))
   {
      return ERROR;
   }

   /* Calculate the region that was invalidated by moving or
      could not be copied, because it was not visible */
   if (RgnUpdate || hrgnUpdate || prcUpdate)
   {
      PREGION RgnOwn, RgnTmp;

      pDC = DC_LockDc(hDC);
      if (!pDC)
      {
         return ERROR;
      }

       if (hrgnUpdate)
       {
           NT_ASSERT(RgnUpdate == NULL);
           RgnUpdate = REGION_LockRgn(hrgnUpdate);
           if (!RgnUpdate)
           {
               DC_UnlockDc(pDC);
               return ERROR;
           }
       }

      /* Begin with the shifted and then clipped scroll rect */
      rcDst = rcScroll;
      RECTL_vOffsetRect(&rcDst, dx, dy);
      RECTL_bIntersectRect(&rcDst, &rcDst, &rcClip);
      if (RgnUpdate)
      {
         RgnOwn = RgnUpdate;
         REGION_SetRectRgn(RgnOwn, rcDst.left, rcDst.top, rcDst.right, rcDst.bottom);
      }
      else
      {
         RgnOwn = IntSysCreateRectpRgnIndirect(&rcDst);
      }

      /* Add the source rect */
      RgnTmp = IntSysCreateRectpRgnIndirect(&rcSrc);
      IntGdiCombineRgn(RgnOwn, RgnOwn, RgnTmp, RGN_OR);

      /* Substract the part of the dest that was visible in source */
      IntGdiCombineRgn(RgnTmp, RgnTmp, pDC->prgnVis, RGN_AND);
      REGION_bOffsetRgn(RgnTmp, dx, dy);
      Result = IntGdiCombineRgn(RgnOwn, RgnOwn, RgnTmp, RGN_DIFF);

      /* DO NOT Unlock DC while messing with prgnVis! */
      DC_UnlockDc(pDC);

      REGION_Delete(RgnTmp);

      if (prcUpdate)
      {
         REGION_GetRgnBox(RgnOwn, prcUpdate);
      }

      if (hrgnUpdate)
      {
         REGION_UnlockRgn(RgnUpdate);
      }
      else if (!RgnUpdate)
      {
         REGION_Delete(RgnOwn);
      }
   }
   else
      Result = NULLREGION;

   return Result;
}

/*************************************************************************
 *           NtUserScrollWindowEx (win32u.@)
 *
 * Note: contrary to what the doc says, pixels that are scrolled from the
 *      outside of clipRect to the inside are NOT painted.
 */
INT WINAPI WineNtUserScrollWindowEx( HWND hwnd, INT dx, INT dy, const RECT *rect,
                                 const RECT *clip_rect, HRGN update_rgn,
                                 RECT *update_rect, UINT flags )
{
    BOOL update = update_rect || update_rgn || flags & (SW_INVALIDATE | SW_ERASE);
    BOOL own_rgn = TRUE, move_caret = FALSE;
    HRGN temp_rgn, winupd_rgn = 0;
    INT retval = NULLREGION;
    HWND caret_hwnd = NULL;
    POINT new_caret_pos;
    RECT rc, cliprc;
    int rdw_flags;
    HDC hdc;
	PWND Window = NULL;

    /*TRACE( "%p, %d,%d update_rgn=%p update_rect = %p %s %04x\n",
           hwnd, dx, dy, update_rgn, update_rect, wine_dbgstr_rect(rect), flags );
    TRACE( "clip_rect = %s\n", wine_dbgstr_rect(clip_rect) );*/
    if (flags & ~(SW_SCROLLCHILDREN | SW_INVALIDATE | SW_ERASE | SW_NODCCACHE))
        FIXME( "some flags (%04x) are unhandled\n", flags );

    rdw_flags = (flags & SW_ERASE) && (flags & SW_INVALIDATE) ?
        RDW_INVALIDATE | RDW_ERASE  : RDW_INVALIDATE;
		
	Window = UserGetWindowObject(hwnd);

    if (!IntIsWindowDrawable( Window )) return ERROR;
    //hwnd = get_full_window_handle( hwnd );

    IntGetClientRect( Window, &rc );
    if (clip_rect) RECTL_bIntersectRect( &cliprc, &rc, clip_rect );
    else cliprc = rc;

    if (rect) RECTL_bIntersectRect( &rc, &rc, rect );
    if (update_rgn) own_rgn = FALSE;
    else if (update) update_rgn = NtGdiCreateRectRgn( 0, 0, 0, 0 );

    new_caret_pos.x = new_caret_pos.y = 0;

    if (/*!IsRectEmpty( &cliprc ) &&*/ (dx || dy))
    {
        //DWORD style = GetWindowLong( hwnd, GWL_STYLE );
        DWORD dcxflags = 0;

        caret_hwnd = co_IntFixCaret( Window, &rc, /*dx, dy,*/ flags/*, &move_caret, &new_caret_pos */);
        if (caret_hwnd) NtUserHideCaret( caret_hwnd );

        if (!(flags & SW_NODCCACHE)) dcxflags |= DCX_CACHE;
        if (Window->style & WS_CLIPSIBLINGS) dcxflags |= DCX_CLIPSIBLINGS;
        //if (GetClassLong( hwnd, GCL_STYLE/*, FALSE*/ ) & CS_PARENTDC) dcxflags |= DCX_PARENTCLIP;
        if (!(flags & SW_SCROLLCHILDREN) && (Window->style & WS_CLIPCHILDREN))
            dcxflags |= DCX_CLIPCHILDREN;
        hdc = NtUserGetDCEx( hwnd, 0, dcxflags);
        if (hdc)
        {
            NtUserScrollDC( hdc, dx, dy, &rc, &cliprc, update_rgn, update_rect );
            UserReleaseDC( Window, hdc, FALSE );
            if (!update) NtUserRedrawWindow( hwnd, NULL, update_rgn, rdw_flags );
        }

        /* If the windows has an update region, this must be scrolled as well.
         * Keep a copy in winupd_rgn to be added to hrngUpdate at the end. */
        temp_rgn = NtGdiCreateRectRgn( 0, 0, 0, 0 );
        retval = NtUserGetUpdateRgn( hwnd, temp_rgn, FALSE );
        if (retval != NULLREGION)
        {
            HRGN clip_rgn = NtGdiCreateRectRgn( cliprc.left, cliprc.top,
                                                cliprc.right, cliprc.bottom );
            if (!own_rgn)
            {
                winupd_rgn = NtGdiCreateRectRgn( 0, 0, 0, 0);
                NtGdiCombineRgn( winupd_rgn, temp_rgn, 0, RGN_COPY);
            }
            NtGdiOffsetRgn( temp_rgn, dx, dy );
            NtGdiCombineRgn( temp_rgn, temp_rgn, clip_rgn, RGN_AND );
            if (!own_rgn) NtGdiCombineRgn( winupd_rgn, winupd_rgn, temp_rgn, RGN_OR );
            NtUserRedrawWindow( hwnd, NULL, temp_rgn, rdw_flags );

           /*
            * Catch the case where the scrolling amount exceeds the size of the
            * original window. This generated a second update area that is the
            * location where the original scrolled content would end up.
            * This second region is not returned by the ScrollDC and sets
            * ScrollWindowEx apart from just a ScrollDC.
            *
            * This has been verified with testing on windows.
            */
            if (abs( dx ) > abs( rc.right - rc.left ) || abs( dy ) > abs( rc.bottom - rc.top ))
            {
                NtGdiSetRectRgn( temp_rgn, rc.left + dx, rc.top + dy, rc.right+dx, rc.bottom + dy );
                NtGdiCombineRgn( temp_rgn, temp_rgn, clip_rgn, RGN_AND );
                NtGdiCombineRgn( update_rgn, update_rgn, temp_rgn, RGN_OR );

                if (update_rect)
                {
                    RECT temp_rect;
                    NtGdiGetRgnBox( temp_rgn, &temp_rect );
                    RECTL_bUnionRect( update_rect, update_rect, &temp_rect );
                }

                if (!own_rgn) NtGdiCombineRgn( winupd_rgn, winupd_rgn, temp_rgn, RGN_OR );
            }
            NtGdiDeleteObjectApp( clip_rgn );
        }
        NtGdiDeleteObjectApp( temp_rgn );
    }
    else
    {
        /* nothing was scrolled */
        if (!own_rgn) NtGdiSetRectRgn( update_rgn, 0, 0, 0, 0 );
        //SetRectEmpty( update_rect );
    }

    if (flags & SW_SCROLLCHILDREN)
    {
        HWND *list = IntWinListChildren( Window );
        if (list)
        {
            RECT r, dummy;
            int i;

            for (i = 0; list[i]; i++)
            {
                //GetWindowRects( list[i], COORDS_PARENT, &r, NULL, get_thread_dpi() );
                //GetWindowRect(list[i], &r);
				if (!rect || RECTL_bIntersectRect( &dummy, &r, rect ))
                    NtUserSetWindowPos( list[i], 0, r.left + dx, r.top  + dy, 0, 0,
                                        SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE |
                                        SWP_NOREDRAW | SWP_DEFERERASE );
            }
            ExFreePoolWithTag(list, USERTAG_WINDOWLIST);
        }
    }

    if (flags & (SW_INVALIDATE | SW_ERASE))
        NtUserRedrawWindow( hwnd, NULL, update_rgn, rdw_flags |
                            ((flags & SW_SCROLLCHILDREN) ? RDW_ALLCHILDREN : 0 ) );

    if (winupd_rgn)
    {
        NtGdiCombineRgn( update_rgn, update_rgn, winupd_rgn, RGN_OR );
        NtGdiDeleteObjectApp( winupd_rgn );
    }

    if (move_caret) SetCaretPos( new_caret_pos.x, new_caret_pos.y );
    if (caret_hwnd) NtUserShowCaret( caret_hwnd );
    if (own_rgn && update_rgn) NtGdiDeleteObjectApp( update_rgn );

    return retval;
}

DWORD
FASTCALL
IntScrollWindowEx(
   PWND Window,
   INT dx,
   INT dy,
   const RECT *prcScroll,
   const RECT *prcClip,
   HRGN hrgnUpdate,
   LPRECT prcUpdate,
   UINT flags)
{
   INT Result;
   RECTL rcScroll, rcClip, rcCaret;
   PWND CaretWnd;
   HDC hDC;
   PREGION RgnUpdate = NULL, RgnTemp, RgnWinupd = NULL;
   HWND hwndCaret;
   DWORD dcxflags = 0;
   int rdw_flags;
   USER_REFERENCE_ENTRY CaretRef;

   if (!Window || !IntIsWindowDrawable(Window))
   {
      return ERROR;
   }

   IntGetClientRect(Window, &rcClip);

   if (prcScroll)
      RECTL_bIntersectRect(&rcScroll, &rcClip, prcScroll);
   else
      rcScroll = rcClip;

   if (prcClip)
      RECTL_bIntersectRect(&rcClip, &rcClip, prcClip);

   if (rcClip.right <= rcClip.left || rcClip.bottom <= rcClip.top ||
         (dx == 0 && dy == 0))
   {
      return NULLREGION;
   }

   /* We must use a copy of the region, as we can't hold an exclusive lock
    * on it while doing callouts to user-mode */
   RgnUpdate = IntSysCreateRectpRgn(0, 0, 0, 0);
   if(!RgnUpdate)
   {
       EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
       return ERROR;
   }

   if (hrgnUpdate)
   {
       RgnTemp = REGION_LockRgn(hrgnUpdate);
       if (!RgnTemp)
       {
           EngSetLastError(ERROR_INVALID_HANDLE);
           Result = ERROR;
           goto Cleanup;
       }
       IntGdiCombineRgn(RgnUpdate, RgnTemp, NULL, RGN_COPY);
       REGION_UnlockRgn(RgnTemp);
   }

   /* ScrollWindow uses the window DC, ScrollWindowEx doesn't */
   if (flags & SW_SCROLLWNDDCE)
   {
      dcxflags = DCX_USESTYLE;

      if (!(Window->pcls->style & (CS_OWNDC|CS_CLASSDC)))
         dcxflags |= DCX_CACHE; // AH??? wine~ If not Powned or with Class go Cheap!

      if (flags & SW_SCROLLCHILDREN && Window->style & WS_CLIPCHILDREN)
         dcxflags |= DCX_CACHE|DCX_NOCLIPCHILDREN;
   }
   else
   {
       /* So in this case ScrollWindowEx uses Cache DC. */
       dcxflags = DCX_CACHE|DCX_USESTYLE;
       if (flags & SW_SCROLLCHILDREN) dcxflags |= DCX_NOCLIPCHILDREN;
   }

   hDC = UserGetDCEx(Window, 0, dcxflags);
   if (!hDC)
   {
      /* FIXME: SetLastError? */
      Result = ERROR;
      goto Cleanup;
   }

   rdw_flags = (flags & SW_ERASE) && (flags & SW_INVALIDATE) ? RDW_INVALIDATE | RDW_ERASE : RDW_INVALIDATE ;

   rcCaret = rcScroll;
   hwndCaret = co_IntFixCaret(Window, &rcCaret, flags);

   Result = UserScrollDC( hDC,
                          dx,
                          dy,
                          &rcScroll,
                          &rcClip,
                          NULL,
                          RgnUpdate,
                          prcUpdate);

   UserReleaseDC(Window, hDC, FALSE);

   /*
    * Take into account the fact that some damage may have occurred during
    * the scroll. Keep a copy in hrgnWinupd to be added to hrngUpdate at the end.
    */

   RgnTemp = IntSysCreateRectpRgn(0, 0, 0, 0);
   if (!RgnTemp)
   {
       EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
       Result = ERROR;
       goto Cleanup;
   }

   if (co_IntGetUpdateRgn(Window, RgnTemp, FALSE) != NULLREGION)
   {
      PREGION RgnClip = IntSysCreateRectpRgnIndirect(&rcClip);
      if (RgnClip)
      {
          if (hrgnUpdate)
          {
             RgnWinupd = IntSysCreateRectpRgn(0, 0, 0, 0);
             // FIXME: What to do if RgnWinupd == NULL??
             IntGdiCombineRgn( RgnWinupd, RgnTemp, 0, RGN_COPY);
          }

          REGION_bOffsetRgn(RgnTemp, dx, dy);

          IntGdiCombineRgn(RgnTemp, RgnTemp, RgnClip, RGN_AND);

          if (hrgnUpdate)
              IntGdiCombineRgn( RgnWinupd, RgnWinupd, RgnTemp, RGN_OR );

          co_UserRedrawWindow(Window, NULL, RgnTemp, rdw_flags );

          REGION_Delete(RgnClip);
      }
   }
   REGION_Delete(RgnTemp);

   if (flags & SW_SCROLLCHILDREN)
   {
      PWND Child;
      RECTL rcChild;
      POINT ClientOrigin;
      USER_REFERENCE_ENTRY WndRef;
      RECTL rcDummy;
      LPARAM lParam;

      IntGetClientOrigin(Window, &ClientOrigin);

      for (Child = Window->spwndChild; Child; Child = Child->spwndNext)
      {
         rcChild = Child->rcWindow;
         RECTL_vOffsetRect(&rcChild, -ClientOrigin.x, -ClientOrigin.y);

         /* Adjust window positions */
         RECTL_vOffsetRect(&Child->rcWindow, dx, dy);
         RECTL_vOffsetRect(&Child->rcClient, dx, dy);

         if (!prcScroll || RECTL_bIntersectRect(&rcDummy, &rcChild, &rcScroll))
         {
            UserRefObjectCo(Child, &WndRef);

            if (UserIsDesktopWindow(Window->spwndParent))
               lParam = MAKELONG(Child->rcClient.left, Child->rcClient.top);
            else
               lParam = MAKELONG(rcChild.left + dx, rcChild.top + dy);

            /* wine sends WM_POSCHANGING, WM_POSCHANGED messages */
            /* windows sometimes a WM_MOVE */
            co_IntSendMessage(UserHMGetHandle(Child), WM_MOVE, 0, lParam);

            UserDerefObjectCo(Child);
         }
      }
   }

   if (flags & (SW_INVALIDATE | SW_ERASE))
   {
      PREGION RgnClip = IntSysCreateRectpRgnIndirect(&rcClip);
      if (RgnClip)
      {
          co_UserRedrawWindow( Window,
                           NULL,
                           RgnClip,
                           rdw_flags |                                    /*    HACK    */
                          ((flags & SW_SCROLLCHILDREN) ? RDW_ALLCHILDREN : RDW_NOCHILDREN) );
          REGION_Delete(RgnClip);
      }
   }

   if (hwndCaret && (CaretWnd = UserGetWindowObject(hwndCaret)))
   {
      UserRefObjectCo(CaretWnd, &CaretRef);

      co_IntSetCaretPos(rcCaret.left + dx, rcCaret.top + dy);
      co_UserShowCaret(CaretWnd);

      UserDerefObjectCo(CaretWnd);
   }

   if (hrgnUpdate && (Result != ERROR))
   {
       /* Give everything back to the caller */
       RgnTemp = REGION_LockRgn(hrgnUpdate);
       /* The handle should still be valid */
       ASSERT(RgnTemp);
       if (RgnWinupd)
           IntGdiCombineRgn(RgnTemp, RgnUpdate, RgnWinupd, RGN_OR);
       else
           IntGdiCombineRgn(RgnTemp, RgnUpdate, NULL, RGN_COPY);
       REGION_UnlockRgn(RgnTemp);
   }

Cleanup:
   if (RgnWinupd)
   {
       REGION_Delete(RgnWinupd);
   }

   if (RgnUpdate)
   {
      REGION_Delete(RgnUpdate);
   }

   return Result;
}


BOOL FASTCALL
IntScrollWindow(PWND pWnd,
                int dx,
                int dy,
                CONST RECT *lpRect,
                CONST RECT *prcClip)
{
   return IntScrollWindowEx( pWnd, dx, dy, lpRect, prcClip, 0, NULL,
                            (lpRect ? 0 : SW_SCROLLCHILDREN) | (SW_ERASE|SW_INVALIDATE|SW_SCROLLWNDDCE)) != ERROR;
}

/*
 * NtUserScrollDC
 *
 * Status
 *    @implemented
 */
BOOL APIENTRY
NtUserScrollDC(
   HDC hDC,
   INT dx,
   INT dy,
   const RECT *prcUnsafeScroll,
   const RECT *prcUnsafeClip,
   HRGN hrgnUpdate,
   LPRECT prcUnsafeUpdate)
{
   BOOL Ret = FALSE;
   DWORD Result;
   NTSTATUS Status = STATUS_SUCCESS;
   RECTL rcScroll, rcClip, rcUpdate;

   TRACE("Enter NtUserScrollDC\n");
   UserEnterExclusive();

   _SEH2_TRY
   {
      if (prcUnsafeScroll)
      {
         ProbeForRead(prcUnsafeScroll, sizeof(*prcUnsafeScroll), 1);
         rcScroll = *prcUnsafeScroll;
      }
      if (prcUnsafeClip)
      {
         ProbeForRead(prcUnsafeClip, sizeof(*prcUnsafeClip), 1);
         rcClip = *prcUnsafeClip;
      }
      if (prcUnsafeUpdate)
      {
         ProbeForWrite(prcUnsafeUpdate, sizeof(*prcUnsafeUpdate), 1);
      }
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
      Status = _SEH2_GetExceptionCode();
   }
   _SEH2_END;

   if (!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      goto Exit; // Return FALSE
   }

   Result = UserScrollDC( hDC,
                          dx,
                          dy,
                          prcUnsafeScroll ? &rcScroll : NULL,
                          prcUnsafeClip   ? &rcClip   : NULL,
                          hrgnUpdate,
                          NULL,
                          prcUnsafeUpdate ? &rcUpdate : NULL);
   if(Result == ERROR)
   {
      /* FIXME: Only if hRgnUpdate is invalid we should SetLastError(ERROR_INVALID_HANDLE) */
      goto Exit; // Return FALSE
   }

   if (prcUnsafeUpdate)
   {
      _SEH2_TRY
      {
         *prcUnsafeUpdate = rcUpdate;
      }
      _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
      {
         Status = _SEH2_GetExceptionCode();
      }
      _SEH2_END;

      if (!NT_SUCCESS(Status))
      {
         /* FIXME: SetLastError? */
         /* FIXME: correct? We have already scrolled! */
         goto Exit; // Return FALSE
      }
   }

   Ret = TRUE;

Exit:
   TRACE("Leave NtUserScrollDC, ret=%i\n", Ret);
   UserLeave();
   return Ret;
}

/*
 * NtUserScrollWindowEx
 *
 * Status
 *    @implemented
 */

DWORD APIENTRY
NtUserScrollWindowEx(
   HWND hWnd,
   INT dx,
   INT dy,
   const RECT *prcUnsafeScroll,
   const RECT *prcUnsafeClip,
   HRGN hrgnUpdate,
   LPRECT prcUnsafeUpdate,
   UINT flags)
{
	return WineNtUserScrollWindowEx(hWnd, dx, dy, prcUnsafeScroll, prcUnsafeClip, hrgnUpdate, prcUnsafeUpdate, flags);
   DWORD Result = ERROR;
   NTSTATUS Status = STATUS_SUCCESS;
   PWND Window = NULL;
   RECTL rcScroll, rcClip, rcUpdate;
   USER_REFERENCE_ENTRY Ref;

   TRACE("Enter NtUserScrollWindowEx\n");
   UserEnterExclusive();

   Window = UserGetWindowObject(hWnd);
   if (!Window || !IntIsWindowDrawable(Window))
   {
      Window = NULL; /* prevent deref at cleanup */
      goto Cleanup; // Return ERROR
   }
   UserRefObjectCo(Window, &Ref);

   _SEH2_TRY
   {
      if (prcUnsafeScroll)
      {
         ProbeForRead(prcUnsafeScroll, sizeof(*prcUnsafeScroll), 1);
         rcScroll = *prcUnsafeScroll;
      }

      if (prcUnsafeClip)
      {
         ProbeForRead(prcUnsafeClip, sizeof(*prcUnsafeClip), 1);
         rcClip = *prcUnsafeClip;
      }
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
      Status = _SEH2_GetExceptionCode();
   }
   _SEH2_END;

   if (!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      goto Cleanup; // Return ERROR
   }

   Result = IntScrollWindowEx(Window,
                              dx, dy,
                              prcUnsafeScroll ? &rcScroll : NULL,
                              prcUnsafeClip   ? &rcClip   : NULL,
                              hrgnUpdate,
                              prcUnsafeUpdate ? &rcUpdate : NULL,
                              flags);

   if (prcUnsafeUpdate)
   {
      _SEH2_TRY
      {
         /* Probe here, to not fail on invalid pointer before scrolling */
         ProbeForWrite(prcUnsafeUpdate, sizeof(*prcUnsafeUpdate), 1);
         *prcUnsafeUpdate = rcUpdate;
      }
      _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
      {
         Status = _SEH2_GetExceptionCode();
      }
      _SEH2_END;

      if (!NT_SUCCESS(Status))
      {
         SetLastNtError(Status);
         Result = ERROR;
      }
   }

Cleanup:
   if (Window)
      UserDerefObjectCo(Window);

   TRACE("Leave NtUserScrollWindowEx, ret=%lu\n", Result);
   UserLeave();
   return Result;
}

/* EOF */
