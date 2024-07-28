/*
 * PROJECT:         ReactOS Win32k subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            win32ss/user/ntuser/windc.c
 * PURPOSE:         Window DC management
 * COPYRIGHT:       Copyright 2007 ReactOS Team
 */

#include <win32k.h>
DBG_DEFAULT_CHANNEL(UserDce);

/* GLOBALS *******************************************************************/

/* NOTE: I think we should store this per window station (including GDI objects) */
/* Answer: No, use the DCE pMonitor to compare with! */

static LIST_ENTRY LEDce;
static INT DCECount = 0; // Count of DCE in system.

BOOL BDCX_MYFLAG=FALSE;

#define DCX_CACHECOMPAREMASK (DCX_CLIPSIBLINGS | DCX_CLIPCHILDREN | \
                              DCX_NORESETATTRS | DCX_LOCKWINDOWUPDATE | \
                              DCX_LAYEREDWIN | DCX_CACHE | DCX_WINDOW | \
                              DCX_PARENTCLIP)

/* FUNCTIONS *****************************************************************/

#define DCX_MYFLAG   0x00200000

CODE_SEG("INIT")
NTSTATUS
NTAPI
InitDCEImpl(VOID)
{
    InitializeListHead(&LEDce);
    return STATUS_SUCCESS;
}

//
// This should be moved to dc.c or dcutil.c.
//
HDC FASTCALL
DceCreateDisplayDC(VOID)
{
  UNICODE_STRING DriverName = RTL_CONSTANT_STRING(L"DISPLAY");

  if (!co_IntGraphicsCheck(TRUE))
    KeBugCheckEx(VIDEO_DRIVER_INIT_FAILURE, 0, 0, 0, USER_VERSION);

  return IntGdiCreateDC(&DriverName, NULL, NULL, NULL, FALSE);
}

void CleanHwndDC(PDCE pdce)
{
	ERR("Clean HwndDC %x\n",pdce);
	for(int i=0;i<20;i++)
	{
		pdce->dceWnd[i].hwndCurrent=NULL;
		pdce->dceWnd[i].pwndOrg=NULL;
		pdce->dceWnd[i].pwndClip=NULL;
	}
}

HWND GetFirstHwndDC(PDCE pdce)
{
	ERR("GetFirstHwndDC %x  %x\n",pdce,pdce->dceWnd[0].hwndCurrent);
	return pdce->dceWnd[0].hwndCurrent;
}

/*
void SetFirstHwndDC(PDCE pdce, HWND hwnd)
{
	pdce->dceWnd[0].hwndCurrent = hwnd;
}*/

void SetFirstFreeHwndDC(PDCE pdce, PWND Window)
{
	if(Window==NULL)return;
	HWND hwnd = UserHMGetHandle(Window);
	ERR("Add HwndDC %x %x\n",pdce, hwnd);
	for(int i=0;i<20;i++)
	{
		if(pdce->dceWnd[i].hwndCurrent == hwnd)
			break;
		if(pdce->dceWnd[i].hwndCurrent == NULL)
		{
			pdce->dceWnd[i].hwndCurrent = hwnd;
			break;
		}
	}	
}

//SetFirstHwndDC
BOOL RemoveHwndDC(PDCE pdce, PWND Window)
{
	if(Window==NULL)return FALSE;
	HWND hwnd = UserHMGetHandle(Window);
	if(hwnd == NULL) return FALSE;
	int position = -1;
	for(int i=0;i<20;i++)
	{
		if(pdce->dceWnd[i].hwndCurrent==hwnd)
		{
			position=i;
			break;
		}
	}
	if(position==-1)
	{
		ERR("RemoveHwndDC-not deleted %x %x\n",pdce,hwnd);
		return FALSE;
	}
	for(int i=20-1;i>position;i--)
	{
		pdce->dceWnd[i-1].hwndCurrent=pdce->dceWnd[i].hwndCurrent;
	}
	ERR("RemoveHwndDC-deleted %x %x\n",pdce,hwnd);
	return TRUE;
}

BOOL ExistHwndDC(PDCE pdce, HWND hwnd)
{
	if(hwnd == NULL) return FALSE;
	for(int i=0;i<20;i++)
	{
		if(pdce->dceWnd[i].hwndCurrent==hwnd)
		{
			ERR("Get HwndDC %x %x\n",pdce, hwnd);
			return TRUE;
		}
	}
	ERR("Get HwndDC-no exist %x\n",hwnd);
	return FALSE;
}

/*
void CleanHwndDC(PDCE pdce)
{
	ERR("Clean HwndDC %x\n",pdce);
	for(int i=0;i<20;i++)
	{
		pdce->dceWnd[i].hwndCurrent=NULL;
		pdce->dceWnd[i].pwndOrg=NULL;
		pdce->dceWnd[i].pwndClip=NULL;
	}
}

HWND GetFirstHwndDC(PDCE pdce)
{
	ERR("GetFirstHwndDC %x  %x\n",pdce,pdce->dceWnd[0].hwndCurrent);
	return pdce->dceWnd[0].hwndCurrent;
}

void SetFirstHwndDC(PDCE pdce, HWND hwnd)
{
	pdce->dceWnd[0].hwndCurrent = hwnd;
}

void SetFirstFreeHwndDC(PDCE pdce, HWND hwnd)
{
	ERR("Add HwndDC %x %x\n",pdce, hwnd);
	for(int i=0;i<20;i++)
	{
		if(pdce->dceWnd[i].hwndCurrent == hwnd)
			break;
		if(pdce->dceWnd[i].hwndCurrent == NULL)
		{
			pdce->dceWnd[i].hwndCurrent = hwnd;
			break;
		}
	}	
}

//SetFirstHwndDC
BOOL RemoveHwndDC(PDCE pdce, HWND hwnd)
{
	if(hwnd == NULL) return FALSE;
	int position = -1;
	for(int i=0;i<20;i++)
	{
		if(pdce->dceWnd[i].hwndCurrent==hwnd)
		{
			position=i;
			break;
		}
	}
	if(position==-1)
	{
		ERR("RemoveHwndDC-not deleted %x %x\n",pdce,hwnd);
		return FALSE;
	}
	for(int i=20-1;i>position;i--)
	{
		pdce->dceWnd[i-1]=pdce->dceWnd[i];
	}
	ERR("RemoveHwndDC-deleted %x %x\n",pdce,hwnd);
	return TRUE;
}

BOOL ExistHwndDC(PDCE pdce, HWND hwnd)
{
	if(hwnd == NULL) return FALSE;
	for(int i=0;i<20;i++)
	{
		if(pdce->dceWnd[i].hwndCurrent==hwnd)
		{
			ERR("Get HwndDC %x %x\n",pdce, hwnd);
			return TRUE;
		}
	}
	ERR("Get HwndDC-no exist %x\n",hwnd);
	return FALSE;
}
*/

