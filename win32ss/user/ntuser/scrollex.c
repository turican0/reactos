/*
 *  COPYRIGHT:        See COPYING in the top level directory
 *  PROJECT:          ReactOS Win32k subsystem
 *  PURPOSE:          Window scrolling function
 *  FILE:             win32ss/user/ntuser/scrollex.c
 *  PROGRAMER:        Filip Navara (xnavara@volny.cz)
 */

#include <win32k.h>

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
	ERR("UserScrollDC\n");
   PDC pDC;
   RECTL rcScroll, rcClip, rcSrc, rcDst;
   INT Result;

   if (GdiGetClipBox(hDC, &rcClip) == ERROR)
   {
       ERR("GdiGetClipBox failed for HDC %p\n", hDC);
       return ERROR;
   }
   
   ERR("rcClip: %d %d %d %d\n",rcClip.left,rcClip.right,rcClip.top,rcClip.bottom);   

   rcScroll = rcClip;
   if (prcClip)
   {
	   ERR("prcClip: %d %d %d %d\n",prcClip->left,prcClip->right,prcClip->top,prcClip->bottom);
      RECTL_bIntersectRect(&rcClip, &rcClip, prcClip);
	  ERR("prcClip: %d %d %d %d\n",prcClip->left,prcClip->right,prcClip->top,prcClip->bottom);
   }
   
   ERR("rcClip: %d %d %d %d\n",rcClip.left,rcClip.right,rcClip.top,rcClip.bottom);   

   if (prcScroll)
   {
	   ERR("prcScroll: %d %d %d %d\n",prcScroll->left,prcScroll->right,prcScroll->top,prcScroll->bottom);
      rcScroll = *prcScroll;
      RECTL_bIntersectRect(&rcSrc, &rcClip, prcScroll);
	  ERR("prcScroll: %d %d %d %d\n",prcScroll->left,prcScroll->right,prcScroll->top,prcScroll->bottom);
   }
   else
   {
      rcSrc = rcClip;
   }
   
   ERR("rcClip: %d %d %d %d\n",rcClip.left,rcClip.right,rcClip.top,rcClip.bottom);   

   rcDst = rcSrc;
   ERR("rcDst: %d %d %d %d\n",rcDst.left,rcDst.right,rcDst.top,rcDst.bottom);   
   RECTL_vOffsetRect(&rcDst, dx, dy);
   ERR("rcDst: %d %d %d %d\n",rcDst.left,rcDst.right,rcDst.top,rcDst.bottom);   
   RECTL_bIntersectRect(&rcDst, &rcDst, &rcClip);
   ERR("rcDst: %d %d %d %d\n",rcDst.left,rcDst.right,rcDst.top,rcDst.bottom);   

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
   
   ERR("(RgnUpdate || hrgnUpdate || prcUpdate) %d\n",(RgnUpdate || hrgnUpdate || prcUpdate));   

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

/*
void CopyRectangle(HDC hdc, int srcX, int srcY, int width, int height, int destX, int destY) {
    if (hdc != NULL) {
        BitBlt(hdc, destX, destY, width, height, hdc, srcX, srcY, SRCCOPY);
    }
}*/

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
   
   if(0)
   {
      UserRefObjectCo(CaretWnd, &CaretRef);
	  if(hwndCaret)
		  ERR("");
   }

	ERR("\n\nIntScrollWindowEx\n");
   if (!Window || !IntIsWindowDrawable(Window))
   {
      return ERROR;
   }

   IntGetClientRect(Window, &rcClip);
   ERR("rcClip: %d %d %d %d\n",rcClip.left,rcClip.right,rcClip.top,rcClip.bottom);
   
   ERR("prcScroll: %d\n",prcScroll);
   
   if (prcScroll)
      RECTL_bIntersectRect(&rcScroll, &rcClip, prcScroll);
   else
      rcScroll = rcClip;
  
  ERR("rcScroll: %d %d %d %d\n",rcScroll.left,rcScroll.right,rcScroll.top,rcScroll.bottom);

