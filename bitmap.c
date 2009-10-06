#include "bitmap.h"
#include <stddef.h>
#include <time.h>
#include <unzip32.h>
typedef __int64 ULHA_INT64;
#include <7-zip32.h>
#include <stdio.h>
#include <zlib.h>

#ifndef MAX
#define MAX(a,b) ((a)<(b)?(b):(a))
#endif

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

static void smemcpy(void *a, const void *b, size_t s){
	for(; s; s--){
		*((char*)a)++ = *((const char*)b)++;
	}
}

#define memcpy smemcpy

#if 0
#define FNAME_MAX32		512
#define	FNAME_MAX	FNAME_MAX32

typedef	HGLOBAL	HARCHIVE;
typedef	HGLOBAL	HARC;

typedef struct {
	DWORD	dwOriginalSize;		/* ファイルのサイズ。		*/
	DWORD	dwCompressedSize;	/* 圧縮後のサイズ。		*/
	DWORD	dwCRC;			/* 格納ファイルのチェックサム/CRC */
	UINT	uFlag;			/* 解凍やテストの処理結果	*/
	UINT	uOSType;		/* このファイルの作成に使われたＯＳ。*/
	WORD	wRatio;			/* 圧縮率（パーミル)		*/
	WORD	wDate;			/* 格納ファイルの日付。		*/
	WORD	wTime;			/* 格納ファイルの時刻。		*/
	char	szFileName[FNAME_MAX32 + 1];/* アーカイブファイル名・*/
	char	dummy1[3];
	char	szAttribute[8];		/* 格納ファイルの属性。		*/
	char	szMode[8];		/* 格納ファイルの格納モード。	*/
} INDIVIDUALINFO, FAR *LPINDIVIDUALINFO;

/*static int (WINAPI *pUnZipGetFileCount)(LPCSTR);
static int (WINAPI *UnZip)(const HWND hWnd,LPCSTR szCmdLine,LPSTR szOutput, const DWORD dwSize);
static HARC (WINAPI *UnZipOpenArchive)(const HWND hWnd,LPCSTR szFileName, const DWORD dwMode);
static int (WINAPI *UnZipCloseArchive)(HARC hArc);
static int (WINAPI *UnZipFindFirst)(HARC hArc,
			LPCSTR szWildName,LPINDIVIDUALINFO lpSubInfo);
static DWORD (WINAPI *UnZipGetOriginalSize)(HARC hArc);
static int (WINAPI *UnZipExtractMem)(const HWND hWnd,LPCSTR szCmdLine,
		LPBYTE szBuffer,const DWORD dwSize,time_t *lpTime,
		LPWORD lpwAttr,LPDWORD lpdwWriteSize);*/
#endif

typedef struct _UzpBuffer {
    unsigned long strlength; /* length of string */
    char * strptr; /* pointer to string */
} UzpBuffer;

typedef unsigned short ush;
typedef int (WINAPI DLLPRNT) (LPSTR, unsigned long);
typedef int (WINAPI DLLPASSWORD) (LPSTR, int, LPCSTR, LPCSTR);
typedef int (WINAPI DLLSERVICE) (LPSTR, unsigned long);
typedef void (WINAPI DLLSND) (void);
typedef int (WINAPI DLLREPLACE)(LPSTR);
typedef void (WINAPI DLLMESSAGE)(unsigned long, unsigned long, unsigned,
   unsigned, unsigned, unsigned, unsigned, unsigned,
   char, LPSTR, LPSTR, unsigned long, char);