/* Returns the DCE pointer from the HDC handle */
DCE*
FASTCALL
DceGetDceFromDC(HDC hdc)
{
	if(BDCX_MYFLAG)ERR("DceGetDceFromDC\n");
    PLIST_ENTRY ListEntry;
    DCE* dce;

    ListEntry = LEDce.Flink;
    while (ListEntry != &LEDce)
    {
        dce = CONTAINING_RECORD(ListEntry, DCE, List);
        ListEntry = ListEntry->Flink;
        if (dce->hDC == hdc)
            return dce;
    }

    return NULL;
}

static
PREGION FASTCALL
DceGetVisRgn(PWND Window, ULONG Flags, HWND hWndChild, ULONG CFlags)
{
    PREGION Rgn;
    Rgn = VIS_ComputeVisibleRegion( Window,
                                    0 == (Flags & DCX_WINDOW),
                                    0 != (Flags & DCX_CLIPCHILDREN),
                                    0 != (Flags & DCX_CLIPSIBLINGS));
    /* Caller expects a non-null region */
    if (!Rgn)
        Rgn = IntSysCreateRectpRgn(0, 0, 0, 0);
    return Rgn;
}

PDCE FASTCALL
DceAllocDCE(PWND Window OPTIONAL, DCE_TYPE Type)
{
  PDCE pDce;

  pDce = ExAllocatePoolWithTag(PagedPool, sizeof(DCE), USERTAG_DCE);
  if(!pDce)
        return NULL;

  pDce->hDC = DceCreateDisplayDC();
  if (!pDce->hDC)
  {
      ExFreePoolWithTag(pDce, USERTAG_DCE);
      return NULL;
  }
  DCECount++;
  TRACE("Alloc DCE's! %d\n",DCECount);
  //pDce->hwndCurrent = (Window ? UserHMGetHandle(Window) : NULL);
  CleanHwndDC(pDce);
  //SetFirstHwndDC(pDce, (Window ? UserHMGetHandle(Window) : NULL));
  SetFirstFreeHwndDC(pDce, Window);
  pDce->pwndOrg  = Window;
  pDce->pwndClip = Window;
  pDce->hrgnClip = NULL;
  pDce->hrgnClipPublic = NULL;
  pDce->hrgnSavedVis = NULL;
  pDce->ppiOwner = NULL;

  InsertTailList(&LEDce, &pDce->List);

  DCU_SetDcUndeletable(pDce->hDC);

  if (Type == DCE_WINDOW_DC || Type == DCE_CLASS_DC) // Window DCE have ownership.
  {
     pDce->ptiOwner = GetW32ThreadInfo();
  }
  else
  {
     TRACE("FREE DCATTR!!!! NOT DCE_WINDOW_DC!!!!! hDC-> %p\n", pDce->hDC);
     GreSetDCOwner(pDce->hDC, GDI_OBJ_HMGR_NONE);
     pDce->ptiOwner = NULL;
  }

  if (Type == DCE_CACHE_DC)
  {
     pDce->DCXFlags = DCX_CACHE | DCX_DCEEMPTY;
  }
  else
  {
     pDce->DCXFlags = DCX_DCEBUSY;
     if (Window)
     {
        if (Type == DCE_WINDOW_DC)
        {
          if (Window->style & WS_CLIPCHILDREN) pDce->DCXFlags |= DCX_CLIPCHILDREN;
          if (Window->style & WS_CLIPSIBLINGS) pDce->DCXFlags |= DCX_CLIPSIBLINGS;
        }
     }
  }
  return(pDce);
}

static VOID APIENTRY
DceSetDrawable( PWND Window OPTIONAL,
                HDC hDC,
                ULONG Flags,
                BOOL SetClipOrigin)
{
  RECTL rect = {0,0,0,0};

  if (Window)
  {
      if (Flags & DCX_WINDOW)
      {
         rect = Window->rcWindow;
      }
      else
      {
         rect = Window->rcClient;
      }
  }

  /* Set DC Origin and Window Rectangle */
  GreSetDCOrg( hDC, rect.left, rect.top, &rect);
}


static VOID FASTCALL
DceDeleteClipRgn(DCE* Dce)
{
   Dce->DCXFlags &= ~(DCX_EXCLUDERGN | DCX_INTERSECTRGN);

   if (Dce->DCXFlags & DCX_KEEPCLIPRGN )
   {
      Dce->DCXFlags &= ~DCX_KEEPCLIPRGN;
   }
   else if (Dce->hrgnClip != NULL)
   {
      GreDeleteObject(Dce->hrgnClip);
   }

   Dce->hrgnClip = NULL;

   /* Make it dirty so that the vis rgn gets recomputed next time */
   Dce->DCXFlags |= DCX_DCEDIRTY;
   IntGdiSetHookFlags(Dce->hDC, DCHF_INVALIDATEVISRGN);
}

