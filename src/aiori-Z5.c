/* -*- mode: c; c-basic-offset: 8; indent-tabs-mode: nil; -*-
 * vim:expandtab:shiftwidth=8:tabstop=8:
 */
/******************************************************************************\
*                                                                              *
*        Copyright (c) 2003, The Regents of the University of California       *
*      See the file COPYRIGHT for a complete copyright notice and license.     *
*                                                                              *
********************************************************************************
*
*  Implement abstract I/O interface for HDF5.
*
\******************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>              /* only for fprintf() */
#include <stdlib.h>
#include <sys/stat.h>
/* HDF5 routines here still use the old 1.6 style.  Nothing wrong with that but
 * save users the trouble of  passing this flag through configure */
#define H5_USE_16_API
#include "z5wrapper.h" 
#include <mpi.h>

#include "aiori.h"              /* abstract IOR interface */
#include "utilities.h"
#include "iordef.h"

#define NUM_DIMS 1              /* number of dimensions to data set */

/******************************************************************************/
/*
 * HDF5_CHECK will display a custom error message and then exit the program
 */

/*
 * should use MPI_Abort(), not exit(), in this macro; some versions of
 * MPI, however, hang with HDF5 property lists et al. left unclosed
 */

/*
 * for versions later than hdf5-1.6, the H5Eget_[major|minor]() functions
 * have been deprecated and replaced with H5Eget_msg()
 */
/**************************** P R O T O T Y P E S *****************************/

static IOR_offset_t SeekOffset(void *, IOR_offset_t, IOR_param_t *);
static void SetupDataSet(void *, IOR_param_t *);
void *Z5_Open(char *, IOR_param_t *);
static void *Z5_Create(char *, IOR_param_t *);
static IOR_offset_t Z5_Xfer(int, void *, IOR_size_t *,
                           IOR_offset_t, IOR_param_t *);
static void Z5_Close(void *, IOR_param_t *);
static void Z5_Delete(char *, IOR_param_t *);
static char* Z5_GetVersion();
static void Z5_Fsync(void *, IOR_param_t *);
static IOR_offset_t Z5_GetFileSize(IOR_param_t *, MPI_Comm, char *);
static int Z5_Access(const char *, int, IOR_param_t *);

/************************** D E C L A R A T I O N S ***************************/

ior_aiori_t z5_aiori = {
        .name = "Z5",
        .name_legacy = NULL,
        .create = Z5_Create,
        .open = Z5_Open,
        .xfer = Z5_Xfer,
        .close = Z5_Close,
        .delete = Z5_Delete,
        .get_version = Z5_GetVersion,
        .fsync = Z5_Fsync,
        .get_file_size = Z5_GetFileSize,
        .statfs = aiori_posix_statfs,
        .mkdir = aiori_posix_mkdir,
        .rmdir = aiori_posix_rmdir,
        .access = Z5_Access,
        .stat = aiori_posix_stat,
};

int newlyOpenedFile;            /* newly opened file */

/***************************** F U N C T I O N S ******************************/
/*
 * Create and open a file through the HDF5 interface.
 */
static void *Z5_Create(char *testFileName, IOR_param_t *param)
{
        Z5_Open(testFileName, param);
}
/*
 * Open a file through the HDF5 interface.
 */
void *Z5_Open(char *testFileName, IOR_param_t * param)
{
        size_t zShape[NUM_DIMS];
        size_t zChunks[NUM_DIMS];
        int ndim = NUM_DIMS;
        int useZlib = 1;
        int level = 1;
        if(param->dryRun)
          return 0;

        size_t fileSize = param->blockSize * param->segmentCount;
        if (param->filePerProc == FALSE) {
                fileSize *= param->numTasks;
        }
        zShape[0] = fileSize;
        zChunks[0] = param->transferSize;
        
        
	if ( rank == 0) {
          z5CreateInt64Dataset(testFileName, ndim, zShape, zChunks, useZlib, level);
	} 

        return ((void*)testFileName);
}

/*
 * Write or read access to file using the HDF5 interface.
 */
static IOR_offset_t Z5_Xfer(int access, void *fd, IOR_size_t * buffer,
                              IOR_offset_t length, IOR_param_t * param)
{

        if(param->dryRun)
          return length;

        int ndim = NUM_DIMS;
        size_t chunks[NUM_DIMS] = {param->transferSize / sizeof(IOR_size_t)};
        size_t offset[NUM_DIMS] = {param->offset / sizeof(IOR_size_t)};
        /* access the file */
        if (access == WRITE) {  /* WRITE */
                z5WriteInt64Subarray((char*)fd, buffer, ndim, chunks, offset);
        } else {                /* READ or CHECK */
                z5ReadInt64Subarray((char*)fd, buffer, ndim, chunks, offset);
        }
        return (length);
}

/*
 * Perform fsync().
 */
static void Z5_Fsync(void *fd, IOR_param_t * param)
{
        ;
}

/*
 * Close a file through the HDF5 interface.
 */
static void Z5_Close(void *fd, IOR_param_t * param)
{
        if(param->dryRun)
          return;
        if (rank == 0){
        }
        //free(fd);
}

/*
 * Delete a file through the HDF5 interface.
 */
static void Z5_Delete(char *testFileName, IOR_param_t * param)
{
  if(param->dryRun)
    return;
  z5Delete(testFileName);
//  MPIIO_Delete(testFileName, param);
  //return;
}

/*
 * Determine api version.
 */
static char * Z5_GetVersion()
{
        ;
}


/*
 * Seek to offset in file using the HDF5 interface and set up hyperslab.
 */
static IOR_offset_t SeekOffset(void *fd, IOR_offset_t offset,
                                            IOR_param_t * param)
{
        return 0;
}


/*
 * Use MPIIO call to get file size.
 */
static IOR_offset_t
Z5_GetFileSize(IOR_param_t * test, MPI_Comm testComm, char *testFileName)
{
  if(test->dryRun)
    return 0;
  return test->blockSize*test->segmentCount*test->numTasks; 
}

/*
 * Use MPIIO call to check for access.
 */
static int Z5_Access(const char *path, int mode, IOR_param_t *param)
{
  if(param->dryRun)
    return 0;
  return(MPIIO_Access(path, mode, param));
}
