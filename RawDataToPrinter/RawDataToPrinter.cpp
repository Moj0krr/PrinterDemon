﻿#include <Windows.h>
#include <StdIO.h>
#include <tchar.h>


//**********************************************************************
//
// https://blog.csdn.net/kingmax54212008/article/details/8985965
// https://docs.microsoft.com/en-us/windows/win32/printdocs/sending-data-directly-to-a-printer
//
// **********************************************************************
// PrintError - uses _tprintf() to display error code information
// 
// Params:
//     dwError       - the error code, usually from GetLastError()
//     lpString      - some caller-defined text to print with the error info
// 
// Returns: void
// **********************************************************************

void PrintError(DWORD dwError, LPCTSTR lpString)
{
#define MAX_MSG_BUF_SIZE 512
    TCHAR* msgBuf;
    DWORD   cMsgLen;

    cMsgLen = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 40, NULL, dwError,
        MAKELANGID(0, SUBLANG_ENGLISH_US), (LPTSTR)&msgBuf,
        MAX_MSG_BUF_SIZE, NULL);
    _tprintf(TEXT("%s Error [%d]:: %s\n"), lpString, dwError, msgBuf);
    LocalFree(msgBuf);
#undef MAX_MSG_BUF_SIZE
}


// **********************************************************************
// ReadFileWithAlloc - allocates memory for and reads contents of a file
// 
// Params:
//   szFileName   - NULL terminated string specifying file name
//   pdwSize      - address of variable to receive file bytes size
//   ppBytes      - address of pointer which will be allocated and contain file bytes
// 
// Returns: TRUE for success, FALSE for failure.
//
// Notes: Caller is responsible for freeing the memory using GlobalFree()
// **********************************************************************

BOOL ReadFileWithAlloc(LPTSTR szFileName, LPDWORD pdwSize, LPBYTE* ppBytes)
{
    HANDLE      hFile;
    DWORD       dwBytes;
    BOOL        bSuccess = FALSE;

    // Validate pointer parameters
    if ((pdwSize == NULL) || (ppBytes == NULL))
        return FALSE;

    // Open the file for reading
    hFile = CreateFile(szFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        PrintError(GetLastError(), TEXT("CreateFile()"));
        return FALSE;
    }

    // How big is the file?
    *pdwSize = GetFileSize(hFile, NULL);
    if (*pdwSize == (DWORD)-1)
        PrintError(GetLastError(), TEXT("GetFileSize()"));
    else
    {
        // Allocate the memory
        *ppBytes = (LPBYTE)GlobalAlloc(GPTR, *pdwSize);
        if (*ppBytes == NULL)
            PrintError(GetLastError(), TEXT("Failed to allocate memory\n"));
        else
        {
            // Read the file into the newly allocated memory
            bSuccess = ReadFile(hFile, *ppBytes, *pdwSize, &dwBytes, NULL);
            if (!bSuccess)
                PrintError(GetLastError(), TEXT("ReadFile()"));
        }
    }

    // Clean up
    CloseHandle(hFile);
    return bSuccess;
}


// **********************************************************************
// RawDataToPrinter - sends binary data directly to a printer
// 
// Params:
//   szPrinterName - NULL terminated string specifying printer name
//   lpData        - Pointer to raw data bytes
//   dwCount       - Length of lpData in bytes
// 
// Returns: TRUE for success, FALSE for failure.
// **********************************************************************

BOOL RawDataToPrinter(LPTSTR szPrinterName, LPBYTE lpData, DWORD dwCount)
{
    HANDLE     hPrinter;
    DOC_INFO_1 DocInfo;
    DWORD      dwJob;
    DWORD      dwBytesWritten;

    // Need a handle to the printer.
    if (!OpenPrinter(szPrinterName, &hPrinter, NULL))
    {
        PrintError(GetLastError(), TEXT("OpenPrinter"));
        return FALSE;
    }

    // Fill in the structure with info about this "document."
    DocInfo.pDocName = (LPTSTR)TEXT("Document");
    DocInfo.pOutputFile = NULL;
    DocInfo.pDatatype = (LPTSTR)TEXT("RAW");

    // Inform the spooler the document is beginning.
    if ((dwJob = StartDocPrinter(hPrinter, 1, (LPBYTE)&DocInfo)) == 0)
    {
        PrintError(GetLastError(), TEXT("StartDocPrinter"));
        ClosePrinter(hPrinter);
        return FALSE;
    }

    // Start a page.
    if (!StartPagePrinter(hPrinter))
    {
        PrintError(GetLastError(), TEXT("StartPagePrinter"));
        EndDocPrinter(hPrinter);
        ClosePrinter(hPrinter);
        return FALSE;
    }

    // Send the data to the printer.
    if (!WritePrinter(hPrinter, lpData, dwCount, &dwBytesWritten))
    {
        PrintError(GetLastError(), TEXT("WritePrinter"));
        EndPagePrinter(hPrinter);
        EndDocPrinter(hPrinter);
        ClosePrinter(hPrinter);
        return FALSE;
    }

    // End the page.
    if (!EndPagePrinter(hPrinter))
    {
        PrintError(GetLastError(), TEXT("EndPagePrinter"));
        EndDocPrinter(hPrinter);
        ClosePrinter(hPrinter);
        return FALSE;
    }

    // Inform the spooler that the document is ending.
    if (!EndDocPrinter(hPrinter))
    {
        PrintError(GetLastError(), TEXT("EndDocPrinter"));
        ClosePrinter(hPrinter);
        return FALSE;
    }

    // Tidy up the printer handle.
    ClosePrinter(hPrinter);

    // Check to see if correct number of bytes were written.
    if (dwBytesWritten != dwCount)
    {
        _tprintf(TEXT("Wrote %d bytes instead of requested %d bytes.\n"), dwBytesWritten, dwCount);
        return FALSE;
    }

    return TRUE;
}


// **********************************************************************
// main - entry point for this console application
// 
// Params:
//   argc        - count of command line arguments
//   argv        - array of NULL terminated command line arguments
//
// Returns: 0 for success, non-zero for failure.
// 
// Command line: c:\>RawPrint PrinterName FileName
//               sends raw data file to printer using spooler APIs
//               written nov 1999 jmh
// **********************************************************************

int _tmain(int argc, TCHAR* argv[])
{
    LPBYTE  pBytes = NULL;
    DWORD   dwSize = 0;

    if (argc != 3)
        return _tprintf(TEXT("Syntax: %s <PrinterName> <FileName>\n"), argv[0]);

    _tprintf(TEXT("Attempting to send file [%s] to printer [%s].\n"), argv[2], argv[1]);

    if (!ReadFileWithAlloc(argv[2], &dwSize, &pBytes))
        return _tprintf(TEXT("Failed to allocate memory for and read file [%s].\n"), argv[2]);

    if (!RawDataToPrinter(argv[1], pBytes, dwSize))
        _tprintf(TEXT("Failed to send data to printer.\n"));
    else
        _tprintf(TEXT("Data sent to printer.\n"));

    GlobalFree((HGLOBAL)pBytes);

    return 0;
}