typedef struct {
DLLPRNT *print;        // = a pointer to the application's print routine.
DLLSND *sound;         /* = a pointer to the application's sound routine. This
                          can be NULL if your application doesn't use
                          sound.*/
DLLREPLACE *replace;    // = a pointer to the application's replace routine.
DLLPASSWORD *password;  // = a pointer to the application's password routine.
DLLMESSAGE *SendApplicationMessage;/* = a pointer to the application's routine
                          for displaying information about specific files
                          in the archive. Used for listing the contents of
                          an archive.*/
DLLSERVICE *ServCallBk;/*  = Callback function designed to be used for
                          allowing the application to process Windows messages,
                          or canceling the operation, as well as giving the
                          option of a progress indicator. If this function
                          returns a non-zero value, then it will terminate
                          what it is doing. It provides the application with
                          the name of the name of the archive member it has
                          just processed, as well as it's original size.
NOTE: The values below are filled in only when listing the contents of an
      archive.
*/
unsigned long TotalSizeComp; /*= value to be filled in by the dll for the
                          compressed total size of the archive. Note this
                          value does not include the size of the archive
                          header and central directory list.*/
unsigned long TotalSize; /*= value to be filled in by the dll for the total
                          size of all files in the archive.*/
int CompFactor;          /*= value to be filled in by the dll for the overall
                          compression factor. This could actually be computed
                          from the other values, but it is available.*/
unsigned int NumMembers; //= total number of files in the archive.
WORD cchComment;        //= flag to be set if archive has a comment
} USERFUNCTIONS, far * LPUSERFUNCTIONS;

__declspec(dllimport) int WINAPI Wiz_UnzipToMemory(LPSTR zip, LPSTR file, LPUSERFUNCTIONS lpUserFunc,
                             UzpBuffer *retstr);
#if 0
#pragma pack(push, 2)
typedef struct sZipLocalHeader{
	DWORD signature; //    local file header signature     4 bytes  (0x04034b50)
    WORD version; // version needed to extract       2 bytes
    WORD flag; //general purpose bit flag        2 bytes
    WORD method; //compression method              2 bytes
    WORD modtime; //last mod file time              2 bytes
    WORD moddate; //last mod file date              2 bytes
    DWORD crc32; //crc-32                          4 bytes
	DWORD csize; //compressed size                 4 bytes
    DWORD usize; //uncompressed size               4 bytes
    WORD namelen; //file name length                2 bytes
    WORD extralen; //extra field length              2 bytes
} ZipLocalHeader;

typedef struct sZipCentralDirHeader
{
    unsigned int signature;   
    unsigned short madever;   
    unsigned short needver;   
    unsigned short option;   
    unsigned short comptype;   
    unsigned short filetime;   
    unsigned short filedate;   
    unsigned int crc32;   
    unsigned int compsize;   
    unsigned int uncompsize;   
    unsigned short fnamelen;   
    unsigned short extralen;   
    unsigned short commentlen;   
    unsigned short disknum;   
    unsigned short inattr;   
    unsigned int outattr;   
    unsigned int headerpos;   
} ZipCentralDirHeader;

typedef struct sZipEndCentDirHeader
{   
    unsigned int signature;   
    unsigned short disknum;   
    unsigned short startdisknum;   
    unsigned short diskdirentry;   
    unsigned short direntry;   
    unsigned int dirsize;   
    unsigned int startpos;   
    unsigned short commentlen;   
} ZipEndCentDirHeader;
#pragma pack(pop, 2)