VOID
FASTCALL
DceUpdateVisRgn(DCE *Dce, PWND Window, ULONG Flags)
{
   PREGION RgnVisible = NULL;
   ULONG DcxFlags;
   PWND DesktopWindow;

   if (Flags & DCX_PARENTCLIP)
   {
      PWND Parent;

      Parent = Window->spwndParent;
      if (!Parent)
      {
         RgnVisible = NULL;
         goto noparent;
      }

      if (Parent->style & WS_CLIPSIBLINGS)
      {
         DcxFlags = DCX_CLIPSIBLINGS |
                    (Flags & ~(DCX_CLIPCHILDREN | DCX_WINDOW));
      }
      else
      {
         DcxFlags = Flags & ~(DCX_CLIPSIBLINGS | DCX_CLIPCHILDREN | DCX_WINDOW);
      }
      RgnVisible = DceGetVisRgn(Parent, DcxFlags, UserHMGetHandle(Window), Flags);
   }
   else if (Window == NULL)
   {
      DesktopWindow = UserGetWindowObject(IntGetDesktopWindow());
      if (NULL != DesktopWindow)
      {
         RgnVisible = IntSysCreateRectpRgnIndirect(&DesktopWindow->rcWindow);
      }
      else
      {
         RgnVisible = NULL;
      }
   }
   else
   {
      RgnVisible = DceGetVisRgn(Window, Flags, 0, 0);
   }

noparent:
   if (Flags & DCX_INTERSECTRGN)
   {
      PREGION RgnClip = NULL;

      if (Dce->hrgnClip != NULL)
          RgnClip = REGION_LockRgn(Dce->hrgnClip);

      if (RgnClip)
      {
         IntGdiCombineRgn(RgnVisible, RgnVisible, RgnClip, RGN_AND);
         REGION_UnlockRgn(RgnClip);
      }
      else
      {
         if (RgnVisible != NULL)
         {
            REGION_Delete(RgnVisible);
         }
         RgnVisible = IntSysCreateRectpRgn(0, 0, 0, 0);
      }
   }
   else if ((Flags & DCX_EXCLUDERGN) && Dce->hrgnClip != NULL)
   {
       PREGION RgnClip = REGION_LockRgn(Dce->hrgnClip);
       IntGdiCombineRgn(RgnVisible, RgnVisible, RgnClip, RGN_DIFF);
       REGION_UnlockRgn(RgnClip);
   }

   Dce->DCXFlags &= ~DCX_DCEDIRTY;
   GdiSelectVisRgn(Dce->hDC, RgnVisible);
   /* Tell GDI driver */
   if (Window)
       IntEngWindowChanged(Window, WOC_RGN_CLIENT);

   if (RgnVisible != NULL)
   {
      REGION_Delete(RgnVisible);
   }
}

static INT FASTCALL
DceReleaseDC(PWND Window, DCE* dce, BOOL EndPaint)
{
	if(BDCX_MYFLAG)ERR("DceReleaseDC\n");
   if (DCX_DCEBUSY != (dce->DCXFlags & (DCX_INDESTROY | DCX_DCEEMPTY | DCX_DCEBUSY)))
   {
      return 0;
   }

   /* Restore previous visible region */
   if (EndPaint)
   {
      DceUpdateVisRgn(dce, dce->pwndOrg, dce->DCXFlags);
   }

   if ((dce->DCXFlags & (DCX_INTERSECTRGN | DCX_EXCLUDERGN)) &&
         ((dce->DCXFlags & DCX_CACHE) || EndPaint))
   {
      DceDeleteClipRgn(dce);
   }

	if(BDCX_MYFLAG)ERR("dce->DCXFlags & DCX_CACHE %d\n",(dce->DCXFlags & DCX_CACHE));
   if (dce->DCXFlags & DCX_CACHE)
   {
      if (!(dce->DCXFlags & DCX_NORESETATTRS))
      {
         // Clean the DC
         if (!IntGdiCleanDC(dce->hDC)) return 0;

         if (dce->DCXFlags & DCX_DCEDIRTY)
         {
           /* Don't keep around invalidated entries
            * because SetDCState() disables hVisRgn updates
            * by removing dirty bit. */
           //dce->hwndCurrent = 0;
		   //SetFirstHwndDC(dce, 0);
		   RemoveHwndDC(dce,(Window ? UserHMGetHandle(Window) : NULL));
           dce->pwndOrg  = NULL;
           dce->pwndClip = NULL;
           dce->DCXFlags &= DCX_CACHE;
           dce->DCXFlags |= DCX_DCEEMPTY;
         }
      }
      dce->DCXFlags &= ~DCX_DCEBUSY;
      TRACE("Exit!!!!! DCX_CACHE!!!!!!   hDC-> %p \n", dce->hDC);
      if (!GreSetDCOwner(dce->hDC, GDI_OBJ_HMGR_NONE))
         return 0;
      dce->ptiOwner = NULL; // Reset ownership.
      dce->ppiOwner = NULL;

#if 0 // Need to research and fix before this is a "growing" issue.
      if (++DCECache > 32)
      {
         ListEntry = LEDce.Flink;
         while (ListEntry != &LEDce)
         {
            pDCE = CONTAINING_RECORD(ListEntry, DCE, List);
            ListEntry = ListEntry->Flink;
            if (!(pDCE->DCXFlags & DCX_DCEBUSY))
            {  /* Free the unused cache DCEs. */
               DceFreeDCE(pDCE, TRUE);
            }
         }
      }
#endif
   }
   return 1; // Released!
}


