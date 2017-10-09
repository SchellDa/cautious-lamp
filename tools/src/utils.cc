#include <TROOT.h>
#include "utils.h"
TH1 *create_h1(const char *name, const char *tit, int n, double x1, double x2)
{
    TH1 *hst = (TH1 *)gROOT->FindObject(name);
    if (hst)
        delete hst;
    
    hst = new TH1D(name, tit, n, x1, x2);
    return hst;
}

TH2 *create_h2(const char *name, const char *tit, int nx, double x1, double x2, int ny, double y1, double y2)
{
    TH2 *hst = (TH2 *)gROOT->FindObject(name);
    if (hst)
        delete hst;
    
    hst = new TH2D(name, tit, nx, x1, x2, ny, y1, y2);
    return hst;
}

TProfile *create_profile(const char *name, const char *tit, int n, double x1, double x2, double y1, double y2)
{
    TProfile *hst = (TProfile *)gROOT->FindObject(name);
    if (hst)
        delete hst;
    
    hst = new TProfile(name, tit, n, x1, x2, y1, y2);
    return hst;
}
