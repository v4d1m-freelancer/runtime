// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

/*============================================================
** 
** Source Code: main.c and semaphore.c
**     main.c creates process and waits for all processes to get over
**     semaphore.c creates a semaphore and then calls threads which will contend for the semaphore
** 
** This test is for Object Management Test case for semaphore where Object type is shareable.
** Algorithm
** o	Main Process Creates OBJECT_TYPE Object
** o	Create PROCESS_COUNT processes aware of the Shared Object 
** 
**
**
**============================================================
*/

#include <palsuite.h>
#include "resulttime.h"

/* Test Input Variables */
unsigned int PROCESS_COUNT = 1;
unsigned int THREAD_COUNT = 1;
unsigned int REPEAT_COUNT = 4;
unsigned int RELATION_ID  = 1001;


unsigned long lInitialCount = 1; /* Signaled */
unsigned long lMaximumCount = 1; /* Maximum value of 1 */

char objectSuffix[MAX_PATH];

struct TestStats{
    DWORD        operationTime;
    unsigned int relationId;
    unsigned int processCount;
    unsigned int threadCount;
    unsigned int repeatCount;
    char*        buildNumber;

};

int GetParameters( int argc, char **argv)
{
    if( (!((argc == 5) || (argc == 6) ) )|| ((argc == 1) && !strcmp(argv[1],"/?")) 
       || !strcmp(argv[1],"/h") || !strcmp(argv[1],"/H"))
    {
        printf("PAL -Composite Object Management event Test\n");
        printf("Usage:\n");
        printf("main\n\t[PROCESS_COUNT (greater than 1)] \n"); 
        printf("\t[THREAD_COUNT (greater than 1)] \n"); 
        printf("\t[REPEAT_COUNT (greater than 1)]\n");
        printf("\t[RELATION_ID  [greater than or equal to 1]\n");        
	 printf("\t[Object Name Suffix]\n");
        return -1;
    }

    PROCESS_COUNT = atoi(argv[1]);
    if( (PROCESS_COUNT < 1) || (PROCESS_COUNT > MAXIMUM_WAIT_OBJECTS) ) 
    {
        printf("\nMain Process:Invalid PROCESS_COUNT number, Pass greater than 1 and less than PROCESS_COUNT %d\n", MAXIMUM_WAIT_OBJECTS);
        return -1;
    }

    THREAD_COUNT = atoi(argv[2]);
    if( (THREAD_COUNT < 1) || (THREAD_COUNT > MAXIMUM_WAIT_OBJECTS) )
    {
        printf("\nInvalid THREAD_COUNT number, Pass greater than 1 and less than %d\n", MAXIMUM_WAIT_OBJECTS);
        return -1;
    }

    REPEAT_COUNT = atoi(argv[3]);
    if( REPEAT_COUNT < 1) 
    {
        printf("\nMain Process:Invalid REPEAT_COUNT number, Pass greater than 1\n");
        return -1;
    }

    RELATION_ID = atoi(argv[4]);
    if( RELATION_ID < 1) 
    {
        printf("\nMain Process:Invalid RELATION_ID number, Pass greater than 1\n");
        return -1;
    }

	
    if(argc == 6)
    {
        strncpy(objectSuffix, argv[5], MAX_PATH-1);
    }

    return 0;
}

 int __cdecl main(INT argc, CHAR **argv)
{
    unsigned int i = 0;
    HANDLE hProcess[MAXIMUM_WAIT_OBJECTS];
    HANDLE hSemaphoreHandle;

    STARTUPINFO si[MAXIMUM_WAIT_OBJECTS];
    PROCESS_INFORMATION pi[MAXIMUM_WAIT_OBJECTS];

    char lpCommandLine[MAX_PATH] = "";
    char ObjName[MAX_PATH] = "SHARED_SEMAPHORE";

    int returnCode = 0;
    DWORD processReturnCode = 0;
    int testReturnCode = PASS;

    char fileName[MAX_PATH];
    FILE *pFile = NULL;
    DWORD dwStartTime;
    struct TestStats testStats;

    if(0 != (PAL_Initialize(argc, argv)))
    {
        return ( FAIL );
    }

/*
"While the new PAL does support named semaphore it's unclear 
if we should change the Windows PAL, since we share that w/ Rotor 
and they are still using the old PAL. For the time being it may 
make the most sense to just skip the named semaphore test on Windows 
- from an object management perspective it doesn't really gain 
us anything over what we already have."
*/
    ZeroMemory( objectSuffix, MAX_PATH );

    if(GetParameters(argc, argv))
    {
        Fail("Error in obtaining the parameters\n");
    }
    
    if(argc == 6)
    {
        strncat(ObjName, objectSuffix, MAX_PATH - (sizeof(ObjName) + 1) );
    }

     /* Register the start time */  
    dwStartTime = GetTickCount();
    testStats.relationId   = RELATION_ID;
    testStats.processCount = PROCESS_COUNT;
    testStats.threadCount  = THREAD_COUNT;
    testStats.repeatCount  = REPEAT_COUNT;
    testStats.buildNumber  = getBuildNumber();

    _snprintf(fileName, MAX_PATH, "main_semaphore_%d_.txt", RELATION_ID);
    pFile = fopen(fileName, "w+");
    if(pFile == NULL)
    { 
        Fail("Error in opening main file for write\n");
    }

    hSemaphoreHandle = CreateSemaphore(
                                NULL, /* lpSemaphoreAttributes */
                                lInitialCount, /*lInitialCount*/
                                lMaximumCount, /*lMaximumCount */
                                ObjName  
                               );
            
    if( hSemaphoreHandle == NULL)
    {
        Fail("Unable to create shared Semaphore handle @ Main returned error [%d]\n", GetLastError());
    }
    
    for( i = 0; i < PROCESS_COUNT; i++ )
    {


        ZeroMemory( lpCommandLine, MAX_PATH );
        if ( _snprintf( lpCommandLine, MAX_PATH-1, "semaphore %d %d %d %d %s", i, THREAD_COUNT, REPEAT_COUNT, RELATION_ID, objectSuffix) < 0 )
        {
            Fail("Error: Insufficient semaphore name string length for %s for iteration [%d]\n", ObjName, i);
        }

       
        /* Zero the data structure space */
        ZeroMemory ( &pi[i], sizeof(pi[i]) );
        ZeroMemory ( &si[i], sizeof(si[i]) );

        /* Set the process flags and standard io handles */
        si[i].cb = sizeof(si[i]);

        if(!CreateProcess( NULL, /* lpApplicationName*/
                          lpCommandLine, /* lpCommandLine */
                          NULL, /* lpProcessAttributes  */
                          NULL, /* lpThreadAttributes */
                          TRUE, /* bInheritHandles */
                          0, /* dwCreationFlags, */
                          NULL, /* lpEnvironment  */
                          NULL, /* pCurrentDirectory  */
                          &si[i], /* lpStartupInfo  */
                          &pi[i] /* lpProcessInformation  */
                          ))
        {
            Fail("Process Not created for [%d], the error code is [%d]\n", i, GetLastError());
        }
        else
        {
            hProcess[i] = pi[i].hProcess;
//            Trace("Process created for [%d]\n", i);

        }

    }

    returnCode = WaitForMultipleObjects( PROCESS_COUNT, hProcess, TRUE, INFINITE);  
    if( WAIT_OBJECT_0 != returnCode )
    {
        Trace("Wait for Object(s) @ Main thread for %d processes returned %d, and GetLastError value is %d\n", PROCESS_COUNT, returnCode, GetLastError());
        testReturnCode = FAIL;
    }

    for( i = 0; i < PROCESS_COUNT; i++ )
    {
        /* check the exit code from the process */
        if( ! GetExitCodeProcess( pi[i].hProcess, &processReturnCode ) )
        {
            Trace( "GetExitCodeProcess call failed for iteration %d with error code %u\n", 
                i, GetLastError() ); 
           
            testReturnCode = FAIL;
        }

        if(processReturnCode == FAIL)
        {
            Trace( "Process [%d] failed and returned FAIL\n", i); 
            testReturnCode = FAIL;
        }

        if(!CloseHandle(pi[i].hThread))
        {
            Trace("Error:%d: CloseHandle failed for Process [%d] hThread\n", GetLastError(), i);
            testReturnCode = FAIL;
        }

        if(!CloseHandle(pi[i].hProcess) )
        {
            Trace("Error:%d: CloseHandle failed for Process [%d] hProcess\n", GetLastError(), i);
            testReturnCode = FAIL;
        }
    }

    testStats.operationTime = GetTimeDiff(dwStartTime); 
    fprintf(pFile, "%d,%d,%d,%d,%d,%s\n", testStats.operationTime, testStats.relationId, testStats.processCount, testStats.threadCount, testStats.repeatCount, testStats.buildNumber);
    if(fclose(pFile))
    {
        Trace("Error: fclose failed for pFile\n");
        testReturnCode = FAIL;
    };

    if(!CloseHandle(hSemaphoreHandle))
    {
        Trace("Error:%d: CloseHandle failed for hSemaphoreHandle\n", GetLastError());
        testReturnCode = FAIL;
        
    } 

    if( testReturnCode == PASS)
    {
        Trace("Test Passed\n");
    }
    else
    {
        Trace("Test Failed\n");
    }

    PAL_Terminate();
    return testReturnCode;
}