HDC FASTCALL
UserGetDCEx(PWND Wnd OPTIONAL, HANDLE ClipRegion, ULONG Flags)
{
   PWND Parent;
   ULONG DcxFlags;
   DCE* Dce = NULL;
   BOOL UpdateClipOrigin = FALSE;
   BOOL bUpdateVisRgn = TRUE;
   HDC hDC = NULL;
   PPROCESSINFO ppi;
   PLIST_ENTRY ListEntry;
   
   BDCX_MYFLAG=FALSE;
   if(Flags & DCX_MYFLAG)
   {
		ERR("UserGetDCEx - DCX_MYFLAG %d\n",Flags);
		BDCX_MYFLAG=TRUE;
		ListEntry = LEDce.Flink;
		  while (ListEntry != &LEDce)
		  {
			  if(BDCX_MYFLAG) ERR("ListEntry\n");
			 Dce = CONTAINING_RECORD(ListEntry, DCE, List);
			 ListEntry = ListEntry->Flink;
			 ERR(" %x ",Dce->hDC);
			 /*if (Dce->hDC == hDC)
			 {
				ERR("UserGetDCEx-flags: %d",Dce->DCXFlags);
			 }*/
		  }
		  Dce = NULL;
   }

   if (NULL == Wnd)
   {
      Flags &= ~DCX_USESTYLE;
      Flags |= DCX_CACHE;
   }

   if (Flags & DCX_PARENTCLIP) Flags |= DCX_CACHE;

   // When GetDC is called with hWnd nz, DCX_CACHE & _WINDOW are clear w _USESTYLE set.
   if (Flags & DCX_USESTYLE)
   {
      Flags &= ~(DCX_CLIPCHILDREN | DCX_CLIPSIBLINGS | DCX_PARENTCLIP);
      if (!(Flags & DCX_WINDOW)) // Not window rectangle
      {
         if (Wnd->pcls->style & CS_PARENTDC)
         {
            Flags |= DCX_PARENTCLIP;
         }

         if (!(Flags & DCX_CACHE) && // Not on the cheap wine list.
             !(Wnd->pcls->style & CS_OWNDC) )
         {
            if (!(Wnd->pcls->style & CS_CLASSDC))
            // The window is not POWNED or has any CLASS, so we are looking for cheap wine.
               Flags |= DCX_CACHE;
            else
            {
               if (Wnd->pcls->pdce) hDC = ((PDCE)Wnd->pcls->pdce)->hDC;
               TRACE("We have CLASS!!\n");
            }
         }

         if (Wnd->style & WS_CLIPSIBLINGS)
         {
            Flags |= DCX_CLIPSIBLINGS;
         }

         if (Wnd->style & WS_CLIPCHILDREN &&
             !(Wnd->style & WS_MINIMIZE))
         {
            Flags |= DCX_CLIPCHILDREN;
         }
         /* If minized with icon in the set, we are forced to be cheap! */
         if (Wnd->style & WS_MINIMIZE && Wnd->pcls->spicn)
         {
            Flags |= DCX_CACHE;
         }
      }
      else
      {
         if (Wnd->style & WS_CLIPSIBLINGS) Flags |= DCX_CLIPSIBLINGS;
         Flags |= DCX_CACHE;
      }
   }

   if (Flags & DCX_WINDOW) Flags &= ~DCX_CLIPCHILDREN;

   if (Flags & DCX_NOCLIPCHILDREN)
   {
      Flags |= DCX_CACHE;
      Flags &= ~(DCX_PARENTCLIP | DCX_CLIPCHILDREN);
   }

   Parent = (Wnd ? Wnd->spwndParent : NULL);

   if (NULL == Wnd || !(Wnd->style & WS_CHILD) || NULL == Parent)
   {
      Flags &= ~DCX_PARENTCLIP;
      Flags |= DCX_CLIPSIBLINGS;
   }

   /* It seems parent clip is ignored when clipping siblings or children */
   if (Flags & (DCX_CLIPSIBLINGS | DCX_CLIPCHILDREN)) Flags &= ~DCX_PARENTCLIP;

   if (Flags & DCX_PARENTCLIP)
   {
      if ((Wnd->style & WS_VISIBLE) &&
          (Parent->style & WS_VISIBLE))
      {
         Flags &= ~DCX_CLIPCHILDREN;
         if (Parent->style & WS_CLIPSIBLINGS)
         {
            Flags |= DCX_CLIPSIBLINGS;
         }
      }
   }

   // Window nz, check to see if we still own this or it is just cheap wine tonight.
   if (!(Flags & DCX_CACHE))
   {
      if ( Wnd->head.pti != GetW32ThreadInfo())
         Flags |= DCX_CACHE; // Ah~ Not Powned! Forced to be cheap~
   }

   DcxFlags = Flags & DCX_CACHECOMPAREMASK;

   if (Flags & DCX_CACHE)
   { // Scan the cheap wine list for our match.
      if(BDCX_MYFLAG) ERR("DCX_CACHE\n");
      DCE* DceEmpty = NULL;
      DCE* DceUnused = NULL;
      KeEnterCriticalRegion();
      ListEntry = LEDce.Flink;
      while (ListEntry != &LEDce)
      {
         Dce = CONTAINING_RECORD(ListEntry, DCE, List);
         ListEntry = ListEntry->Flink;
//
// The way I understand this, you can have more than one DC per window.
// Only one Owned if one was requested and saved and one Cached.
//
         if ((Dce->DCXFlags & (DCX_CACHE | DCX_DCEBUSY)) == DCX_CACHE)
         {
            DceUnused = Dce;
            if (Dce->DCXFlags & DCX_DCEEMPTY)
            {
               DceEmpty = Dce;
            }
            //else if (Dce->hwndCurrent == (Wnd ? UserHMGetHandle(Wnd) : NULL) &&
			//else if (GetFirstHwndDC(Dce) == (Wnd ? UserHMGetHandle(Wnd) : NULL) &&
			else if (ExistHwndDC(Dce, (Wnd ? UserHMGetHandle(Wnd) : NULL)) &&
                     ((Dce->DCXFlags & DCX_CACHECOMPAREMASK) == DcxFlags))
            {
               UpdateClipOrigin = TRUE;
               break;
            }
         }
         Dce = NULL; // Loop issue?
      }
      KeLeaveCriticalRegion();

      Dce = (DceEmpty == NULL) ? DceUnused : DceEmpty;

      if (Dce == NULL)
      {
         Dce = DceAllocDCE(NULL, DCE_CACHE_DC);
      }
      if (Dce == NULL) return NULL;

      //Dce->hwndCurrent = (Wnd ? UserHMGetHandle(Wnd) : NULL);
	  //SetFirstHwndDC(Dce, (Wnd ? UserHMGetHandle(Wnd) : NULL));
	  SetFirstFreeHwndDC(Dce, Wnd);
      Dce->pwndOrg = Dce->pwndClip = Wnd;
   }
   else // If we are here, we are POWNED or having CLASS.
   {
	   if(BDCX_MYFLAG)ERR("NO-DCX_CACHE\n");
      KeEnterCriticalRegion();
      ListEntry = LEDce.Flink;
      while (ListEntry != &LEDce)
      {
		  if(BDCX_MYFLAG)ERR("ListEntry\n");
          Dce = CONTAINING_RECORD(ListEntry, DCE, List);
          ListEntry = ListEntry->Flink;

          // Skip Cache DCE entries.
          if (!(Dce->DCXFlags & DCX_CACHE))
          {
			  if(BDCX_MYFLAG)
			  {
			  ERR("ListEntry B: %x %x\n",GetFirstHwndDC(Dce),UserHMGetHandle(Wnd));
			  ERR("ListEntry C: %x %x\n",Dce->hDC,hDC);
			  }
             // Check for Window handle than HDC match for CLASS.
             //if (Dce->hwndCurrent == UserHMGetHandle(Wnd))
		     //if (GetFirstHwndDC(Dce) == UserHMGetHandle(Wnd))
			 if (ExistHwndDC(Dce, UserHMGetHandle(Wnd)))
             {
                bUpdateVisRgn = FALSE;
				if(BDCX_MYFLAG)ERR(" A ");
                break;
             }
             else
				if (Dce->hDC == hDC)
				{
					if(BDCX_MYFLAG)ERR(" B-second DCE ");
					break;
				}
				
          }
		  if(BDCX_MYFLAG)ERR(" C ");
          Dce = NULL; // Loop issue?
      }
      KeLeaveCriticalRegion();
	  
	  if(BDCX_MYFLAG)ERR(" EEE %x\n",Dce);

      if (Dce == NULL)
      {
         return(NULL);
      }

      if ( (Flags & (DCX_INTERSECTRGN|DCX_EXCLUDERGN)) &&
           (Dce->DCXFlags & (DCX_INTERSECTRGN|DCX_EXCLUDERGN)) )
      {
         DceDeleteClipRgn(Dce);
      }
   }
// First time use hax, need to use DceAllocDCE during window display init.
   if (NULL == Dce)
   {
      return(NULL);
   }

   if (!GreIsHandleValid(Dce->hDC))
   {
      ERR("FIXME: Got DCE with invalid hDC! %p\n", Dce->hDC);
      Dce->hDC = DceCreateDisplayDC();
      /* FIXME: Handle error */
   }

   Dce->DCXFlags = Flags | DCX_DCEBUSY;

   /*
    * Bump it up! This prevents the random errors in wine dce tests and with
    * proper bits set in DCX_CACHECOMPAREMASK.
    * Reference:
    *   https://reactos.org/archives/public/ros-dev/2008-July/010498.html
    *   https://reactos.org/archives/public/ros-dev/2008-July/010499.html
    */
   RemoveEntryList(&Dce->List);
   InsertHeadList(&LEDce, &Dce->List);

   /* Introduced in rev 6691 and modified later. */
   if ( (Flags & DCX_INTERSECTUPDATE) && !ClipRegion )
   {
      Flags |= DCX_INTERSECTRGN | DCX_KEEPCLIPRGN;
      Dce->DCXFlags |= DCX_INTERSECTRGN | DCX_KEEPCLIPRGN;
      ClipRegion = Wnd->hrgnUpdate;
      bUpdateVisRgn = TRUE;
   }

   if (ClipRegion == HRGN_WINDOW)
   {
      if (!(Flags & DCX_WINDOW))
      {
         Dce->hrgnClip = NtGdiCreateRectRgn(
             Wnd->rcClient.left,
             Wnd->rcClient.top,
             Wnd->rcClient.right,
             Wnd->rcClient.bottom);
      }
      else
      {
          Dce->hrgnClip = NtGdiCreateRectRgn(
              Wnd->rcWindow.left,
              Wnd->rcWindow.top,
              Wnd->rcWindow.right,
              Wnd->rcWindow.bottom);
      }
      Dce->DCXFlags &= ~DCX_KEEPCLIPRGN;
      bUpdateVisRgn = TRUE;
   }
   else if (ClipRegion != NULL)
   {
      if (Dce->hrgnClip != NULL)
      {
         ERR("Should not be called!!\n");
         GreDeleteObject(Dce->hrgnClip);
         Dce->hrgnClip = NULL;
      }
      Dce->hrgnClip = ClipRegion;
      bUpdateVisRgn = TRUE;
   }

   if (IntGdiSetHookFlags(Dce->hDC, DCHF_VALIDATEVISRGN)) bUpdateVisRgn = TRUE;

   DceSetDrawable(Wnd, Dce->hDC, Flags, UpdateClipOrigin);

   if (bUpdateVisRgn) DceUpdateVisRgn(Dce, Wnd, Flags);

   if (Dce->DCXFlags & DCX_CACHE)
   {
      TRACE("ENTER!!!!!! DCX_CACHE!!!!!!   hDC-> %p\n", Dce->hDC);
      // Need to set ownership so Sync dcattr will work.
      GreSetDCOwner(Dce->hDC, GDI_OBJ_HMGR_POWNED);
      Dce->ptiOwner = GetW32ThreadInfo(); // Set the temp owning
   }

   if ( Wnd &&
        Wnd->ExStyle & WS_EX_LAYOUTRTL &&
       !(Flags & DCX_KEEPLAYOUT) )
   {
      NtGdiSetLayout(Dce->hDC, -1, LAYOUT_RTL);
   }

   if (Dce->DCXFlags & DCX_PROCESSOWNED)
   {
      ppi = PsGetCurrentProcessWin32Process();
      ppi->W32PF_flags |= W32PF_OWNDCCLEANUP;
      Dce->ptiOwner = NULL;
      Dce->ppiOwner = ppi;
   }

   return(Dce->hDC);
}

