
#pragma once

using FileHandle_t = void*;
class CUtlBuffer;

#define FILESYSTEM_INVALID_HANDLE	( FileHandle_t )0

enum FileSystemSeek_t
{
    FILESYSTEM_SEEK_HEAD = 0, // SEEK_SET
    FILESYSTEM_SEEK_CURRENT = 1, // SEEK_CUR
    FILESYSTEM_SEEK_TAIL = 2 // SEEK_END
};

enum
{
    FILESYSTEM_INVALID_FIND_HANDLE = -1
};

//-----------------------------------------------------------------------------
// File system allocation functions. Client must free on failure
//-----------------------------------------------------------------------------
typedef void* (*FSAllocFunc_t)(const char* pszFilename, unsigned nBytes);

// This is the minimal interface that can be implemented to provide access to
// a named set of files.
#define BASEFILESYSTEM_INTERFACE_VERSION		"VBaseFileSystem011"

class IBaseFileSystem
{
public:
    // Returns number of bytes read/written.
	virtual int				Read( void* pOutput, int size, FileHandle_t file ) = 0;
	virtual int				Write( void const* pInput, int size, FileHandle_t file ) = 0;

	// if pathID is NULL, all paths will be searched for the file
	virtual FileHandle_t	Open( const char *pFileName, const char *pOptions, const char *pathID = nullptr ) = 0;
	virtual void			Close( FileHandle_t file ) = 0;


	virtual void			Seek( FileHandle_t file, int pos, FileSystemSeek_t seekType ) = 0;
	virtual unsigned int	Tell( FileHandle_t file ) = 0;
	virtual unsigned int	Size( FileHandle_t file ) = 0;
	virtual unsigned int	Size( const char *pFileName, const char *pPathID = nullptr) = 0;

	virtual void			Flush( FileHandle_t file ) = 0;
	virtual bool			Precache( const char *pFileName, const char *pPathID = nullptr) = 0;

	virtual bool			FileExists( const char *pFileName, const char *pPathID = nullptr) = 0;
	virtual bool			IsFileWritable( char const *pFileName, const char *pPathID = nullptr) = 0;
	virtual bool			SetFileWritable( char const *pFileName, bool writable, const char *pPathID = nullptr) = 0;

	virtual long			GetFileTime( const char *pFileName, const char *pPathID = 0 ) = 0;

	//--------------------------------------------------------
	// Reads/writes files to utlbuffers. Use this for optimal read performance when doing open/read/close
	//--------------------------------------------------------
	virtual bool			ReadFile( const char *pFileName, const char *pPath, CUtlBuffer &buf, int nMaxBytes = 0, int nStartingByte = 0, FSAllocFunc_t pfnAlloc = nullptr ) = 0;
	virtual bool			WriteFile( const char *pFileName, const char *pPath, CUtlBuffer &buf ) = 0;
	virtual bool			UnzipFile( const char *pFileName, const char *pPath, const char *pDestination ) = 0;
};
