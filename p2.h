//
// Created by Brenda Tran on 2/12/18.
//

#include <stdio.h>
#include "getword.h"
#include "CHK.h"
#include <signal.h>
#include <stdlib.h>
#include <zconf.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#define MAXITEM 100 // max number of words per line
#define MAXPIPES 10