/***********************************************************************
 *           DceFreeDCE
 */
void FASTCALL
DceFreeDCE(PDCE pdce, BOOLEAN Force)
{
  BOOL Hit = FALSE;

  ASSERT(pdce != NULL);
  if (NULL == pdce) return;

  pdce->DCXFlags |= DCX_INDESTROY;

  if (Force &&
      GreGetObjectOwner(pdce->hDC) != GDI_OBJ_HMGR_POWNED)
  {
     TRACE("Change ownership for DCE! -> %p\n" , pdce);
     // NOTE: Windows sets W32PF_OWNDCCLEANUP and moves on.
     if (GreIsHandleValid(pdce->hDC))
     {
         GreSetDCOwner(pdce->hDC, GDI_OBJ_HMGR_POWNED);
     }
     else
     {
         ERR("Attempted to change ownership of an DCEhDC %p currently being destroyed!!!\n",
             pdce->hDC);
         Hit = TRUE;
     }
  }
  else
  {
     if (GreGetObjectOwner(pdce->hDC) == GDI_OBJ_HMGR_PUBLIC)
        GreSetDCOwner(pdce->hDC, GDI_OBJ_HMGR_POWNED);
  }

  if (!Hit) IntGdiDeleteDC(pdce->hDC, TRUE);

  if (pdce->hrgnClip && !(pdce->DCXFlags & DCX_KEEPCLIPRGN))
  {
      GreDeleteObject(pdce->hrgnClip);
      pdce->hrgnClip = NULL;
  }

  RemoveEntryList(&pdce->List);

  ExFreePoolWithTag(pdce, USERTAG_DCE);

  DCECount--;
  TRACE("Freed DCE's! %d \n", DCECount);
}

