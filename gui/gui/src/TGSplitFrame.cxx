// @(#)root/gui:$Id$
// Author: Bertrand Bellenot 23/01/2008

/*************************************************************************
 * Copyright (C) 1995-2008, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include "TGFrame.h"
#include "TGLayout.h"
#include "TGMenu.h"
#include "TGSplitter.h"
#include "TGSplitFrame.h"
#include "TGInputDialog.h"
#include "TGResourcePool.h"
#include "TRootContextMenu.h"
#include "TClassMenuItem.h"
#include "TContextMenu.h"
#include "TString.h"
#include "TClass.h"
#include "TList.h"
#include "Riostream.h"

ClassImp(TGSplitTool)
ClassImp(TGSplitFrame)

//______________________________________________________________________________
TGSplitTool::TGSplitTool(const TGWindow *p, const TGFrame *f)
   : TGCompositeFrame(p, 10, 10, kHorizontalFrame | kRaisedFrame | kFixedSize)
{
   // Create a split frame tool tip. P is the tool tips parent window (normally
   // fClient->GetRoot() and f is the frame to which the tool tip is associated.

   SetWindowAttributes_t attr;
   attr.fMask             = kWAOverrideRedirect | kWASaveUnder;
   attr.fOverrideRedirect = kTRUE;
   attr.fSaveUnder        = kTRUE;

   gVirtualX->ChangeWindowAttributes(fId, &attr);
   SetBackgroundColor(fClient->GetResourcePool()->GetTipBgndColor());

   fRectGC.SetFillStyle(kFillSolid);
   fRectGC.SetForeground(0x99ff99);
   
   TClass *cl = TClass::GetClass("TGSplitFrame");
   cl->MakeCustomMenuList();
   TList *ml = cl->GetMenuList();
   ((TClassMenuItem *)ml->At(1))->SetTitle("Cleanup Frame");
   ((TClassMenuItem *)ml->At(2))->SetTitle("Close and Collapse");
   ((TClassMenuItem *)ml->At(3))->SetTitle("Undock Frame");
   ((TClassMenuItem *)ml->At(4))->SetTitle("Dock Frame Back");
   ((TClassMenuItem *)ml->At(5))->SetTitle("Switch to Main");
   ((TClassMenuItem *)ml->At(6))->SetTitle("Horizontally Split...");
   ((TClassMenuItem *)ml->At(7))->SetTitle("Vertically Split...");
   fContextMenu = new TContextMenu("SplitFrameContextMenu", "Actions");
   fMap.SetOwner(kTRUE);
   fMap.SetOwnerValue(kFALSE);
   MapSubwindows();
   if (f) Resize(f->GetWidth()/10, f->GetHeight()/10);
   AddInput(kButtonPressMask | kButtonReleaseMask | kPointerMotionMask);

   fWindow = f;
   fX = fY = -1;
}

//______________________________________________________________________________
TGSplitTool::~TGSplitTool()
{
   // TGSplitTool destructor.

   delete fContextMenu;
}

//______________________________________________________________________________
void TGSplitTool::AddRectangle(TGFrame *frame, Int_t x, Int_t y, Int_t w, Int_t h)
{
   // Add a rectangle representation of a split frame in the map, together 
   // with the frame itself.

   TGRectMap *rect = new TGRectMap(x, y, w, h); 
   fMap.Add(rect, frame);
}

//______________________________________________________________________________
void TGSplitTool::DoRedraw()
{
   // Redraw split frame tool.

   TGRectMap *rect;
   TMapIter next(&fMap);
   while ((rect = (TGRectMap*)next())) {
      gVirtualX->FillRectangle(fId, GetBckgndGC()(), rect->fX, 
                               rect->fY, rect->fW, rect->fH);
      gVirtualX->DrawRectangle(fId, GetBlackGC()(), rect->fX, rect->fY, 
                               rect->fW, rect->fH);
   }
   DrawBorder();
}

//______________________________________________________________________________
void TGSplitTool::DrawBorder()
{
   // Draw border of tool window.

   gVirtualX->DrawLine(fId, GetShadowGC()(), 0, 0, fWidth-2, 0);
   gVirtualX->DrawLine(fId, GetShadowGC()(), 0, 0, 0, fHeight-2);
   gVirtualX->DrawLine(fId, GetBlackGC()(),  0, fHeight-1, fWidth-1, fHeight-1);
   gVirtualX->DrawLine(fId, GetBlackGC()(),  fWidth-1, fHeight-1, fWidth-1, 0);
}

//______________________________________________________________________________
Bool_t TGSplitTool::HandleButton(Event_t *event)
{
   // Handle mouse click events in the tool.

   if (event->fType != kButtonPress)
      return kTRUE;
   Int_t px = 0, py = 0;
   Window_t wtarget;
   TGRectMap *rect;
   TGSplitFrame *frm = 0;
   TMapIter next(&fMap);
   while ((rect = (TGRectMap*)next())) {
      if (rect->Contains(event->fX, event->fY)) {
         frm = (TGSplitFrame *)fMap.GetValue((const TObject *)rect);
         gVirtualX->TranslateCoordinates(event->fWindow,
                                         gClient->GetDefaultRoot()->GetId(), 
                                         event->fX, event->fY, px, py, wtarget);
         fContextMenu->Popup(px, py, frm);
         // connect PoppedDown signal to close the tool window 
         // when the menu is closed
         TRootContextMenu *menu = ((TRootContextMenu *)fContextMenu->GetContextMenuImp());
         ((TGPopupMenu *)menu)->Connect("PoppedDown()", "TGSplitTool", this, "Hide()");
         return kTRUE;
      }
   }
   Hide();
   return kTRUE;
}

//______________________________________________________________________________
Bool_t TGSplitTool::HandleMotion(Event_t *event)
{
   // handle mouse motion events

   static TGRectMap *rect = 0, *oldrect = 0;
   TMapIter next(&fMap);
   while ((rect = (TGRectMap*)next())) {
      if (rect->Contains(event->fX, event->fY)) {
         // if inside a rectangle, highlight it
         if (rect != oldrect) {
            if (oldrect) {
               gVirtualX->FillRectangle(fId, GetBckgndGC()(), oldrect->fX, 
                                        oldrect->fY, oldrect->fW, oldrect->fH);
               gVirtualX->DrawRectangle(fId, GetBlackGC()(), oldrect->fX, oldrect->fY, 
                                        oldrect->fW, oldrect->fH);
            }
            gVirtualX->FillRectangle(fId, fRectGC(), rect->fX, rect->fY, rect->fW, 
                                     rect->fH);
            gVirtualX->DrawRectangle(fId, GetBlackGC()(), rect->fX, rect->fY, 
                                     rect->fW, rect->fH);
            oldrect = rect;
         }
         return kTRUE;
      }
   }
   if (oldrect) {
      gVirtualX->FillRectangle(fId, GetBckgndGC()(), oldrect->fX, 
                               oldrect->fY, oldrect->fW, oldrect->fH);
      gVirtualX->DrawRectangle(fId, GetBlackGC()(), oldrect->fX, oldrect->fY, 
                               oldrect->fW, oldrect->fH);
   }
   return kTRUE;
}
//______________________________________________________________________________
void TGSplitTool::Hide()
{
   // Hide tool window. Use this method to hide the tool in a client class.

   gVirtualX->GrabPointer(0, 0, 0, 0, kFALSE);  // ungrab pointer
   fMap.Delete();
   UnmapWindow();
}

//______________________________________________________________________________
void TGSplitTool::Reset()
{
   // Reset tool tip popup delay timer. Use this method to activate tool tip
   // in a client class.

   fMap.Delete();
}

//______________________________________________________________________________
void TGSplitTool::SetPosition(Int_t x, Int_t y)
{
   // Set popup position within specified frame (as specified in the ctor).
   // To get back default behaviour (in the middle just below the designated
   // frame) set position to -1,-1.

   fX = x;
   fY = y;

   if (fX < -1)
      fX = 0;
   if (fY < -1)
      fY = 0;

   if (fWindow) {
      if (fX > (Int_t) fWindow->GetWidth())
         fX = fWindow->GetWidth();
      if (fY > (Int_t) fWindow->GetHeight())
         fY = fWindow->GetHeight();
   }
}

//______________________________________________________________________________
void TGSplitTool::Show(Int_t x, Int_t y)
{
   // Show tool window.

   Move(x, y);
   MapWindow();
   RaiseWindow();

   // last argument kFALSE forces all specified events to this window
   gVirtualX->GrabPointer(fId, kButtonPressMask | kPointerMotionMask, kNone, 
                          fClient->GetResourcePool()->GetGrabCursor(), 
                          kTRUE, kFALSE);
   // Long_t args[2];
   // args[0] = x;
   // args[1] = y;
}

//______________________________________________________________________________
TGSplitFrame::TGSplitFrame(const TGWindow *p, UInt_t w, UInt_t h,
        UInt_t options) : TGCompositeFrame(p, w, h, options), 
        fFrame(0), fSplitter(0), fFirst(0), fSecond(0)
{
   // Default constructor.

   fSplitTool = new TGSplitTool(gClient->GetDefaultRoot(), this);
   fHRatio = fWRatio = 0.0;
   fUndocked = 0;
   AddInput(kStructureNotifyMask);
   SetCleanup(kLocalCleanup);
}

//______________________________________________________________________________
TGSplitFrame::~TGSplitFrame()
{
   // Destructor. Make cleanup.
   
   delete fSplitTool;
   Cleanup();
}

//______________________________________________________________________________
void TGSplitFrame::AddFrame(TGFrame *f, TGLayoutHints *l)
{
   // Add a frame in the split frame using layout hints l.

   TGCompositeFrame::AddFrame(f, l);
   fFrame = f;
}

//______________________________________________________________________________
void TGSplitFrame::RemoveFrame(TGFrame *f)
{
   // Add a frame in the split frame using layout hints l.

   TGCompositeFrame::RemoveFrame(f);
   if (f == fFrame)
      fFrame = 0;
}

//______________________________________________________________________________
void TGSplitFrame::Cleanup()
{
   // Recursively cleanup child frames.

   TGCompositeFrame::Cleanup();
   fFirst = 0;
   fSecond = 0;
   fSplitter = 0;
   fUndocked = 0;
}

//______________________________________________________________________________
TGSplitFrame *TGSplitFrame::GetTopFrame()
{
   // Return the top level split frame.

   TGSplitFrame *top = this;
   TGWindow *w = (TGWindow *)GetParent();
   TGSplitFrame *p = dynamic_cast<TGSplitFrame *>(w);
   while (p) {
      top = p;
      w = (TGWindow *)p->GetParent();
      p = dynamic_cast<TGSplitFrame *>(w);
   }
   return top;
}

//______________________________________________________________________________
void TGSplitFrame::Close()
{
   // Close (unmap and remove from the list of frames) the frame contained in
   // this split frame.

   if (fFrame) {
      fFrame->UnmapWindow();
      RemoveFrame(fFrame);
   }
   fFrame = 0;
}

//______________________________________________________________________________
void TGSplitFrame::CloseAndCollapse()
{
   // Close (unmap, remove from the list of frames and destroy) the frame 
   // contained in this split frame. Then unsplit the parent frame.

   if (!fSplitter || !fFirst || !fSecond) {
      TGSplitFrame *parent = (TGSplitFrame *)GetParent();
      if (parent->GetFirst() && parent->GetSecond()) {
         if (parent->GetFirst()  == this)
            parent->UnSplit("first");
         else if (parent->GetSecond() == this)
            parent->UnSplit("second");
      }
   }
}
//______________________________________________________________________________
void TGSplitFrame::Docked(TGFrame* frame)
{
   // Emit Undocked() signal.

   Emit("Docked(TGFrame*)", (Long_t)frame);
}

//______________________________________________________________________________
void TGSplitFrame::ExtractFrame()
{
   // Extract the frame contained in this split frame an reparent it in a 
   // transient frame. Keep a pointer on the transient frame to be able to
   // swallow the child frame back to this.

   if (fFrame) {
      fFrame->UnmapWindow();
      fUndocked = new TGTransientFrame(gClient->GetDefaultRoot(), GetMainFrame(), 800, 600);
      fFrame->ReparentWindow(fUndocked);
      fUndocked->AddFrame(fFrame, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));
      // Layout...
      fUndocked->MapSubwindows();
      fUndocked->Layout();
      fUndocked->MapWindow();
      RemoveFrame(fFrame);
      fUndocked->Connect("CloseWindow()", "TGSplitFrame", this, "SwallowBack()");
      Undocked(fFrame);
   }
}

//______________________________________________________________________________
Bool_t TGSplitFrame::HandleConfigureNotify(Event_t *)
{
   // Handles resize events for this frame.
   // This is needed to keep as much as possible the sizes ratio between
   // all subframes.

   if (!fFirst) {
      // case of resizing a frame with the splitter (and not from parent)
      TGWindow *w = (TGWindow *)GetParent();
      TGSplitFrame *p = dynamic_cast<TGSplitFrame *>(w);
      if (p) {
         if (p->GetFirst()) {
            // set the correct ratio for this child
            Float_t hratio = (Float_t)p->GetFirst()->GetHeight() / (Float_t)p->GetHeight();
            Float_t wratio = (Float_t)p->GetFirst()->GetWidth() / (Float_t)p->GetWidth();
            p->SetHRatio(hratio);
            p->SetWRatio(wratio);
         }
      }
      return kTRUE;
   }
   // case of resize event comes from the parent (i.e. by rezing TGMainFrame)
   if ((fHRatio > 0.0) && (fWRatio > 0.0)) {
      Float_t h = fHRatio * (Float_t)GetHeight();
      fFirst->SetHeight((UInt_t)h);
      Float_t w = fWRatio * (Float_t)GetWidth();
      fFirst->SetWidth((UInt_t)w);
   }
   // memorize the actual ratio for next resize event
   fHRatio = (Float_t)fFirst->GetHeight() / (Float_t)GetHeight();
   fWRatio = (Float_t)fFirst->GetWidth() / (Float_t)GetWidth();
   fClient->NeedRedraw(this);
   if (!gVirtualX->InheritsFrom("TGX11"))
      Layout();
   return kTRUE;
}

//______________________________________________________________________________
void TGSplitFrame::HSplit(UInt_t h)
{
   // Horizontally split the frame.

   // return if already splitted
   if ((fSplitter != 0) || (fFirst != 0) || (fSecond != 0) || (fFrame != 0))
      return;
   UInt_t height = (h > 0) ? h : fHeight/2;
   // set correct option (vertical frame)
   ChangeOptions((GetOptions() & ~kHorizontalFrame) | kVerticalFrame);
   // create first split frame with fixed height - required for the splitter
   fFirst = new TGSplitFrame(this, fWidth, height, kSunkenFrame | kFixedHeight);
   // create second split frame
   fSecond = new TGSplitFrame(this, fWidth, height, kSunkenFrame);
   // create horizontal splitter
   fSplitter = new TGHSplitter(this, 4, 4);
   // set the splitter's frame to the first one
   fSplitter->SetFrame(fFirst, kTRUE);
   fSplitter->Connect("ProcessedEvent(Event_t*)", "TGSplitFrame", this, 
                      "OnSplitterClicked(Event_t*)");
   // add all frames
   TGCompositeFrame::AddFrame(fFirst, new TGLayoutHints(kLHintsExpandX));
   TGCompositeFrame::AddFrame(fSplitter, new TGLayoutHints(kLHintsLeft | 
                              kLHintsTop | kLHintsExpandX));
   TGCompositeFrame::AddFrame(fSecond, new TGLayoutHints(kLHintsExpandX | 
                              kLHintsExpandY));
}

//______________________________________________________________________________
void TGSplitFrame::VSplit(UInt_t w)
{
   // Vertically split the frame.

   // return if already splitted
   if ((fSplitter != 0) || (fFirst != 0) || (fSecond != 0) || (fFrame != 0))
      return;
   UInt_t width = (w > 0) ? w : fWidth/2;
   // set correct option (horizontal frame)
   ChangeOptions((GetOptions() & ~kVerticalFrame) | kHorizontalFrame);
   // create first split frame with fixed width - required for the splitter
   fFirst = new TGSplitFrame(this, width, fHeight, kSunkenFrame | kFixedWidth);
   // create second split frame
   fSecond = new TGSplitFrame(this, width, fHeight, kSunkenFrame);
   // create vertical splitter
   fSplitter = new TGVSplitter(this, 4, 4);
   // set the splitter's frame to the first one
   fSplitter->SetFrame(fFirst, kTRUE);
   fSplitter->Connect("ProcessedEvent(Event_t*)", "TGSplitFrame", this, 
                      "OnSplitterClicked(Event_t*)");
   // add all frames
   TGCompositeFrame::AddFrame(fFirst, new TGLayoutHints(kLHintsExpandY));
   TGCompositeFrame::AddFrame(fSplitter, new TGLayoutHints(kLHintsLeft | 
                              kLHintsTop | kLHintsExpandY));
   TGCompositeFrame::AddFrame(fSecond, new TGLayoutHints(kLHintsExpandX | 
                              kLHintsExpandY));
}

//______________________________________________________________________________
void TGSplitFrame::MapToSPlitTool(TGSplitFrame *top)
{
   // Map this split frame in the small overview tooltip.

   Int_t px = 0, py = 0;
   Int_t rx = 0, ry = 0;
   Int_t cx, cy, cw, ch;
   Window_t wtarget;
   if (!fFirst && !fSecond) {
      TGSplitFrame *parent = dynamic_cast<TGSplitFrame *>((TGFrame *)fParent);
      if (parent && parent->fSecond == this) {
         if (parent->GetOptions() & kVerticalFrame)
            ry = parent->GetFirst()->GetHeight();
         if (parent->GetOptions() & kHorizontalFrame)
            rx = parent->GetFirst()->GetWidth();
      }
      gVirtualX->TranslateCoordinates(GetId(), top->GetId(), 
                                      fX, fY, px, py, wtarget);
      cx = ((px-rx)/10)+2;
      cy = ((py-ry)/10)+2;
      cw = (fWidth/10)-4;
      ch = (fHeight/10)-4;
      top->GetSplitTool()->AddRectangle(this, cx, cy, cw, ch);
      return;
   }
   if (fFirst)
      fFirst->MapToSPlitTool(top);
   if (fSecond)
      fSecond->MapToSPlitTool(top);
}

//______________________________________________________________________________
void TGSplitFrame::OnSplitterClicked(Event_t *event)
{
   // Handle mouse click events on the splitter.

   Int_t    px = 0, py = 0;
   Window_t wtarget;
   if (event->fType != kButtonPress)
      return;
   if (event->fCode != kButton3)
      return;
   gVirtualX->TranslateCoordinates(event->fWindow,
                                   gClient->GetDefaultRoot()->GetId(), 
                                   event->fX, event->fY, px, py, wtarget);
   TGSplitFrame *top = GetTopFrame();
   top->GetSplitTool()->Reset();
   top->GetSplitTool()->Resize(1+top->GetWidth()/10, 1+top->GetHeight()/10);
   top->MapToSPlitTool(top);
   top->GetSplitTool()->Show(px, py);
}

//______________________________________________________________________________
void TGSplitFrame::SplitHor()
{
   // Horizontally split the frame, and if it contains a child frame, ask 
   // the user where to keep it (top or bottom). This is the method used 
   // via the context menu.

   char side[200];
   snprintf(side, 200, "top");
   if (fFrame) {
      new TGInputDialog(gClient->GetRoot(), GetTopFrame(),
               "In which side the actual frame has to be kept (top / bottom)", 
               side, side);
      if ( strcmp(side, "") == 0 )  // Cancel button was pressed
         return;
   }
   SplitHorizontal(side);
}

//______________________________________________________________________________
void TGSplitFrame::SplitHorizontal(const char *side)
{
   // Horizontally split the frame, and if it contains a child frame, ask 
   // the user where to keep it (top or bottom). This method is the actual
   // implementation.

   if (fFrame) {
      TGFrame *frame = fFrame;
      frame->UnmapWindow();
      frame->ReparentWindow(gClient->GetDefaultRoot());
      RemoveFrame(fFrame);
      HSplit();
      if (!strcmp(side, "top")) {
         frame->ReparentWindow(GetFirst());
         GetFirst()->AddFrame(frame, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));
      }
      else if (!strcmp(side, "bottom")) {
         frame->ReparentWindow(GetSecond());
         GetSecond()->AddFrame(frame, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));
      }
   }
   else {
      HSplit();
   }
   MapSubwindows();
   Layout();
}

//______________________________________________________________________________
void TGSplitFrame::SplitVer()
{
   // Vertically split the frame, and if it contains a child frame, ask 
   // the user where to keep it (left or right). This is the method used 
   // via the context menu.

   char side[200];
   snprintf(side, 200, "left");
   if (fFrame) {
      new TGInputDialog(gClient->GetRoot(), GetTopFrame(),
               "In which side the actual frame has to be kept (left / right)", 
               side, side);
      if ( strcmp(side, "") == 0 )  // Cancel button was pressed
         return;
   }
   SplitVertical(side);
}

//______________________________________________________________________________
void TGSplitFrame::SplitVertical(const char *side)
{
   // Vertically split the frame, and if it contains a child frame, ask 
   // the user where to keep it (left or right). This method is the actual
   // implementation.

   if (fFrame) {
      TGFrame *frame = fFrame;
      frame->UnmapWindow();
      frame->ReparentWindow(gClient->GetDefaultRoot());
      RemoveFrame(fFrame);
      VSplit();
      if (!strcmp(side, "left")) {
         frame->ReparentWindow(GetFirst());
         GetFirst()->AddFrame(frame, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));
      }
      else if (!strcmp(side, "right")) {
         frame->ReparentWindow(GetSecond());
         GetSecond()->AddFrame(frame, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));
      }
   }
   else {
      VSplit();
   }
   MapSubwindows();
   Layout();
}

//______________________________________________________________________________
void TGSplitFrame::SwallowBack()
{
   // Swallow back the child frame previously extracted, and close its
   // parent (transient frame).

   if (!fUndocked) {
      fUndocked = dynamic_cast<TGTransientFrame *>((TQObject*)gTQSender);
   }
   if (fUndocked) {
      TGFrameElement *el = dynamic_cast<TGFrameElement*>(fUndocked->GetList()->First());
      if (!el || !el->fFrame) return;
      TGSplitFrame *frame = (TGSplitFrame *)el->fFrame;
      frame->UnmapWindow();
      fUndocked->RemoveFrame(frame);
      frame->ReparentWindow(this);
      AddFrame(frame, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));
      // Layout...
      MapSubwindows();
      Layout();
      fUndocked->CloseWindow();
      fUndocked = 0;
      Docked(frame);
   }
}

//______________________________________________________________________________
void TGSplitFrame::SwitchFrames(TGFrame *frame, TGCompositeFrame *dest,
                                TGFrame *prev)
{
   // Switch (exchange) two frames.
   // frame is the source, dest is the destination (the new parent)
   // prev is the frame that has to be exchanged with the source 
   // (the one actually in the destination)

   // get parent of the source (its container)
   TGCompositeFrame *parent = (TGCompositeFrame *)frame->GetParent();

   // unmap the window (to avoid flickering)
   prev->UnmapWindow();
   // remove it from the destination frame
   dest->RemoveFrame(prev);
   // temporary reparent it to root (desktop window)
   prev->ReparentWindow(gClient->GetDefaultRoot());

   // now unmap the source window (still to avoid flickering)
   frame->UnmapWindow();
   // remove it from its parent (its container)
   parent->RemoveFrame(frame);
   // reparent it to the target location
   frame->ReparentWindow(dest);
   // add it to its new parent (for layout managment)
   dest->AddFrame(frame, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));
   // Layout...
   frame->Resize(dest->GetDefaultSize());
   dest->MapSubwindows();
   dest->Layout();

   // now put back the previous one in the previous source parent
   // reparent to the previous source container
   prev->ReparentWindow(parent);
   // add it to the frame (for layout managment)
   parent->AddFrame(prev, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));
   // Layout...
   prev->Resize(parent->GetDefaultSize());
   parent->MapSubwindows();
   parent->Layout();
}

//______________________________________________________________________________
void TGSplitFrame::SwitchToMain()
{
   // Switch the actual embedded frame to the main (first) split frame.

   TGFrame *source = fFrame;
   TGSplitFrame *dest = GetTopFrame()->GetFirst();
   TGFrame *prev = (TGFrame *)(dest->GetFrame());
   if ((source != prev) && (source != dest))
      SwitchFrames(source, dest, prev);
}

//______________________________________________________________________________
void TGSplitFrame::Undocked(TGFrame* frame)
{
   // Emit Undocked() signal.

   Emit("Undocked(TGFrame*)", (Long_t)frame);
}

//______________________________________________________________________________
void TGSplitFrame::UnSplit(const char *which)
{
   // Close (unmap and remove from the list of frames) the frame contained in
   // this split frame.

   TGCompositeFrame *keepframe = 0;
   TGSplitFrame *kframe = 0, *dframe = 0;
   if (!strcmp(which, "first")) {
      dframe = GetFirst();
      kframe = GetSecond();
   }
   else if (!strcmp(which, "second")) {
      dframe = GetSecond();
      kframe = GetFirst();
   }
   if (!kframe || !dframe)
      return;
   keepframe = (TGCompositeFrame *)kframe->GetFrame();
   if (keepframe) {
      keepframe->UnmapWindow();
      keepframe->ReparentWindow(gClient->GetDefaultRoot());
      kframe->RemoveFrame(keepframe);
   }
   Cleanup();
   if (keepframe) {
      keepframe->ReparentWindow(this);
      AddFrame(keepframe, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY));
   }
   MapSubwindows();
   Layout();
}

//______________________________________________________________________________
void TGSplitFrame::SavePrimitive(ostream &out, Option_t *option /*= ""*/)
{
   // Save a splittable frame as a C++ statement(s) on output stream out.

   if (fBackground != GetDefaultFrameBackground()) SaveUserColor(out, option);

   out << endl << "   // splittable frame" << endl;
   out << "   TGSplitFrame *";
   out << GetName() << " = new TGSplitFrame(" << fParent->GetName()
       << "," << GetWidth() << "," << GetHeight();

   if (fBackground == GetDefaultFrameBackground()) {
      if (!GetOptions()) {
         out << ");" << endl;
      } else {
         out << "," << GetOptionString() <<");" << endl;
      }
   } else {
      out << "," << GetOptionString() << ",ucolor);" << endl;
   }
   if (option && strstr(option, "keep_names"))
      out << "   " << GetName() << "->SetName(\"" << GetName() << "\");" << endl;

   // setting layout manager if it differs from the main frame type
   TGLayoutManager * lm = GetLayoutManager();
   if ((GetOptions() & kHorizontalFrame) &&
       (lm->InheritsFrom(TGHorizontalLayout::Class()))) {
      ;
   } else if ((GetOptions() & kVerticalFrame) &&
              (lm->InheritsFrom(TGVerticalLayout::Class()))) {
      ;
   } else {
      out << "   " << GetName() <<"->SetLayoutManager(";
      lm->SavePrimitive(out, option);
      out << ");"<< endl;
   }

   SavePrimitiveSubframes(out, option);
}