static void *unzip(const char *fname, const char *ename, unsigned long *psize){
	FILE *fp;
	int reti;
	void *ret = NULL, *cret;
	ZipLocalHeader lh;
	ZipCentralDirHeader cdh;
	ZipEndCentDirHeader ecd;
	char namebuf[256];
	unsigned long usize;
	z_stream z;
	fp = fopen(fname, "rb");
	if(!fp)
		return NULL;
	fseek(fp, -(long)sizeof ecd, SEEK_END);
	fread(&ecd, sizeof ecd, 1, fp);
	if(ecd.signature != 0x06054B50)
		goto error_tag;
	fseek(fp, ecd.startpos, SEEK_SET);
	while(fread(&cdh, sizeof cdh, 1, fp)){
		if(cdh.signature != 0x02014b50 || sizeof namebuf <= cdh.fnamelen)
			goto error_tag;
		fread(namebuf, cdh.fnamelen, 1, fp);
		namebuf[cdh.fnamelen] = '\0';
		fseek(fp, cdh.extralen, SEEK_CUR); // skip extra field
		if(!cdh.fnamelen || strncmp(ename, namebuf, cdh.fnamelen)){
//			fseek(fp, cdh.csize, SEEK_CUR); // skip this entry
//			if(lh.flag & (1<<3))
//				fseek(fp, 12, SEEK_CUR); // skip data descriptor if any
//			fseek(fp, cdh.extralen, SEEK_CUR);
			continue;
		}
		fseek(fp, cdh.headerpos, SEEK_SET);
		fread(&lh, sizeof lh, 1, fp);
		fseek(fp, lh.namelen + lh.extralen, SEEK_CUR);
		cret = malloc(lh.csize);
		fread(cret, lh.csize, 1, fp);
		ret = malloc(lh.usize);
		usize = lh.usize;
		z.zalloc = Z_NULL;
		z.zfree = Z_NULL;
		z.opaque = Z_NULL;
		z.next_in = Z_NULL;
		z.avail_in = 0;
		inflateInit2(&z, -15);
		z.next_in = cret;
		z.avail_in = lh.csize;
		z.next_out = ret;
		z.avail_out = usize;
		reti = inflate(&z, Z_NO_FLUSH);
		inflateEnd(&z);
		free(cret);
		if(psize)
			*psize = usize;
		fprintf(stderr, "%s(%s) %lu/%lu=%lg%%\n", fname, ename, lh.csize, lh.usize, (double)100. * lh.csize / lh.usize);
		break;
	}
error_tag:
	fclose(fp);
	return ret;
}
#endif