/***********************************************************************
 *           DceFreeWindowDCE
 *
 * Remove owned DCE and reset unreleased cache DCEs.
 */
void FASTCALL
DceFreeWindowDCE(PWND Window)
{
  PDCE pDCE;
  PLIST_ENTRY ListEntry;
  
  if(BDCX_MYFLAG)ERR("DceFreeWindowDCE\n");

  if (DCECount <= 0)
  {
     ERR("FreeWindowDCE No Entry! %d\n",DCECount);
     return;
  }

  ListEntry = LEDce.Flink;
  while (ListEntry != &LEDce)
  {
     pDCE = CONTAINING_RECORD(ListEntry, DCE, List);
     ListEntry = ListEntry->Flink;
     //if ( pDCE->hwndCurrent == UserHMGetHandle(Window) &&
	 //if ( GetFirstHwndDC(pDCE) == UserHMGetHandle(Window) &&
	 if ( ExistHwndDC(pDCE,UserHMGetHandle(Window)) &&
          !(pDCE->DCXFlags & DCX_DCEEMPTY) )
     {
        if (!(pDCE->DCXFlags & DCX_CACHE)) /* Owned or Class DCE */
        {
           if (Window->pcls->style & CS_CLASSDC) /* Test Class first */
           {
              if (pDCE->DCXFlags & (DCX_INTERSECTRGN | DCX_EXCLUDERGN)) /* Class DCE */
                 DceDeleteClipRgn(pDCE);
              // Update and reset Vis Rgn and clear the dirty bit.
              // Should release VisRgn than reset it to default.
              DceUpdateVisRgn(pDCE, Window, pDCE->DCXFlags);
              pDCE->DCXFlags = DCX_DCEEMPTY|DCX_CACHE;
              //pDCE->hwndCurrent = 0;
			  //SetFirstHwndDC(pDCE, 0);
			  //SetFirstHwndDC(pDCE, (Window ? UserHMGetHandle(Window) : NULL));
			  RemoveHwndDC(pDCE,Window);
              pDCE->pwndOrg = pDCE->pwndClip = NULL;

              TRACE("POWNED DCE going Cheap!! DCX_CACHE!! hDC-> %p \n",
                    pDCE->hDC);
              if (!GreSetDCOwner( pDCE->hDC, GDI_OBJ_HMGR_NONE))
              {
                  ERR("Fail Owner Switch hDC-> %p \n", pDCE->hDC);
                  break;
              }
              /* Do not change owner so thread can clean up! */
           }
           else if (Window->pcls->style & CS_OWNDC) /* Owned DCE */
           {
              DceFreeDCE(pDCE, FALSE);
              continue;
           }
           else
           {
              ERR("Not POWNED or CLASSDC hwndCurrent -> %p \n",
                  //pDCE->hwndCurrent);
				  GetFirstHwndDC(pDCE));
              // ASSERT(FALSE); /* bug 5320 */
           }
        }
        else
        {
           if (pDCE->DCXFlags & DCX_DCEBUSY) /* Shared cache DCE */
           {
              /* FIXME: AFAICS we are doing the right thing here so
               * this should be a TRACE. But this is best left as an ERR
               * because the 'application error' is likely to come from
               * another part of Wine (i.e. it's our fault after all).
               * We should change this to TRACE when ReactOS is more stable
               * (for 1.0?).
               */
              ERR("[%p] GetDC() without ReleaseDC()!\n", UserHMGetHandle(Window));
              DceReleaseDC(Window, pDCE, FALSE);//xxxxxxxx release All DC?
           }
           pDCE->DCXFlags |= DCX_DCEEMPTY;
           //pDCE->hwndCurrent = 0;
		   //SetFirstHwndDC(pDCE, 0);
		   RemoveHwndDC(dce,(Window ? UserHMGetHandle(Window) : NULL));
           pDCE->pwndOrg = pDCE->pwndClip = NULL;
        }
     }
  }
}

