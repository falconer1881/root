// @(#)root/test:$Id$
// Author: David Smith   20/10/14

/////////////////////////////////////////////////////////////////
//
//___A test for I/O plugins by reading files___
//
//   The files used in this test have been generated by
//   stress.cxx and preplaced on some data servers.
//   stressReadOnly reads the remote files via various data
//   access protocols to test ROOT IO plugins. The data read are
//   tested via tests based on some of stress.cxx tests.
//
//   Can be run as:
//     stressReadOnly [name]
//   The 1st parameter is a scheme name, as expected in a url
//   e.g. xroot
//
// An example of output when all the tests run OK is shown below:
//
// *  Starting  R O O T - S T R E S S (READ ONLY) for protocol xroot
// Test  1 : Check size & compression factor of a Root file........ OK
// Test  2 : Test graphics & Postscript............................ OK
// Test  3 : Trees split and compression modes..................... OK
//
//_____________________________batch only_____________________
#ifndef __CINT__

#include <stdlib.h>
#include <TROOT.h>
#include <TSystem.h>
#include <TH1.h>
#include <TH2.h>
#include <TFile.h>
#include <TMath.h>
#include <TF1.h>
#include <TF2.h>
#include <TCanvas.h>
#include <TPostScript.h>
#include <TTree.h>
#include <TTreeCache.h>
#include <TSystem.h>
#include <TApplication.h>
#include <TClassTable.h>
#include <Compression.h>
#include "Event.h"

void stressReadOnly();
void stressReadOnly(const char *protoName);
void stressReadOnly1();
void stressReadOnly2();
void stressReadOnly3();
void cleanup();

int main(int argc, char **argv)
{
   gROOT->SetBatch();
   TApplication theApp("App", &argc, argv);
   const char *proto = 0;
   if (argc > 1)  proto = argv[1];
   stressReadOnly(proto);
   return 0;
}

#endif

class TH1;
class TTree;

#if defined(__CINT__) || defined(__CLING__)
struct ensureEventLoaded {
   public:
   ensureEventLoaded() {
     // if needed load the Event shard library, making sure it exists
     // This test dynamic linking when running in interpreted mode
     if (!TClassTable::GetDict("Event")) {
        Int_t st1 = -1;
        if (gSystem->DynamicPathName("$ROOTSYS/test/libEvent",kTRUE)) {
           st1 = gSystem->Load("$(ROOTSYS)/test/libEvent");
        }
        if (st1 == -1) {
           if (gSystem->DynamicPathName("test/libEvent",kTRUE)) {
              st1 = gSystem->Load("test/libEvent");
           }
           if (st1 == -1) {
              printf("===>stress8 will try to build the libEvent library\n");
              Bool_t UNIX = strcmp(gSystem->GetName(), "Unix") == 0;
              if (UNIX) gSystem->Exec("(cd $ROOTSYS/test; make Event)");
              else      gSystem->Exec("(cd %ROOTSYS%\\test && nmake libEvent.dll)");
              st1 = gSystem->Load("$(ROOTSYS)/test/libEvent");
           }
        }
     }
   }
   ~ensureEventLoaded() { }
} mytmp;
#endif

//_______________________ common part_________________________

Double_t ntotin=0, ntotout=0;
TString gPfx;

int setPath(const char *proto)
{
   if (!proto) return -1;
   TString p(proto);
   if (p == "root" || p == "xroot") {
      gPfx = "root://eospublic.cern.ch//eos/opstest/dhsmith/ReadOnlyTestFiles/";
      return 0;
   }
   return -1;
}

void stressReadOnly(const char *protoName)
{
   //Main control function invoking all test programs
   if (!protoName) {
     stressReadOnly("xroot");
     return;
   }

   printf("*  Starting  R O O T - S T R E S S (READ ONLY) for protocol %s\n", protoName);

   if (setPath(protoName)) {
     printf("No server and path available to test protocol %s\n", protoName);
     return;
   }

   stressReadOnly1();
   stressReadOnly2();
   stressReadOnly3();

   cleanup();
}

void stressReadOnly()
{
   stressReadOnly((const char*)0);
}

void Bprint(Int_t id, const char *title)
{
  // Print test program number and its title
   const Int_t kMAX = 65;
   char header[80];
   snprintf(header,80,"Test %2d : %s",id,title);
   Int_t nch = strlen(header);
   for (Int_t i=nch;i<kMAX;i++) header[i] = '.';
   header[kMAX] = 0;
   header[kMAX-1] = ' ';
   printf("%s",header);
}

