// NOTE: This file is part of the DryWellSuite/OpenHydroQual codebase.
#include "paths.h"

// global variables
QString base;
QString obsPath;
QString ohq_r;

void InitializePaths()
{
#ifdef PowerEdge
    base    = "/mnt/3rd900/Projects/LA Project/";
    obsPath = "/mnt/3rd900/Projects/LA Project new/Results/";
    ohq_r   = "/mnt/3rd900/Projects/OpenHydroQual/resources/";
#endif

#ifdef Hooman
    base    = "/home/hoomanmoradpour/Dropbox/LA Project/Data/";
    obsPath = "/home/hoomanmoradpour/Dropbox/LA Project/Rosemead_Data/";
    ohq_r   = "/home/hoomanmoradpour/Projects/OpenHydroQual/resources/";
#endif

#ifdef Arash
    base    = "/home/arash/Dropbox/LA Project/Data/";
    obsPath = "/home/arash/Dropbox/LA Project/Rosemead_Data/";
    ohq_r   = "/home/arash/Projects/OpenHydroQual/resources/";
#endif

#ifdef Behzad
    base    = "/home/behzad/Dropbox/LA Project/Data/";
    obsPath = "/home/behzad/Projects/LA Project new/Results/";
    ohq_r   = "/home/behzad/Projects/OpenHydroQual/resources/";
#endif

#ifdef SligoCreek
    base    = "/media/arash/E/Projects/LA Project/Data/";
    obsPath = "/media/arash/E/Projects/LA Project new/Results/";
    ohq_r   = "/media/arash/E/Projects/OpenHydroQual/resources/";
#endif
}