void FASTCALL
DceFreeClassDCE(PDCE pdceClass)
{
   PDCE pDCE;
   PLIST_ENTRY ListEntry;
   
   if(BDCX_MYFLAG)ERR("DceFreeClassDCE\n");

   ListEntry = LEDce.Flink;
   while (ListEntry != &LEDce)
   {
       pDCE = CONTAINING_RECORD(ListEntry, DCE, List);
       ListEntry = ListEntry->Flink;
       if (pDCE == pdceClass)
       {
          DceFreeDCE(pDCE, TRUE); // Might have gone cheap!
       }
   }
}

void FASTCALL
DceFreeThreadDCE(PTHREADINFO pti)
{
   PDCE pDCE;
   PLIST_ENTRY ListEntry;
   
   if(BDCX_MYFLAG)ERR("DceFreeThreadDCE\n");

   ListEntry = LEDce.Flink;
   while (ListEntry != &LEDce)
   {
       pDCE = CONTAINING_RECORD(ListEntry, DCE, List);
       ListEntry = ListEntry->Flink;
       if (pDCE->ptiOwner == pti)
       {
          if (pDCE->DCXFlags & DCX_CACHE)
          {
             DceFreeDCE(pDCE, TRUE);
          }
       }
   }
}

VOID FASTCALL
DceEmptyCache(VOID)
{
   PDCE pDCE;
   PLIST_ENTRY ListEntry;
   
   if(BDCX_MYFLAG)ERR("DceEmptyCache\n");

   ListEntry = LEDce.Flink;
   while (ListEntry != &LEDce)
   {
      pDCE = CONTAINING_RECORD(ListEntry, DCE, List);
      ListEntry = ListEntry->Flink;
      DceFreeDCE(pDCE, TRUE);
   }
}

VOID FASTCALL
DceResetActiveDCEs(PWND Window)
{
   DCE *pDCE;
   PDC dc;
   PWND CurrentWindow;
   INT DeltaX;
   INT DeltaY;
   PLIST_ENTRY ListEntry;
   
   if(BDCX_MYFLAG)ERR("DceResetActiveDCEs\n");

   if (NULL == Window)
   {
      return;
   }

   ListEntry = LEDce.Flink;
   while (ListEntry != &LEDce)
   {
      pDCE = CONTAINING_RECORD(ListEntry, DCE, List);
      ListEntry = ListEntry->Flink;
      if (0 == (pDCE->DCXFlags & (DCX_DCEEMPTY|DCX_INDESTROY)))
      {
         //if (UserHMGetHandle(Window) == pDCE->hwndCurrent)
	     //if (UserHMGetHandle(Window) == GetFirstHwndDC(pDCE))
		 if (ExistHwndDC(pDCE, UserHMGetHandle(Window)))
         {
            CurrentWindow = Window;
         }
         else
         {
			 /*
            //if (!pDCE->hwndCurrent)
			if (!GetFirstHwndDC(pDCE))
               CurrentWindow = NULL;
            else
               //CurrentWindow = UserGetWindowObject(pDCE->hwndCurrent);
		       CurrentWindow = UserGetWindowObject(GetFirstHwndDC(pDCE));
            if (NULL == CurrentWindow)
            {
               continue;
            }
			*/
			CurrentWindow = NULL;
			continue;
         }

         if (!GreIsHandleValid(pDCE->hDC) ||
             (dc = DC_LockDc(pDCE->hDC)) == NULL)
         {
            continue;
         }
         if (Window == CurrentWindow || IntIsChildWindow(Window, CurrentWindow))
         {
            if (pDCE->DCXFlags & DCX_WINDOW)
            {
               DeltaX = CurrentWindow->rcWindow.left - dc->ptlDCOrig.x;
               DeltaY = CurrentWindow->rcWindow.top - dc->ptlDCOrig.y;
               dc->ptlDCOrig.x = CurrentWindow->rcWindow.left;
               dc->ptlDCOrig.y = CurrentWindow->rcWindow.top;
            }
            else
            {
               DeltaX = CurrentWindow->rcClient.left - dc->ptlDCOrig.x;
               DeltaY = CurrentWindow->rcClient.top - dc->ptlDCOrig.y;
               dc->ptlDCOrig.x = CurrentWindow->rcClient.left;
               dc->ptlDCOrig.y = CurrentWindow->rcClient.top;
            }

            if (NULL != dc->dclevel.prgnClip)
            {
               REGION_bOffsetRgn(dc->dclevel.prgnClip, DeltaX, DeltaY);
               dc->fs |= DC_DIRTY_RAO;
            }
            if (NULL != pDCE->hrgnClip)
            {
               NtGdiOffsetRgn(pDCE->hrgnClip, DeltaX, DeltaY);
            }
         }
         DC_UnlockDc(dc);

         DceUpdateVisRgn(pDCE, CurrentWindow, pDCE->DCXFlags);
         IntGdiSetHookFlags(pDCE->hDC, DCHF_VALIDATEVISRGN);
      }
   }
}