ERR("prcClip: %d\n",prcClip);

   if (prcClip)
      RECTL_bIntersectRect(&rcClip, &rcClip, prcClip);
  
  ERR("rcClip: %d %d %d %d\n",rcClip.left,rcClip.right,rcClip.top,rcClip.bottom);

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
	   ERR("RgnTemp1: %d %d %d %d\n",RgnTemp->rdh.rcBound.left,RgnTemp->rdh.rcBound.right,RgnTemp->rdh.rcBound.top,RgnTemp->rdh.rcBound.bottom);
   }
   
   ERR("RgnUpdate1: %d %d %d %d\n",RgnUpdate->rdh.rcBound.left,RgnUpdate->rdh.rcBound.right,RgnUpdate->rdh.rcBound.top,RgnUpdate->rdh.rcBound.bottom);
   
   ERR("flags: %d\n",flags);
   ERR("SW_SCROLLWNDDCE: %d\n",SW_SCROLLWNDDCE);

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
   
   ERR("dcxflags: %d\n",dcxflags);

   hDC = UserGetDCEx(Window, 0, dcxflags);
   if (!hDC)
   {
      /* FIXME: SetLastError? */
      Result = ERROR;
      goto Cleanup;
   }

   rdw_flags = (flags & SW_ERASE) && (flags & SW_INVALIDATE) ? RDW_INVALIDATE | RDW_ERASE : RDW_INVALIDATE ;
   
   ERR("rdw_flags: %d\n",rdw_flags);

   rcCaret = rcScroll;
   hwndCaret = co_IntFixCaret(Window, &rcCaret, flags);
   
   ERR("rcScroll4: %d %d %d %d\n",rcScroll.left,rcScroll.right,rcScroll.top,rcScroll.bottom);
   ERR("rcClip4: %d %d %d %d\n",rcClip.left,rcClip.right,rcClip.top,rcClip.bottom);
		//here 1
   ERR("dx dy: %d %d\n",dx,dy);
   
   if(prcUpdate!=NULL)
	ERR("prcUpdate: %d %d %d %d\n",prcUpdate->left,prcUpdate->right,prcUpdate->top,prcUpdate->bottom);
   
   //CopyRectangle(hDC, 0, 0, 300, 300, 0+dx, 0+dy);

   Result = UserScrollDC( hDC,
                          dx,
                          dy,
                          &rcScroll,
                          &rcClip,
                          NULL,
                          RgnUpdate,
                          prcUpdate);

   UserReleaseDC(Window, hDC, FALSE);
   
   
   ERR("RgnUpdate4: %d %d %d %d\n",RgnUpdate->rdh.rcBound.left,RgnUpdate->rdh.rcBound.right,RgnUpdate->rdh.rcBound.top,RgnUpdate->rdh.rcBound.bottom);
   
   ERR("dx dy: %d\n",dx,dy);

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
	   ERR("co_IntGetUpdateRgn\n");
      PREGION RgnClip = IntSysCreateRectpRgnIndirect(&rcClip);
      if (RgnClip)
      {
          if (hrgnUpdate)
          {
             RgnWinupd = IntSysCreateRectpRgn(0, 0, 0, 0);
			 ERR("RgnWinupd: %x\n",RgnWinupd);
             // FIXME: What to do if RgnWinupd == NULL??
			 if (!RgnWinupd)
			   {
				   EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
				   Result = ERROR;
				   goto Cleanup;
			   }
             IntGdiCombineRgn( RgnWinupd, RgnTemp, 0, RGN_COPY);
			 ERR("RgnTemp2: %d %d %d %d\n",RgnTemp->rdh.rcBound.left,RgnTemp->rdh.rcBound.right,RgnTemp->rdh.rcBound.top,RgnTemp->rdh.rcBound.bottom);
          }

          REGION_bOffsetRgn(RgnTemp, dx, dy);
			
			ERR("RgnTemp3: %d %d %d %d\n",RgnTemp->rdh.rcBound.left,RgnTemp->rdh.rcBound.right,RgnTemp->rdh.rcBound.top,RgnTemp->rdh.rcBound.bottom);
			
          IntGdiCombineRgn(RgnTemp, RgnTemp, RgnClip, RGN_AND);

		ERR("RgnTemp4: %d %d %d %d\n",RgnTemp->rdh.rcBound.left,RgnTemp->rdh.rcBound.right,RgnTemp->rdh.rcBound.top,RgnTemp->rdh.rcBound.bottom);

          if (hrgnUpdate)
              IntGdiCombineRgn( RgnWinupd, RgnWinupd, RgnTemp, RGN_OR );
		  
ERR("RgnTemp5: %d %d %d %d\n",RgnTemp->rdh.rcBound.left,RgnTemp->rdh.rcBound.right,RgnTemp->rdh.rcBound.top,RgnTemp->rdh.rcBound.bottom);

          co_UserRedrawWindow(Window, NULL, RgnTemp, rdw_flags );
		  
		  	//MoveToEx(hDC,RgnTemp->rdh.rcBound.right+Window->rcClient.left,RgnTemp->rdh.rcBound.bottom+Window->rcClient.top,NULL);
			//LineTo(hDC,RgnTemp->rdh.rcBound.left+Window->rcClient.left,RgnTemp->rdh.rcBound.top+Window->rcClient.top);

          REGION_Delete(RgnClip);
      }
   }
   REGION_Delete(RgnTemp);

	ERR("flags & SW_SCROLLCHILDREN %d\n",flags & SW_SCROLLCHILDREN);
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

         // Adjust window positions 
         RECTL_vOffsetRect(&Child->rcWindow, dx, dy);
         RECTL_vOffsetRect(&Child->rcClient, dx, dy);

         if (!prcScroll || RECTL_bIntersectRect(&rcDummy, &rcChild, &rcScroll))
         {
            UserRefObjectCo(Child, &WndRef);

            if (UserIsDesktopWindow(Window->spwndParent))
               lParam = MAKELONG(Child->rcClient.left, Child->rcClient.top);
            else
               lParam = MAKELONG(rcChild.left + dx, rcChild.top + dy);

            // wine sends WM_POSCHANGING, WM_POSCHANGED messages
            // windows sometimes a WM_MOVE
            co_IntSendMessage(UserHMGetHandle(Child), WM_MOVE, 0, lParam);

            UserDerefObjectCo(Child);
         }
      }
   }

	ERR("flags & (SW_INVALIDATE | SW_ERASE) %d\n",(flags & (SW_INVALIDATE | SW_ERASE)));
	ERR("RgnUpdate5: %d %d %d %d\n",RgnUpdate->rdh.rcBound.left,RgnUpdate->rdh.rcBound.right,RgnUpdate->rdh.rcBound.top,RgnUpdate->rdh.rcBound.bottom);
   if (flags & (SW_INVALIDATE | SW_ERASE))
   {
	   /*
	   RgnUpdate->rdh.rcBound.left = prcScroll->left;
	   RgnUpdate->rdh.rcBound.right = prcScroll->right;
	   RgnUpdate->rdh.rcBound.top = prcScroll->top;
	   RgnUpdate->rdh.rcBound.bottom = prcScroll->bottom;
	   */
	   //prcScroll->left,prcScroll->right,prcScroll->top,prcScroll->bottom);
	   //here
	   
	   
	   
	   
	   
	   
	  PREGION RgnClip = IntSysCreateRectpRgnIndirect(&rcClip);
      if (RgnClip)
      {
		  ERR("co_UserRedrawWindow 2\n");
          co_UserRedrawWindow( Window,
                           NULL,
                           RgnClip,
                           rdw_flags |                                    //    HACK    
                          ((flags & SW_SCROLLCHILDREN) ? RDW_ALLCHILDREN : RDW_NOCHILDREN) );

          REGION_Delete(RgnClip);
	  }

   }

	ERR("hwndCaret && (CaretWnd = UserGetWindowObject(hwndCaret)) %d\n",(hwndCaret && (CaretWnd = UserGetWindowObject(hwndCaret))));

   if (hwndCaret && (CaretWnd = UserGetWindowObject(hwndCaret)))
   {
      UserRefObjectCo(CaretWnd, &CaretRef);

      co_IntSetCaretPos(rcCaret.left + dx, rcCaret.top + dy);
      co_UserShowCaret(CaretWnd);

      UserDerefObjectCo(CaretWnd);
   }

	ERR("hrgnUpdate && (Result != ERROR) %d\n",(hrgnUpdate && (Result != ERROR)));
   if (hrgnUpdate && (Result != ERROR))
   {
       // Give everything back to the caller
       RgnTemp = REGION_LockRgn(hrgnUpdate);
       // The handle should still be valid
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