BITMAPINFO *ReadBitmap(const char *szFileName)
{
	HWND hWnd = NULL;
    DWORD dwResult;
	DWORD dwFileSize, dwSizeImage;
    HANDLE hF;
    BITMAPFILEHEADER bf;
	BITMAPINFOHEADER bi;
	char *szBuffer;
	HARC arc;
	static int init = 0;

/*	if(!init){
		HMODULE hm = LoadLibrary("..\\unzp\\UNZIP32.DLL");
		pUnZipGetFileCount = (int (WINAPI *)(LPCSTR))GetProcAddress(hm, "UnZipGetFileCount");
		UnZip = (int (WINAPI *)())GetProcAddress(hm, "UnZip");
		UnZipOpenArchive = (int (WINAPI *)())GetProcAddress(hm, "UnZipOpenArchive");
		UnZipCloseArchive = (int (WINAPI *)())GetProcAddress(hm, "UnZipCloseArchive");
		UnZipFindFirst = (int (WINAPI *)())GetProcAddress(hm, "UnZipFindFirst");
		UnZipGetOriginalSize = (int (WINAPI *)())GetProcAddress(hm, "UnZipGetOriginalSize");
		UnZipExtractMem = (int (WINAPI *)())GetProcAddress(hm, "UnZipExtractMem");
		init = 1;
	}*/

	do{
#if 1
		void *p;
		unsigned long size;
		p = ZipUnZip("billiardrc2.zip", szFileName, &size);
		if(p){
			szBuffer = LocalAlloc(LMEM_FIXED, size - sizeof(BITMAPFILEHEADER));
			memcpy(szBuffer, &((BITMAPFILEHEADER*)p)[1], size - sizeof(BITMAPFILEHEADER));
			free(p);
			return szBuffer;
		}
#elif 0
		USERFUNCTIONS uf = {NULL};
		UzpBuffer ub;
		if(Wiz_UnzipToMemory("billiardrc.zip", szFileName, &uf, &ub))
			return ub.strptr;
#elif 1
		INDIVIDUALINFO ii;
		DWORD size;
		arc = SevenZipOpenArchive(NULL, "billiardrc.zip", 0);

		if(!arc)
			break;

		SevenZipFindFirst(arc, szFileName, &ii);

		size = SevenZipGetOriginalSize(arc);

		szBuffer = LocalAlloc(LMEM_FIXED, size);

		if(SevenZipExtractMem(NULL, "", szBuffer, size, NULL, NULL, NULL))
			break;

		if(arc)
			SevenZipCloseArchive(arc);

		return szBuffer;
#else
		INDIVIDUALINFO ii;
		DWORD size;
		arc = UnZipOpenArchive(NULL, "billiardrc.zip", 0);

		if(!arc)
			break;

		UnZipFindFirst(arc, szFileName, &ii);

		size = UnZipGetOriginalSize(arc);

		szBuffer = LocalAlloc(LMEM_FIXED, size);

/*		if(UnZipExtractMem(NULL, "", szBuffer, size, NULL, NULL, NULL))
			break;*/

		if(arc)
			UnZipCloseArchive(arc);

		return szBuffer;
#endif
	}while(0);

    hF = CreateFile(szFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (hF == INVALID_HANDLE_VALUE) {
        return NULL;
    }
    ReadFile(hF, &bf, sizeof(BITMAPFILEHEADER), &dwResult, NULL);
    
	dwFileSize = bf.bfSize;

	if (/*memcmp(&bf.bfType, "BM", 2) != 0*/ bf.bfType != *(WORD*)"BM") {
 /*       MessageBox(hWnd, "ビットマップではありません", "Error", MB_OK);*/
		CloseHandle(hF);
		return NULL;
	}

    ReadFile(hF, &bi, sizeof(BITMAPINFOHEADER), &dwResult, NULL);

	dwSizeImage = bf.bfSize - bf.bfOffBits;

    szBuffer = LocalAlloc(LMEM_FIXED, dwFileSize - sizeof(BITMAPFILEHEADER));

    //読みこむのはBITMAPINFOHEADER, RGBQUAD, ビットデータ
    SetFilePointer(hF, sizeof(BITMAPFILEHEADER), 0, FILE_BEGIN);
    ReadFile(hF, szBuffer,
        dwSizeImage + bi.biClrUsed * sizeof(RGBQUAD) + sizeof(BITMAPINFOHEADER),
        &dwResult, NULL);
    
    CloseHandle(hF);
    return (BITMAPINFO*)szBuffer;
}

void DrawBitmapPaletteTransparent(drawdata_t *dd, const BITMAPINFO *bi, const RGBQUAD *pal, int x0, int y0, const RECT *pr, unsigned tp){
	int dx, ssx;
	RECT r;
	unsigned char *head;
	if(dd->bm.bmBitsPixel != 32 || !dd->bm.bmBits || bi->bmiHeader.biCompression != BI_RGB)
		return;
	if(pr)
		r = *pr;
	else{
		r.left = r.top = 0;
		r.right = bi->bmiHeader.biWidth;
		r.bottom = bi->bmiHeader.biHeight;
	}

	/* mirrored image is specified by setting r.left greater than r.right.
	  in that case, we must tweak the values a little. */
	dx = r.right - r.left;
	ssx = dx < 0 ? -1 : 1;
	dx *= ssx; /* absolute */

	/* in a mirrored image, x0 remains left edge coord, while sx scans opposite direction */
	if(x0 + dx < 0 || dd->bm.bmWidth <= x0 || y0 + (r.bottom - r.top) < 0 || dd->bm.bmWidth <= y0)
		return;

	head = &((unsigned char*)bi)[offsetof(BITMAPINFO, bmiColors) + bi->bmiHeader.biClrUsed * sizeof(RGBQUAD)];

	switch(bi->bmiHeader.biBitCount){
	case 1:
	{
		int x, y, sx, sy;
/*		printf("2 col bpl: %d\n", (bi->bmiHeader.biWidth + 31) / 32 * 4);*/
		for(y = MAX(y0, 0), sy = y - y0 + r.top; y <= MIN(y0 + r.bottom - r.top - 1, dd->bm.bmHeight-1); sy++, y++){
			unsigned char *buf = head + (bi->bmiHeader.biWidth + 31) / 32 * 4 * (bi->bmiHeader.biHeight - sy - 1);
/*			printf("	%p\n", buf);*/
			for(x = MAX(x0, 0), sx = x - x0 + r.left; x <= MIN(x0 + dx - 1, dd->bm.bmWidth-1); sx += ssx, x++){
				int index = 0x1 & (buf[sx / 8] >> (7 - (sx) % 8));
				if(index == tp)
					continue;
				((RGBQUAD*)dd->bm.bmBits)[y * dd->bm.bmWidth + x] = pal[index];
			}
		}
	}
	break;
	case 4:
	{
		int x, y, sx, sy;
		for(y = MAX(y0, 0), sy = y - y0 + r.top; y <= MIN(y0 + r.bottom - r.top - 1, dd->bm.bmHeight-1); sy++, y++){
/*			unsigned char *buf = (unsigned char*)bi + offsetof(BITMAPINFO, bmiColors) + bi->bmiHeader.biClrUsed * sizeof(RGBQUAD) + (bi->bmiHeader.biWidth + 15) / 16 * 8 * (bi->bmiHeader.biHeight - sy - 1);*/
			unsigned char *buf = head + (bi->bmiHeader.biWidth * bi->bmiHeader.biBitCount + 31) / 32 * 4 * (bi->bmiHeader.biHeight - sy - 1);
			for(x = MAX(x0, 0), sx = x - x0 + r.left; x <= MIN(x0 + dx - 1, dd->bm.bmWidth-1); sx += ssx, x++){
				int index = 0xf & (buf[sx / 2] >> ((sx+1) % 2 * 4));
				if(index == tp)
					continue;
				((RGBQUAD*)dd->bm.bmBits)[y * dd->bm.bmWidth + x] = pal[index];
			}
		}
	}
	break;
	case 8:
	{
		int x, y, sx, sy;
		for(y = MAX(y0, 0), sy = y - y0 + r.top; y <= MIN(y0 + r.bottom - r.top - 1, dd->bm.bmHeight-1); sy++, y++){
			unsigned char *buf = head + (bi->bmiHeader.biWidth + 3) / 4 * 4 * (bi->bmiHeader.biHeight - sy - 1);
			for(x = MAX(x0, 0), sx = x - x0 + r.left; x <= MIN(x0 + dx - 1, dd->bm.bmWidth-1); sx += ssx, x++){
				int index = buf[sx];
				if(index == tp)
					continue;
				((RGBQUAD*)dd->bm.bmBits)[y * dd->bm.bmWidth + x] = pal[index];
			}
		}
	}
	break;
	case 24:
	{
		int x, y, sx, sy;
		for(y = MAX(y0, 0), sy = y - y0 + r.top; y <= MIN(y0 + r.bottom - r.top - 1, dd->bm.bmHeight-1); sy++, y++){
			unsigned char *buf = (unsigned char*)bi + offsetof(BITMAPINFO, bmiColors) + bi->bmiHeader.biClrUsed * sizeof(RGBQUAD) + bi->bmiHeader.biWidth * 3 * (bi->bmiHeader.biHeight - sy - 1);
			for(x = MAX(x0, 0), sx = x - x0 + r.left; x <= MIN(x0 + dx - 1, dd->bm.bmWidth-1); sx += ssx, x++){
				RGBQUAD q;
				CopyMemory(&q, &buf[sx * 3], 3);
				q.rgbReserved = 0;
				if(*(unsigned*)&q == tp)
					continue;
				((RGBQUAD*)dd->bm.bmBits)[y * dd->bm.bmWidth + x] = q;
			}
		}
	}
	break;
	case 32:
	{
		int x, y, sx, sy;
		for(y = MAX(y0, 0), sy = y - y0 + r.top; y <= MIN(y0 + r.bottom - r.top - 1, dd->bm.bmHeight-1); sy++, y++){
			unsigned char *buf = (unsigned char*)bi + offsetof(BITMAPINFO, bmiColors) + bi->bmiHeader.biClrUsed * sizeof(RGBQUAD) + bi->bmiHeader.biWidth * 4 * (bi->bmiHeader.biHeight - sy - 1);
			for(x = MAX(x0, 0), sx = x - x0 + r.left; x <= MIN(x0 + dx - 1, dd->bm.bmWidth-1); sx += ssx, x++){
				RGBQUAD *p = &((RGBQUAD*)buf)[sx];
				if(*(unsigned*)p == tp)
					continue;
				((RGBQUAD*)dd->bm.bmBits)[y * dd->bm.bmWidth + x] = *p;
			}
		}
	}
	break;
	}
	return;
}

void DrawBitmapTransparent(drawdata_t *dd, const BITMAPINFO *bi, int x0, int y0, const RECT *r, unsigned tp){
	DrawBitmapPaletteTransparent(dd, bi, bi->bmiColors, x0, y0, r, tp);
}

void DrawBitmapPaletteMask(drawdata_t *dd, const BITMAPINFO *bi, const RGBQUAD *pal, int x0, int y0, const RECT *pr, const BITMAPINFO *mask, unsigned maskindex){
	RECT r;
	if(dd->bm.bmBitsPixel != 32 || !dd->bm.bmBits || bi->bmiHeader.biCompression != BI_RGB)
		return;
	if(pr)
		r = *pr;
	else{
		r.left = r.top = 0;
		r.right = bi->bmiHeader.biWidth;
		r.bottom = bi->bmiHeader.biHeight;
	}
	if(x0 + (r.right - r.left) < 0 || dd->bm.bmWidth <= x0 || y0 + (r.bottom - r.top) < 0 || dd->bm.bmWidth <= y0)
		return;
	if(bi->bmiHeader.biBitCount == 4 && mask->bmiHeader.biBitCount == 1){
		int x, y, sx, sy, mx, my;
		for(y = MAX(y0, 0), sy = y - y0 + r.top, my = sy - r.top; y <= MIN(y0 + r.bottom - r.top - 1, dd->bm.bmHeight-1); sy++, y++, my++){
			unsigned char *buf = (unsigned char*)bi + offsetof(BITMAPINFO, bmiColors) + bi->bmiHeader.biClrUsed * sizeof(RGBQUAD) + (bi->bmiHeader.biWidth + 1) / 2 * (bi->bmiHeader.biHeight - sy - 1);
			unsigned char *mbuf = (unsigned char*)mask + offsetof(BITMAPINFO, bmiColors) + mask->bmiHeader.biClrUsed * sizeof(RGBQUAD) + (mask->bmiHeader.biWidth + 7) / 8 * (mask->bmiHeader.biHeight - my - 1);
			for(x = MAX(x0, 0), sx = x - x0 + r.left, mx = sx - r.left; x <= MIN(x0 + r.right - r.left - 1, dd->bm.bmWidth-1); sx++, x++, mx++){
				int index = 0xf & (buf[sx / 2] >> ((sx+1) % 2 * 4));
				if(((mbuf[mx / 8] >> (7 - mx % 8)) & 1) == maskindex)
					continue;
				((RGBQUAD*)dd->bm.bmBits)[y * dd->bm.bmWidth + x] = pal[index];
			}
		}
	}
}

void DrawBitmapMask(drawdata_t *dd, const BITMAPINFO *bi, int x0, int y0, const RECT *pr, const BITMAPINFO *mask, unsigned maskindex){
	DrawBitmapPaletteMask(dd, bi, bi->bmiColors, x0, y0, pr, mask, maskindex);
}