HWND FASTCALL
IntWindowFromDC(HDC hDc)
{
  DCE *Dce;
  PLIST_ENTRY ListEntry;
  HWND Ret = NULL;
  
  if(BDCX_MYFLAG)ERR("IntWindowFromDC\n");

  ListEntry = LEDce.Flink;
  while (ListEntry != &LEDce)
  {
      Dce = CONTAINING_RECORD(ListEntry, DCE, List);
      ListEntry = ListEntry->Flink;
      if (Dce->hDC == hDc)
      {
         if (Dce->DCXFlags & DCX_INDESTROY)
            Ret = NULL;
         else
            //Ret = Dce->hwndCurrent;
		    Ret = GetFirstHwndDC(Dce);
         break;
      }
  }
  return Ret;
}

INT FASTCALL
UserReleaseDC(PWND Window, HDC hDc, BOOL EndPaint)
{
  PDCE dce;
  PLIST_ENTRY ListEntry;
  INT nRet = 0;
  BOOL Hit = FALSE;
  
  if(BDCX_MYFLAG)
  {
	  ERR("UserReleaseDC: %x\n",hDc);
	  ListEntry = LEDce.Flink;
	  while (ListEntry != &LEDce)
	  {
		 dce = CONTAINING_RECORD(ListEntry, DCE, List);
		 ListEntry = ListEntry->Flink;
		 ERR(" %x ",dce->hDC);
		 if (dce->hDC == hDc)
		 {
			ERR("UserReleaseDC-flags: %d",dce->DCXFlags);
		 }
	  }
	  dce = NULL;
  }

  TRACE("%p %p\n", Window, hDc);
  ListEntry = LEDce.Flink;
  while (ListEntry != &LEDce)
  {
     dce = CONTAINING_RECORD(ListEntry, DCE, List);
     ListEntry = ListEntry->Flink;
     if (dce->hDC == hDc)
     {
        Hit = TRUE;
        break;
     }
  }

  if ( Hit && (dce->DCXFlags & DCX_DCEBUSY))
  {
     nRet = DceReleaseDC(Window, dce, EndPaint);
  }

  return nRet;
}

HDC FASTCALL
UserGetWindowDC(PWND Wnd)
{
  return UserGetDCEx(Wnd, 0, DCX_USESTYLE | DCX_WINDOW);
}

HWND FASTCALL
UserGethWnd( HDC hdc, PWNDOBJ *pwndo)
{
  EWNDOBJ* Clip;
  PWND Wnd;
  HWND hWnd;

  hWnd = IntWindowFromDC(hdc);

  if (hWnd && (Wnd = UserGetWindowObject(hWnd)))
  {
     Clip = (EWNDOBJ*)UserGetProp(Wnd, AtomWndObj, TRUE);

     if ( Clip && Clip->Hwnd == hWnd )
     {
        if (pwndo) *pwndo = (PWNDOBJ)Clip;
     }
  }
  return hWnd;
}

HDC APIENTRY
NtUserGetDCEx(HWND hWnd OPTIONAL, HANDLE ClipRegion, ULONG Flags)
{
  PWND Wnd=NULL;
  HDC Ret = NULL;

  TRACE("Enter NtUserGetDCEx: hWnd %p, ClipRegion %p, Flags %x.\n",
      hWnd, ClipRegion, Flags);
  UserEnterExclusive();

  if (hWnd && !(Wnd = UserGetWindowObject(hWnd)))
  {
      goto Exit; // Return NULL
  }
  Ret = UserGetDCEx(Wnd, ClipRegion, Flags);

Exit:
  TRACE("Leave NtUserGetDCEx, ret=%p\n", Ret);
  UserLeave();
  return Ret;
}

/*
 * NtUserGetWindowDC
 *
 * The NtUserGetWindowDC function retrieves the device context (DC) for the
 * entire window, including title bar, menus, and scroll bars. A window device
 * context permits painting anywhere in a window, because the origin of the
 * device context is the upper-left corner of the window instead of the client
 * area.
 *
 * Status
 *    @implemented
 */
HDC APIENTRY
NtUserGetWindowDC(HWND hWnd)
{
  return NtUserGetDCEx(hWnd, 0, DCX_USESTYLE | DCX_WINDOW);
}

HDC APIENTRY
NtUserGetDC(HWND hWnd)
{
 TRACE("NtUGetDC -> %p:%x\n", hWnd, !hWnd ? DCX_CACHE | DCX_WINDOW : DCX_USESTYLE);

  return NtUserGetDCEx(hWnd, NULL, NULL == hWnd ? DCX_CACHE | DCX_WINDOW : DCX_USESTYLE);
}

/*!
 * Select logical palette into device context.
 * \param	hDC 				handle to the device context
 * \param	hpal				handle to the palette
 * \param	ForceBackground 	If this value is FALSE the logical palette will be copied to the device palette only when the application
 * 								is in the foreground. If this value is TRUE then map the colors in the logical palette to the device
 * 								palette colors in the best way.
 * \return	old palette
 *
 * \todo	implement ForceBackground == TRUE
*/
HPALETTE
APIENTRY
NtUserSelectPalette(HDC  hDC,
              HPALETTE  hpal,
       BOOL  ForceBackground)
{
    HPALETTE oldPal;
    UserEnterExclusive();
    // Implement window checks
    oldPal = GdiSelectPalette( hDC, hpal, ForceBackground);
    UserLeave();
    return oldPal;
}

/* EOF */