//_______________________________________________________________
void stressReadOnly1()
{
   //based on stress2()
   //check length and compression factor in stress.root
   Bprint(1,"Check size & compression factor of a Root file");

   TFile *f = TFile::Open(gPfx + "stress_2.root");
   if (!f) {
     printf("FAILED\n");
     printf("Could not open test file for stressReadOnly2\n");
     return;
   }

   Long64_t last = f->GetEND();
   Float_t comp = f->GetCompressionFactor();

   Bool_t OK = kTRUE;
   Long64_t lastgood = 9428;
   if (last <lastgood-200 || last > lastgood+200 || comp <2.0 || comp > 2.4) OK = kFALSE;
   if (OK) printf("OK\n");
   else    {
      printf("FAILED\n");
      printf("%-8s last =%lld, comp=%f\n"," ",last,comp);
   }
   delete f;
}

//_______________________________________________________________
void stressReadOnly2()
{
// based on stress5()
// Test of Postscript.
// Make a complex picture. Verify number of lines on ps file
// Testing automatically the graphics package is a complex problem.
// The best way we have found is to generate a Postscript image
// of a complex canvas containing many objects.
// The number of lines in the ps file is compared with a reference run.
// A few lines (up to 2 or 3) of difference may be expected because
// Postscript works with floats. The date and time of the run are also
// different.
// You can also inspect visually the ps file with a ps viewer.

   Bprint(2,"Test graphics & Postscript");

   TCanvas *c1 = new TCanvas("c1","stress canvas",800,600);
   gROOT->LoadClass("TPostScript","Postscript");
   TPostScript ps("stressro.ps",112);

   //Get objects generated in previous test
   TFile *f = TFile::Open(gPfx + "stress_5.root");
   if (!f) {
     printf("FAILED\n");
     printf("Could not open test file for stressReadOnly5\n");
     return;
   }

   TF1  *f1form = (TF1*)f->Get("f1form");
   TF2  *f2form = (TF2*)f->Get("f2form");
   TH1F *h1form = (TH1F*)f->Get("h1form");
   TH2F *h2form = (TH2F*)f->Get("h2form");

   //Divide the canvas in subpads. Plot with different options
   c1->Divide(2,2);
   c1->cd(1);
   f1form->Draw();
   c1->cd(2);
   h1form->Draw();
   c1->cd(3);
   h2form->Draw("box");
   f2form->Draw("cont1same");
   c1->cd(4);
   f2form->Draw("surf");

   ps.Close();

   //count number of lines in ps file
   FILE *fp = fopen("stressro.ps","r");
   char line[260];
   Int_t nlines = 0;
   Int_t nlinesGood = 632;
   while (fgets(line,255,fp)) {
      nlines++;
   }
   fclose(fp);
   ntotin  += f->GetBytesRead();
   ntotout += f->GetBytesWritten();
   Bool_t OK = kTRUE;
   if (nlines < nlinesGood-110 || nlines > nlinesGood+110) OK = kFALSE;
   if (OK) printf("OK\n");
   else    {
      printf("FAILED\n");
      printf("%-8s nlines in stressro.ps file = %d\n"," ",nlines);
   }
   delete c1;
   delete f;
}

//_______________________________________________________________
Int_t test3read(const TString &fn)
{

//  Read the event file
//  Loop on all events in the file (reading everything).
//  Count number of bytes read

   Int_t nevent = 0;

   TFile *hfile = TFile::Open(gPfx + fn);
   if (!hfile) {
      printf("Could not open event file during stressReadOnly8\n");
      return 0;
   }
   TTree *tree; hfile->GetObject("T",tree);
   Event *event = 0;
   tree->SetBranchAddress("event",&event);
   Int_t nentries = (Int_t)tree->GetEntries();
   Int_t nev = TMath::Max(nevent,nentries);
   //activate the treeCache
   Int_t cachesize = 10000000; //this is the default value: 10 MBytes
   tree->SetCacheSize(cachesize);
   TTreeCache::SetLearnEntries(1); //one entry is sufficient to learn
   TTreeCache *tc = (TTreeCache*)hfile->GetCacheRead();
   tc->SetEntryRange(0,nevent);
   Int_t nb = 0;
   for (Int_t ev = 0; ev < nev; ev++) {
      nb += tree->GetEntry(ev);        //read complete event in memory
   }
   ntotin  += hfile->GetBytesRead();

   delete event;
   delete hfile;
   return nb;
}


//_______________________________________________________________
void stressReadOnly3()
{
   // based on stress8()
   Bprint(3,"Trees split and compression modes");

   Int_t nbr0 = test3read("Event_8a.root");
   Event::Reset();

   Int_t nbr1 = test3read("Event_8b.root");
   Event::Reset();

   Bool_t OK = kTRUE;
   if (nbr0 != nbr1) OK = kFALSE;
   if (OK) printf("OK\n");
   else    {
      printf("FAILED\n");
      printf("%-8s nbr0=%d, nbr1=%d\n"," ",nbr0,nbr1);
   }
}

void cleanup()
{
   gSystem->Unlink("stressro.ps");
}
